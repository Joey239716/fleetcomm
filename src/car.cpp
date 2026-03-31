#include "graph.hpp"
#include "car.hpp"
#include "v2v_message.hpp"
#include <cmath>
#include <algorithm>

void Car::applyHumanDriving(std::mt19937& rng, const RoadGraph& graph) {
    if (status != CarStatus::Active) return;

    constexpr float MIN_SPEED = 10.0f;
    const float     MAX_SPEED = graph.getEdgeByIndex(currentEdgeIdx).speedLimit;

    // Random speed fluctuation — human inconsistency
    std::uniform_real_distribution<float> fluctuation(-2.0f, 2.0f);
    speed = std::clamp(speed + fluctuation(rng), MIN_SPEED, MAX_SPEED);

    // Guard — no next node to look toward if on last segment
    if (routeIdx >= (int)route.size() - 1) return;

    const Node& currentNode = graph.getNode(route[routeIdx]);
    const Node& nextNode    = graph.getNode(route[routeIdx + 1]);

    // Check vision cone — brake if any visible car is ahead, recover speed if clear
    bool carAhead = false;
    for (auto& [peerId, peer] : knownPeers) {
        if (!canSee(*peer, currentNode, nextNode)) continue;
        carAhead = true;
        break;
    }

    if (carAhead) {
        // Late braking — sudden reduction simulating human reaction
        speed = std::max(speed * 0.5f, MIN_SPEED);
    } else {
        // Road is clear — gradually accelerate back toward the speed limit
        speed = std::min(speed + 3.0f, MAX_SPEED);
    }
}

bool Car::move(const RoadGraph& graph) {
    if (status != CarStatus::Active) return false;

    const Edge& currentEdge = graph.getEdgeByIndex(currentEdgeIdx);
    const float TICK_RATE = 30.0f;

    // Stopped at a controlled intersection — count down, hold at intersection node
    if (stopTimer > 0) {
        --stopTimer;
        speed = 0.0f;
        progress = 1.0; // pin at the arrival node
        // Advance to next edge only once the stop is over
        if (stopTimer == 0) {
            if (routeIdx == (int)route.size() - 2) {
                const Node& finalNode = graph.getNode(route.back());
                x = finalNode.x;
                y = finalNode.y;
                return true;
            }
            progress = 0.0;
            routeIdx++;
            currentEdgeIdx = graph.findEdgeIdx(route[routeIdx], route[routeIdx + 1]);
        }
        // Position is computed below using current edge + progress=1.0 while waiting
    } else {
        progress += (double)(speed / currentEdge.length) * (1.0 / TICK_RATE) / 3600.0;

        if (progress >= 1.0) {
            // Reached the end of the last edge — signal reroute needed
            if (routeIdx == (int)route.size() - 2) {
                const Node& finalNode = graph.getNode(route.back());
                x = finalNode.x;
                y = finalNode.y;
                return true;
            }

            // Set stop timer — edge advance happens when timer expires
            const Node& arrivedAt = graph.getNode(route[routeIdx + 1]);
            if (arrivedAt.control == ControlType::TrafficLight) {
                stopTimer = 90;
                progress = 1.0;
            } else if (arrivedAt.control == ControlType::StopSign) {
                stopTimer = 45;
                progress = 1.0;
            } else {
                // Uncontrolled — advance immediately
                progress = 0.0;
                routeIdx++;
                currentEdgeIdx = graph.findEdgeIdx(route[routeIdx], route[routeIdx + 1]);
            }
        }
    }

    const Node& currentNode = graph.getNode(route[routeIdx]);
    const Node& nextNode    = graph.getNode(route[routeIdx + 1]);

    x = currentNode.x + (nextNode.x - currentNode.x) * progress;
    y = currentNode.y + (nextNode.y - currentNode.y) * progress;

    return false;
}


bool Car::canSee(const Car& other, const Node& currentNode, const Node& nextNode) const {
    // Gate 1 — distance: is the other car within vision range?
    float dist = haversineDistance((float)x, (float)y, (float)other.x, (float)other.y);
    if (dist >= visionRange) return false;

    // Longitude correction factor — fixes degree-space distortion at this latitude
    double cosLat = cos(x * M_PI / 180.0);

    // Facing vector: direction this car is travelling, corrected and normalized
    double fdx = nextNode.x - currentNode.x;
    double fdy = (nextNode.y - currentNode.y) * cosLat;
    double flen = sqrt(fdx * fdx + fdy * fdy);
    if (flen == 0.0) return false;
    fdx /= flen;
    fdy /= flen;

    // To-other vector: direction from this car to the other, corrected and normalized
    double odx = other.x - x;
    double ody = (other.y - y) * cosLat;
    double olen = sqrt(odx * odx + ody * ody);
    if (olen == 0.0) return false;
    odx /= olen;
    ody /= olen;

    // Gate 2 — angle: dot product gives cos(θ), compare against cos(visionAngle)
    double dot = fdx * odx + fdy * ody;
    return dot >= cos((double)visionAngle);
}

