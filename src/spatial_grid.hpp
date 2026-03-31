#pragma once
#include "car.hpp"
#include <vector>
#include <unordered_map>

class SpatialGrid {
public:
    SpatialGrid(double minLat, double maxLat, double minLng, double maxLng, int rows, int cols);

    void clear();
    void insert(Car* car);
    std::vector<Car*> getNearby(double x, double y) const;

private:
    int getCellIndex(double x, double y) const;

    double minLat, maxLat;
    double minLng, maxLng;
    int rows, cols;

    std::unordered_map<int, std::vector<Car*>> cells;
};
