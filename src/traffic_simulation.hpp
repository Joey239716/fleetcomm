#pragma once
#include "graph.hpp"
#include "car.hpp"
#include "spatial_grid.hpp"
#include <vector>
#include <string>
#include <random>
#include <unordered_set>

struct CarFrame {
    int id;
    double lat, lng;
    float speed, heading;
    CarStatus status;
    int currentEdgeIdx;
};
struct SimFrame {
    int tick;
    std::vector<CarFrame> cars;
};

class TrafficSimulation {
public:
    TrafficSimulation();

    bool v2vEnabled = false;

    void spawnCar(int id, const CarModel& model, const std::string& fromNodeId, const std::string& toNodeId);
    void triggerBreakdown(int carId);
    void snapshotFrame();
    SimFrame latestFrame() const;
    const std::vector<SimFrame>& getFrames() const { return frames; }
    virtual void tick();
    void printState() const;
    void printMetrics() const;
    float getEdgeSpeedLimit(int edgeIdx) const;


protected:
    void rerouteCarToRandom(Car& car);
    void rerouteCarFromNode(Car& car, const std::string& fromNode, const std::unordered_set<int>& excludedEdges);
    void tickRepairs();
    void detectStuckCars();
    void detectShockwaves();
    void computeMetrics();

    RoadGraph graph;
    std::vector<Car> cars;
    std::mt19937 rng;

    SpatialGrid spatialGrid;

    int  tickCount      = 0;
    int  shockwaveCount = 0;
    std::unordered_set<int> prevShockwaveEdges;
    std::vector<SimFrame> frames;

    float averageFleetSpeed = 0.0f;
    float stopRate          = 0.0f;
    float speedVariance     = 0.0f;
};
