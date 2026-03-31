#pragma once
#include "traffic_simulation.hpp"

class V2VSimulation: public TrafficSimulation {
    public:
        void tick() override;
};