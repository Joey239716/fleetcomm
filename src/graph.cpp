#include "graph.hpp"
#include <queue>
#include <limits>
#include <algorithm>
#include <unordered_map>



void RoadGraph::addNode(const Node& n) {
    nodes[n.id] = n;
}

void RoadGraph::addEdge(const Edge& e) {
    edges.push_back(e);
    adj[e.from].push_back(edges.size() - 1);
    adj[e.to].push_back(edges.size() - 1);
}

std::vector<std::string> RoadGraph::dijkstra(const std::string& start, const std::string& end) {
    using P = std::pair<float, std::string>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;

    std::unordered_map<std::string, float> dist;

    std::unordered_map<std::string, std::string> prev;

    for (auto& [id, node] : nodes) {
        dist[id] = std::numeric_limits<float>::infinity();
    }

    dist[start] = 0;

    pq.push({0, start});

    while (pq.size() > 0) {
        auto [curDist, curNode] = pq.top(); //In CPP, peak and then pop, where as in python, pop does both
        pq.pop();

        //Take our node, find all the edges it connects to and add those to our priority queue with their length away from the starting node
        if (curDist > dist[curNode]) { 
            continue;
        }

        if (curNode == end) {
            break;
        }

        for (int idx : adj[curNode]) {
            const Edge& e = edges[idx]; //edge
            
            std::string neighbour = (e.from == curNode) ? e.to : e.from;
            
            float newDist = dist[curNode] + e.length;

            if (newDist < dist[neighbour]){
                dist[neighbour] = newDist;
                prev[neighbour] = curNode;
                pq.push({newDist, neighbour});
            }
        }
    }

    std::vector<std::string> path;

    std::string node = end;

    while(node != start) {
        path.push_back(node);
        node = prev[node];
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());

    return path;

}