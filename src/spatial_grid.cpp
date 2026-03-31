#include "spatial_grid.hpp"
#include <algorithm>
#include <cmath>

SpatialGrid::SpatialGrid(double minLat, double maxLat, double minLng, double maxLng, int rows, int cols)
    : minLat(minLat), maxLat(maxLat), minLng(minLng), maxLng(maxLng), rows(rows), cols(cols) {}

void SpatialGrid::clear() {
    cells.clear();
}

void SpatialGrid::insert(Car* car) {
    cells[getCellIndex(car->x, car->y)].push_back(car);

}

std::vector<Car*> SpatialGrid::getNearby(double x, double y) const {

    std::vector<Car*> res;

    double fracRow = (x - minLat) / (maxLat - minLat);
    double fracCol = (y - minLng) / (maxLng - minLng);

    int centerRow = std::clamp(static_cast<int>(fracRow * rows), 0, rows - 1);
    int centerCol = std::clamp(static_cast<int>(fracCol * cols), 0, cols - 1);

    //neighbouring cells
    const std::vector<std::pair<int, int>> directions = {
            {-1, -1}, {-1, 0}, {-1, 1},
            { 0, -1}, { 0, 0}, { 0, 1},
            { 1, -1}, { 1, 0}, { 1, 1}
    };

    for (const auto& direction : directions) {
        int neighbourRow = centerRow + direction.first;
        int neighbourCol = centerCol + direction.second;

        if (neighbourRow < 0 || neighbourRow >= rows) continue;
        if (neighbourCol < 0 || neighbourCol >= cols) continue;

        int cellIndex = neighbourRow * cols + neighbourCol;

        if (cells.count(cellIndex)) {
            for (auto* car : cells.at(cellIndex)) {
                res.push_back(car);
            }
        }

    }

    return res;
}

int SpatialGrid::getCellIndex(double x, double y) const {
    double fracRow = (x - minLat) / (maxLat - minLat);
    double fracCol = (y - minLng) / (maxLng - minLng);

    int row = std::clamp(static_cast<int>(fracRow * rows), 0, rows - 1);
    int col = std::clamp(static_cast<int>(fracCol * cols), 0, cols - 1);

    return row * cols + col;
}
