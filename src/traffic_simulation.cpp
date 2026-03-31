#include "traffic_simulation.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>

TrafficSimulation::TrafficSimulation()
    : rng(std::random_device{}()),
      spatialGrid(37.786, 37.798, -122.407, -122.396, 4, 4) {
    graph.buildHardcodedMap();
}

void TrafficSimulation::spawnCar(int id, const CarModel& model, const std::string& fromNodeId, const std::string& toNodeId) {
    std::vector<std::string> route = graph.dijkstra(fromNodeId, toNodeId);
    if (route.size() < 2) {
        throw std::runtime_error("No valid route found from " + fromNodeId + " to " + toNodeId);
    }

    Car car(id, model, route);
    car.currentEdgeIdx = graph.findEdgeIdx(route[0], route[1]);
    car.laneOffset = 1; // always offset right of travel direction — direction vector handles separation

    const Node& startNode = graph.getNode(route[0]);
    car.x = startNode.x;
    car.y = startNode.y;

    cars.push_back(car);
}

void TrafficSimulation::tick() {
    ++tickCount;

    tickRepairs();

    // Rebuild spatial grid
    spatialGrid.clear();
    for (auto& car : cars) {
        if (car.status == CarStatus::Active || car.status == CarStatus::Breakdown) {
            spatialGrid.insert(&car);
        }
    }

    if (v2vEnabled) {
        // V2V: collect broadcasts and distribute to nearby cars
        std::unordered_map<int, V2VMessage> broadcasts;
        for (auto& car : cars) {
            if (car.status != CarStatus::Active && car.status != CarStatus::Breakdown) continue;
            if (car.routeIdx >= (int)car.route.size() - 1) continue;
            broadcasts[car.id] = car.broadcast(graph);
        }
        for (auto& car : cars) {
            car.receivedMessages.clear();
            for (Car* nearby : spatialGrid.getNearby(car.x, car.y)) {
                if (nearby->id != car.id && broadcasts.count(nearby->id)) {
                    car.receivedMessages[nearby->id] = broadcasts[nearby->id];
                }
            }
        }
        for (auto& car : cars) {
            if (car.status == CarStatus::Active) {
                car.applyV2VDriving(graph);
            }
        }
    } else {
        // Human: populate knownPeers via vision cone
        for (auto& car : cars) {
            car.knownPeers.clear();
            for (Car* other : spatialGrid.getNearby(car.x, car.y)) {
                if (other->id != car.id &&
                    (other->status == CarStatus::Active || other->status == CarStatus::Breakdown)) {
                    car.knownPeers[other->id] = other;
                }
            }
        }
        for (auto& car : cars) {
            car.applyHumanDriving(rng, graph);
        }
    }

    // Move all cars; reroute when destination reached
    for (auto& car : cars) {
        if (car.move(graph)) {
            rerouteCarToRandom(car);
        }
    }

    detectStuckCars();
    detectShockwaves();

    if (tickCount % 30 == 0) {
        computeMetrics();
    }
}

void TrafficSimulation::snapshotFrame() {
    SimFrame f;
    f.tick = tickCount;
    for (const auto& car : cars) {
        if (car.status == CarStatus::Dead) continue;
        float heading = 0.0f;
        if (car.routeIdx < (int)car.route.size() - 1) {
            const Node& curr = graph.getNode(car.route[car.routeIdx]);
            const Node& next = graph.getNode(car.route[car.routeIdx + 1]);
            float cosLat = (float)cos(car.x * M_PI / 180.0);
            float dx = next.x - curr.x;
            float dy = (next.y - curr.y) * cosLat;
            heading = atan2f(dy, dx) * 180.0f / static_cast<float>(M_PI);
        }
        f.cars.push_back({car.id, car.x, car.y, car.speed, heading, car.status, car.currentEdgeIdx});
    }
    frames.push_back(std::move(f));
}

