#pragma once
#include<iostream>
#include<string>
#include<vector>

enum class CarStatus { Active, Dead, Breakdown };

class Car {
    public:
        int id;
        std::string model;
        int direction;
        float progress;
        bool hasHazard;
        std::vector<std::string> route;
        int routeIdx;
        float x;
        float y;
        float speed;
        int laneOffset;
        CarStatus status;


        Car(int id, std::string model, std::vector<std::string> route, float speed = 0.001f) {
            this->id = id;
            this->model = model;
            this->route = route;
            this->routeIdx = 0;
            this->direction = 1;
            this->progress = 0.0f;
            this->hasHazard = false;
            this->laneOffset = 1;
            this->status = CarStatus::Active;
            this->speed = speed;
        }

    void move();

    void broadcast();

    void receive();

    void reroute();

    void setStatus();

};