V2VMessage Car::broadcast(const RoadGraph& graph) const {
    const Node& currentNode = graph.getNode(route[routeIdx]);
    const Node& nextNode    = graph.getNode(route[routeIdx + 1]);

    double cosLat = cos(x * M_PI / 180.0);
    double dx = nextNode.x - currentNode.x;
    double dy = (nextNode.y - currentNode.y) * cosLat;
    float heading = (float)atan2(dy, dx);

    return V2VMessage{id, (float)x, (float)y, speed, hasHazard, heading, currentEdgeIdx};
}

void Car::applyV2VDriving(const RoadGraph& graph) {
    if (status != CarStatus::Active) return;
    if (routeIdx >= (int)route.size() - 1) return;

    hasHazard = false; // recomputed every tick from current messages

    constexpr float MIN_SPEED = 10.0f;
    const float     MAX_SPEED = graph.getEdgeByIndex(currentEdgeIdx).speedLimit;

    const Node& currentNode = graph.getNode(route[routeIdx]);
    const Node& nextNode    = graph.getNode(route[routeIdx + 1]);

    double cosLat    = cos(x * M_PI / 180.0);
    double dx        = nextNode.x - currentNode.x;
    double dy        = (nextNode.y - currentNode.y) * cosLat;
    float myHeading  = (float)atan2(dy, dx);

    // Determine relevant edge indices — current edge and next edge if it exists
    int nextEdgeIdx = -1;
    if (routeIdx + 2 < (int)route.size()) {
        nextEdgeIdx = graph.findEdgeIdx(route[routeIdx + 1], route[routeIdx + 2]);
    }

    constexpr float MAX_REACT_DIST_KM = 0.200f; // 200m

    bool  slowing     = false;
    float targetSpeed = speed;

    // Normalised facing vector for ahead check
    double flen = sqrt(dx * dx + dy * dy);
    double fdx  = (flen > 0) ? dx / flen : 0.0;
    double fdy  = (flen > 0) ? dy / flen : 0.0;

    for (auto& [senderId, msg] : receivedMessages) {
        if (msg.hasHazard) {
            hasHazard = true;
            slowing = true;
            targetSpeed = std::min(targetSpeed, MIN_SPEED + 5.0f);
            continue;
        }

        // Only react to cars on current edge or next edge ahead
        bool relevantEdge = (msg.currentEdgeIdx == currentEdgeIdx) ||
                            (nextEdgeIdx != -1 && msg.currentEdgeIdx == nextEdgeIdx);
        if (!relevantEdge) continue;

        // Distance filter — ignore cars beyond 200m
        float dist = haversineDistance((float)x, (float)y, msg.x, msg.y);

        constexpr float EARLY_WARN_DIST_KM = 0.400f;
        if (dist > EARLY_WARN_DIST_KM) continue;

        if (msg.hasHazard && dist > MAX_REACT_DIST_KM) {
            // In early warning zone — gentle nudge only
            slowing = true;
            targetSpeed = std::min(targetSpeed, speed * 0.85f);
            continue;
        }

        if (dist > MAX_REACT_DIST_KM) continue;

        // Ahead check — dot product of facing vector and vector to sender
        double toX  = (msg.x - x);
        double toY  = (msg.y - y) * cosLat;
        double dot  = fdx * toX + fdy * toY;
        if (dot <= 0.0) continue; // sender is behind us

        // Hazard relay — propagate if sender is already flagged, not inferred from speed
        if (msg.hasHazard) {
            hasHazard = true;
        }

        // Safe following distance — brake proportionally if too close regardless of peer speed
        constexpr float MIN_FOLLOW_DIST_KM = 0.040f; // 40m
        if (dist < MIN_FOLLOW_DIST_KM) {
            slowing     = true;
            float gapSpeed = speed * (dist / MIN_FOLLOW_DIST_KM);
            targetSpeed = std::min(targetSpeed, gapSpeed);
            continue;
        }

        // Same direction and slower — gradual match to slower car ahead
        float headingDiff = fabsf(msg.heading - myHeading);
        if (headingDiff < (float)(M_PI / 2.0) && msg.speed < speed) {
            slowing     = true;
            targetSpeed = std::min(targetSpeed, msg.speed);
        }
    }

    if (slowing) {
        // Gradual nudge toward the slowest car ahead — predictive, not reactive
        speed += (targetSpeed - speed) * 0.1f;
        speed  = std::max(speed, MIN_SPEED);
    } else {
        // Road is clear — recover at same rate as human driving
        speed = std::min(speed + 3.0f, MAX_SPEED);
    }
}
