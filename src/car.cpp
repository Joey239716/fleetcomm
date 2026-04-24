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
        if (!canSee(*peer, currentNode, nextNode, graph)) continue;
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

    // Stopped at a controlled intersection — hold until clear
    if (stopTimer > 0) {
        speed = 0.0f;
        progress = 1.0; // pin at the arrival node

        // For traffic lights: hold without decrementing while red; decrement when green
        if (routeIdx + 1 < (int)route.size()) {
            const Node& waitingAt = graph.getNode(route[routeIdx + 1]);
            if (waitingAt.control == ControlType::TrafficLight) {
                bool green = graph.isGreenFor(route[routeIdx + 1], route[routeIdx]);
                if (!green) {
                    // Stay stopped — don't decrement
                    // Position is computed below using current edge + progress=1.0
                    goto position_update;
                }
                // Just turned green — give a short clearance window if we were holding
                if (stopTimer > 5) stopTimer = 5;
            }
        }

        --stopTimer;
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
                // stopTimer = 1 means "waiting at red"; cleared to 5 when green detected
                stopTimer = 1;
                progress = 1.0;
            } else if (arrivedAt.control == ControlType::StopSign) {
                stopTimer = 15;
                progress = 1.0;
            } else {
                // Uncontrolled — advance immediately
                progress = 0.0;
                routeIdx++;
                currentEdgeIdx = graph.findEdgeIdx(route[routeIdx], route[routeIdx + 1]);
            }
        }
    }
    position_update:

    const Node& currentNode = graph.getNode(route[routeIdx]);
    const Node& nextNode    = graph.getNode(route[routeIdx + 1]);

    x = currentNode.x + (nextNode.x - currentNode.x) * progress;
    y = currentNode.y + (nextNode.y - currentNode.y) * progress;

    return false;
}


bool Car::canSee(const Car& other, const Node& currentNode, const Node& nextNode, const RoadGraph& graph) const {
 
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

    // Gate 3 - heading: ignore oncoming cars in the opposite lane
    // Use the last valid segment index so this fires even when the car is on its final hop
    {
        const int checkIdx = std::min(other.routeIdx, (int)other.route.size() - 2);
        if (checkIdx >= 0) {
            const Node& oFrom = graph.getNode(other.route[checkIdx]);
            const Node& oTo   = graph.getNode(other.route[checkIdx + 1]);
            double ofdx = oTo.x - oFrom.x;
            double ofdy = (oTo.y - oFrom.y) * cosLat;
            double oflen = sqrt(ofdx * ofdx + ofdy * ofdy);
            if (oflen > 0.0 && (fdx * (ofdx/oflen) + fdy * (ofdy/oflen)) < 0.0) return false;
        }
    }


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

    hasHazard = false;

    constexpr float MIN_SPEED        = 10.0f;
    constexpr float HARD_BRAKE_DIST  = 0.025f; // 25m — emergency stop zone
    constexpr float FOLLOW_DIST      = 0.050f; // 50m — tight platoon following
    constexpr float REACT_DIST       = 0.100f; // 100m — only react within this range
    constexpr float SPEED_DIFF_BRAKE = 15.0f;  // only slow if peer is this much slower

    const float MAX_SPEED = graph.getEdgeByIndex(currentEdgeIdx).speedLimit;

    const Node& currentNode = graph.getNode(route[routeIdx]);
    const Node& nextNode    = graph.getNode(route[routeIdx + 1]);

    double cosLat   = cos(x * M_PI / 180.0);
    double dx       = nextNode.x - currentNode.x;
    double dy       = (nextNode.y - currentNode.y) * cosLat;
    float myHeading = (float)atan2(dy, dx);

    double flen = sqrt(dx * dx + dy * dy);
    double fdx  = (flen > 0) ? dx / flen : 0.0;
    double fdy  = (flen > 0) ? dy / flen : 0.0;

    bool  slowing     = false;
    float targetSpeed = MAX_SPEED; // assume full speed unless blocked

    for (auto& [senderId, msg] : receivedMessages) {
        // Only care about cars on the same edge
        if (msg.currentEdgeIdx != currentEdgeIdx) continue;

        // Skip oncoming traffic — heading difference > 120° means opposing direction
        float hd = fabsf(msg.heading - myHeading);
        if (hd > (float)M_PI) hd = 2.0f * (float)M_PI - hd;
        if (hd > (float)(M_PI * 2.0 / 3.0)) continue;

        float dist = haversineDistance((float)x, (float)y, msg.x, msg.y);
        if (dist > REACT_DIST) continue;

        // Ahead check
        double toX = (msg.x - x);
        double toY = (msg.y - y) * cosLat;
        if (fdx * toX + fdy * toY <= 0.0) continue; // behind us

        if (msg.hasHazard) {
            hasHazard = true;
            slowing = true;
            targetSpeed = std::min(targetSpeed, MIN_SPEED);
            continue;
        }

        // Emergency: car is dangerously close — hard brake
        if (dist < HARD_BRAKE_DIST) {
            slowing = true;
            targetSpeed = std::min(targetSpeed, MIN_SPEED);
            continue;
        }

        // Tight following zone — match speed precisely
        if (dist < FOLLOW_DIST) {
            slowing = true;
            targetSpeed = std::min(targetSpeed, msg.speed);
            continue;
        }

        // Beyond follow zone — only brake if peer is significantly slower
        float headingDiff = fabsf(msg.heading - myHeading);
        if (headingDiff < (float)(M_PI / 2.0) && (speed - msg.speed) > SPEED_DIFF_BRAKE) {
            slowing = true;
            targetSpeed = std::min(targetSpeed, msg.speed + SPEED_DIFF_BRAKE * 0.5f);
        }
    }

    if (slowing) {
        speed += (targetSpeed - speed) * 0.15f;
        speed  = std::max(speed, MIN_SPEED);
    } else {
        // V2V advantage: accelerate faster when road is confirmed clear
        speed = std::min(speed + 6.0f, MAX_SPEED);
    }
}