SimFrame TrafficSimulation::latestFrame() const {
    SimFrame f;
    f.tick = tickCount;
    for (const auto& car : cars) {
        if (car.status == CarStatus::Dead) continue;
        float heading = 0.0f;
        if (car.routeIdx < (int)car.route.size() - 1) {
            const Node& curr = graph.getNode(car.route[car.routeIdx]);
            const Node& next = graph.getNode(car.route[car.routeIdx + 1]);
            float cosLat = (float)cos(car.x * M_PI / 180.0);
            float dx = next.x - curr.x;
            float dy = (next.y - curr.y) * cosLat;
            heading = atan2f(dy, dx) * 180.0f / static_cast<float>(M_PI);
        }
        f.cars.push_back({car.id, car.x, car.y, car.speed, heading, car.status, car.currentEdgeIdx});

    }
    return f;
}

void TrafficSimulation::triggerBreakdown(int carId) {
    for (auto& car : cars) {
        if (car.id != carId || car.status != CarStatus::Active) continue;
        car.status      = CarStatus::Breakdown;
        car.speed       = 0.0f;
        car.repairTimer = 600; // ~20 seconds at 30 ticks/sec
        const Edge& edge = graph.getEdgeByIndex(car.currentEdgeIdx);
        std::cout << "[BREAKDOWN] Car " << car.id
                  << " on \"" << edge.roadName << "\""
                  << " (tick " << tickCount << ")\n";
        return;
    }
}

void TrafficSimulation::tickRepairs() {
    for (auto& car : cars) {
        if (car.status != CarStatus::Breakdown) continue;
        if (--car.repairTimer <= 0) {
            car.status = CarStatus::Dead;
            std::cout << "[CLEARED]   Car " << car.id
                      << " removed (tick " << tickCount << ")\n";
        }
    }
}

void TrafficSimulation::detectStuckCars() {
    for (auto& car : cars) {
        if (car.status != CarStatus::Active) {
            car.stuckTimer = 0;
            continue;
        }
        if (car.stopTimer > 0) { // intentionally stopped at intersection
            car.stuckTimer = 0;
            continue;
        }
        if (car.speed < 5.0f) {
            ++car.stuckTimer;
        } else {
            car.stuckTimer = 0;
        }
        if (car.stuckTimer >= 90) {
            // Snap back to the node we came from and find a detour
            const Node& snap = graph.getNode(car.route[car.routeIdx]);
            car.x        = snap.x;
            car.y        = snap.y;
            car.progress = 0.0f;
            car.stuckTimer = 0;
            rerouteCarFromNode(car, car.route[car.routeIdx], {car.currentEdgeIdx});
        }
    }
}

void TrafficSimulation::rerouteCarFromNode(Car& car, const std::string& fromNode,
                                            const std::unordered_set<int>& excludedEdges) {
    const std::vector<std::string> nodeIds = graph.getNodeIds();
    std::uniform_int_distribution<size_t> dist(0, nodeIds.size() - 1);
    for (int attempt = 0; attempt < 10; ++attempt) {
        std::string toNode = nodeIds[dist(rng)];
        if (toNode == fromNode) continue;
        std::vector<std::string> newRoute = graph.dijkstra(fromNode, toNode, excludedEdges);
        if (newRoute.size() >= 2) {
            car.route          = std::move(newRoute);
            car.routeIdx       = 0;
            car.progress       = 0.0f;
            car.currentEdgeIdx = graph.findEdgeIdx(car.route[0], car.route[1]);
            return;
        }
    }
    // No detour found — car stays put until road clears
}

void TrafficSimulation::rerouteCarToRandom(Car& car) {
    const std::string fromNode = car.route.back();
    const std::vector<std::string> nodeIds = graph.getNodeIds();

    std::uniform_int_distribution<size_t> dist(0, nodeIds.size() - 1);
    std::string toNode;
    do {
        toNode = nodeIds[dist(rng)];
    } while (toNode == fromNode);

    std::vector<std::string> newRoute = graph.dijkstra(fromNode, toNode);
    if (newRoute.size() < 2) return;

    car.route = std::move(newRoute);
    car.routeIdx = 0;
    car.progress = 0.0f;
    car.currentEdgeIdx = graph.findEdgeIdx(car.route[0], car.route[1]);
}

