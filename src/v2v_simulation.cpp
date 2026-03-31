#include "v2v_simulation.hpp"
#include <unordered_map>

void V2VSimulation::tick() {
    ++tickCount;

    tickRepairs();

    // Rebuild spatial grid — Active and Breakdown cars occupy physical space
    spatialGrid.clear();
    for (auto& car : cars) {
        if (car.status == CarStatus::Active || car.status == CarStatus::Breakdown) {
            spatialGrid.insert(&car);
        }
    }

    // Collect broadcasts — breakdown cars broadcast their stopped position (speed=0 triggers hazard relay)
    std::unordered_map<int, V2VMessage> broadcasts;
    for (auto& car : cars) {
        if (car.status != CarStatus::Active && car.status != CarStatus::Breakdown) continue;
        if (car.routeIdx >= (int)car.route.size() - 1) continue;
        broadcasts[car.id] = car.broadcast(graph);
    }

    // Distribute messages to nearby cars
    for (auto& car : cars) {
        car.receivedMessages.clear();

        for (Car* nearby : spatialGrid.getNearby(car.x, car.y)) {
            if (nearby->id != car.id && broadcasts.count(nearby->id)) {
                car.receivedMessages[nearby->id] = broadcasts[nearby->id];
            }
        }
    }

    // V2V driving behaviour
    for (auto& car : cars) {
        if (car.status == CarStatus::Active) {
            car.applyV2VDriving(graph);
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