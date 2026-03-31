#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include "v2v_message.hpp"

class RoadGraph;
struct Node;

enum class CarStatus { Active, Dead, Breakdown };

struct CarModel {
    std::string name;
    float length;
    float width;
};

class Car {
    public:
        int id;
        CarModel model;
        double progress;
        bool hasHazard;
        std::vector<std::string> route;
        int routeIdx;
        double x;
        double y;
        float speed;
        int laneOffset;
        int currentEdgeIdx;
        CarStatus status;
        std::unordered_map<int, Car*> knownPeers;
        float visionRange;
        float visionAngle;
        std::unordered_map<int, V2VMessage> receivedMessages;
        int repairTimer;
        int stuckTimer;
        int stopTimer;

        Car(int id, CarModel model, std::vector<std::string> route, float speed = 50.0f, int currentEdgeIdx = 0)
            : id(id), model(model), route(route), speed(speed), currentEdgeIdx(currentEdgeIdx),
            routeIdx(0), progress(0.0), hasHazard(false), laneOffset(1), status(CarStatus::Active),
            visionRange(0.06f),         // 60m — tight city driving, cars only see 2-3 car lengths ahead
            visionAngle(M_PI / 4.0f),   // 45° half-angle — 90° total cone, simulates focused forward gaze
            repairTimer(0), stuckTimer(0), stopTimer(0)
            {}

    bool move(const RoadGraph& graph);
    void applyHumanDriving(std::mt19937& rng, const RoadGraph& graph);

    bool canSee(const Car& other, const Node& currentNode, const Node& nextNode) const;

    V2VMessage broadcast(const RoadGraph& graph) const;
    void applyV2VDriving(const RoadGraph& graph);

    void receive();

    void reroute();

    void setStatus();

};