void TrafficSimulation::detectShockwaves() {
    constexpr float SLOW_THRESHOLD = 15.0f;
    constexpr int   MIN_CARS       = 3;

    std::unordered_map<int, std::vector<Car*>> byEdge;
    for (auto& car : cars) {
        if (car.status == CarStatus::Active) {
            byEdge[car.currentEdgeIdx].push_back(&car);
        }
    }

    std::unordered_set<int> currentShockwaveEdges;

    for (auto& [edgeIdx, edgeCars] : byEdge) {
        if (static_cast<int>(edgeCars.size()) < MIN_CARS) continue;

        std::sort(edgeCars.begin(), edgeCars.end(),
                  [](const Car* a, const Car* b) { return a->progress < b->progress; });

        int consecutive = 0;
        int maxRun      = 0;
        for (const Car* car : edgeCars) {
            if (car->speed < SLOW_THRESHOLD) {
                maxRun = std::max(maxRun, ++consecutive);
            } else {
                consecutive = 0;
            }
        }

        if (maxRun >= MIN_CARS) {
            currentShockwaveEdges.insert(edgeIdx);
            if (prevShockwaveEdges.find(edgeIdx) == prevShockwaveEdges.end()) {
                ++shockwaveCount;
                const Edge& edge = graph.getEdgeByIndex(edgeIdx);
                std::cout << "[SHOCKWAVE] \"" << edge.roadName
                          << "\" — " << maxRun << " cars slowed"
                          << " (tick " << tickCount << ")\n";
            }
        }
    }

    prevShockwaveEdges = std::move(currentShockwaveEdges);
}

void TrafficSimulation::computeMetrics() {
    int   activeCount = 0;
    float speedSum    = 0.0f;
    int   stopCount   = 0;

    for (const auto& car : cars) {
        if (car.status != CarStatus::Active) continue;
        ++activeCount;
        speedSum += car.speed;
        if (car.speed < 5.0f) ++stopCount;
    }

    if (activeCount == 0) return;

    averageFleetSpeed = speedSum / static_cast<float>(activeCount);
    stopRate          = static_cast<float>(stopCount) / static_cast<float>(activeCount);

    float varianceSum = 0.0f;
    for (const auto& car : cars) {
        if (car.status != CarStatus::Active) continue;
        float diff = car.speed - averageFleetSpeed;
        varianceSum += diff * diff;
    }
    speedVariance = varianceSum / static_cast<float>(activeCount);
}

void TrafficSimulation::printState() const {
    for (const auto& car : cars) {
        std::string statusStr;
        switch (car.status) {
            case CarStatus::Active:    statusStr = "Active";    break;
            case CarStatus::Dead:      statusStr = "Dead";      break;
            case CarStatus::Breakdown: statusStr = "Breakdown"; break;
        }
        std::cout << "Car " << car.id << " [" << car.model.name << "]"
                  << "  pos=(" << car.x << ", " << car.y << ")"
                  << "  status=" << statusStr << "\n";
    }
}

void TrafficSimulation::printMetrics() const {
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "\n=== Fleet Metrics (V2V: " << (v2vEnabled ? "ON" : "OFF") << ") ===\n";
    std::cout << "Average speed:   " << averageFleetSpeed << " km/h\n";
    std::cout << "Stop rate:       " << (stopRate * 100.0f) << "%\n";
    std::cout << "Speed variance:  " << speedVariance << "\n";
    std::cout << "Shockwaves:      " << shockwaveCount << "\n";
}

float TrafficSimulation::getEdgeSpeedLimit(int edgeIdx) const {
    return graph.getEdgeByIndex(edgeIdx).speedLimit;
}