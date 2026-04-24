#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class ControlType { TrafficLight, StopSign, Uncontrolled };

struct IntersectionState {
    int phase;       // 0 = N-S green, 1 = E-W green
    int phaseTimer;  // ticks remaining in current phase
    // phaseFromNodes[p] = list of fromNodeIds whose approach direction is green in phase p
    std::vector<std::vector<std::string>> phaseFromNodes;
};

struct Node {
    std::string id;
    std::string name;
    double x;
    double y;
    ControlType control;
};

struct Edge {
    std::string id;
    std::string roadName;
    float length;
    std::string from;
    std::string to;
    float congestion;
    bool closed;
    float speedLimit;
    bool oneWay;
};

// Free function — haversine distance between raw lat/lng coords, returns km
float haversineDistance(float lat1, float lng1, float lat2, float lng2);

class RoadGraph {
    public:
        void addNode(const Node& n);
        void addEdge(const Edge& e);
        void buildHardcodedMap();

        const Node& getNode(const std::string& id) const { return nodes.at(id); }
        const Edge& getEdgeByIndex(int idx) const { return edges[idx]; }
        int findEdgeIdx(const std::string& from, const std::string& to) const;
        std::vector<std::string> getNodeIds() const;

        std::vector<std::string> dijkstra(const std::string& start, const std::string& end,
                                          const std::unordered_set<int>& excludedEdges = {});
        float haversineDistance(const Node& a, const Node& b);

        void initIntersections();
        void tickIntersections();
        bool isGreenFor(const std::string& nodeId, const std::string& fromNodeId) const;

    private:
        std::unordered_map<std::string, Node> nodes;
        std::vector<Edge> edges;
        std::unordered_map<std::string, std::vector<int>> adj;
        std::unordered_map<std::string, IntersectionState> intersections;
};
