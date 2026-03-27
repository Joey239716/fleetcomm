#include <iostream>
#include <zmq.hpp>
#include "graph.hpp"

int main() {
    std::cout << "Fleetcomm starting..." << std::endl;
    std::cout << "ZeroMQ version: "
              << ZMQ_VERSION_MAJOR << "."
              << ZMQ_VERSION_MINOR << "."
              << ZMQ_VERSION_PATCH << std::endl;
        
    RoadGraph graph;
    Node n1 = {"N1", "Market & 4th",  37.7864f, -122.4042f};
    Node n2 = {"N2", "Market & 5th",  37.7836f, -122.4071f};
    Node n3 = {"N3", "Market & 7th",  37.7815f, -122.4110f};
    Node n4 = {"N4", "Mission & 4th", 37.7848f, -122.4038f};

    graph.addNode(n1);
    graph.addNode(n2);
    graph.addNode(n3);
    graph.addNode(n4);

    Edge e1 = {"E1", "Market St", 0.35f, "N1", "N2", 0.0f, false};
    Edge e2 = {"E2", "Market St", 0.42f, "N2", "N3", 0.0f, false};
    Edge e3 = {"E3", "4th St",    0.18f, "N1", "N4", 0.0f, false};

    graph.addEdge(e1);
    graph.addEdge(e2);
    graph.addEdge(e3);

    std::vector<std::string> path = graph.dijkstra("N1", "N3");

    for (const auto& node : path) {
        std::cout << node << std::endl;
    }


    return 0;
}