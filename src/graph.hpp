#pragma once
#include <string>
#include <unordered_map>
#include <vector>

//Our node needs an ID/location
struct Node {
    std::string id;
    std::string name;
    float x;
    float y;
};

struct Edge {
    std::string id;
    std::string roadName;
    float length;
    std::string from;
    std::string to;
    float congestion;
    bool closed;
};

class RoadGraph { 
    public:
        void addNode(const Node& n);
        void addEdge(const Edge& e);
        void buildHardcodedMap();

        std::vector<std::string> dijkstra(const std::string& start, const std::string& end);
    
    private:
        std::unordered_map<std::string, Node> nodes;
        std::vector<Edge> edges;
        std::unordered_map<std::string, std::vector<int>> adj;
};
