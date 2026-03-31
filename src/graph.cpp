#include "graph.hpp"
#include <queue>
#include <limits>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cmath>


float haversineDistance(float lat1, float lng1, float lat2, float lng2) {
    const float R  = 6371.0f;
    const float PI = static_cast<float>(M_PI);
    float dlat = (lat2 - lat1) * PI / 180.0f;
    float dlng = (lng2 - lng1) * PI / 180.0f;
    float a = sinf(dlat / 2) * sinf(dlat / 2)
            + cosf(lat1 * PI / 180.0f) * cosf(lat2 * PI / 180.0f)
            * sinf(dlng / 2) * sinf(dlng / 2);
    return R * 2.0f * atan2f(sqrtf(a), sqrtf(1.0f - a));
}

void RoadGraph::addNode(const Node& n) {
    nodes[n.id] = n;
}

void RoadGraph::addEdge(const Edge& e) {
    edges.push_back(e);
    adj[e.from].push_back(edges.size() - 1);
    if (!e.oneWay) {
        adj[e.to].push_back(edges.size() - 1);
    }
}

std::vector<std::string> RoadGraph::dijkstra(const std::string& start, const std::string& end,
                                              const std::unordered_set<int>& excludedEdges) {
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
            if (excludedEdges.count(idx)) continue;
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

    // No path found — prev won't contain a chain back to start
    if (start != end && prev.find(end) == prev.end()) {
        return {};
    }

    std::vector<std::string> path;

    std::string node = end;

    while(node != start) {
        path.push_back(node);
        if (prev.find(node) == prev.end()) return {}; // safety: broken chain
        node = prev[node];
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());

    return path;

}

float RoadGraph::haversineDistance(const Node& a, const Node& b) {
    const double R = 6371.0;
    const double PI = M_PI;

    double lat1 = a.x * PI / 180.0;
    double lat2 = b.x * PI / 180.0;
    double dlat = (b.x - a.x) * PI / 180.0;
    double dlng = (b.y - a.y) * PI / 180.0;

    double h = sin(dlat/2) * sin(dlat/2) +
               cos(lat1) * cos(lat2) *
               sin(dlng/2) * sin(dlng/2);

    double c = 2 * atan2(sqrt(h), sqrt(1 - h));

    return (float)(R * c);
}

std::vector<std::string> RoadGraph::getNodeIds() const {
    std::vector<std::string> ids;
    ids.reserve(nodes.size());
    for (const auto& [id, node] : nodes) {
        ids.push_back(id);
    }
    return ids;
}

int RoadGraph::findEdgeIdx(const std::string& from, const std::string& to) const {
    for (int idx : adj.at(from)) {
        const Edge& e = edges[idx];
        if ((e.from == from && e.to == to) || (e.from == to && e.to == from)) {
            return idx;
        }
    }
    return -1;
}

void RoadGraph::buildHardcodedMap() {
    addNode({"N1", "Jackson & Kearny", 37.796226f, -122.405143f, ControlType::StopSign});
    addNode({"N2", "Jackson & Montgomery", 37.796424f, -122.403520f, ControlType::StopSign});
    addNode({"N3", "Jackson & Sansome", 37.796630f, -122.401878f, ControlType::StopSign});
    addNode({"N4", "Jackson & Battery", 37.796780f, -122.400702f, ControlType::StopSign});
    addNode({"N5", "Jackson & Front", 37.796940f, -122.399511f, ControlType::StopSign});
    addNode({"N6", "Clay & Kearny", 37.794466f, -122.404791f, ControlType::StopSign});
    addNode({"N7", "Clay & Montgomery", 37.794680f, -122.403154f, ControlType::StopSign});
    addNode({"N8", "Clay & Sansome", 37.794857f, -122.401507f, ControlType::StopSign});
    addNode({"N9", "Clay & Battery", 37.794998f, -122.400335f, ControlType::StopSign});
    addNode({"N10", "Clay & Front", 37.795198f, -122.399181f, ControlType::StopSign});
    addNode({"N11", "Sacramento & Kearny", 37.793578f, -122.404616f, ControlType::StopSign});
    addNode({"N12", "Sacramento & Montgomery", 37.793790f, -122.402972f, ControlType::StopSign});
    addNode({"N13", "Sacramento & Sansome", 37.793995f, -122.401333f, ControlType::StopSign});
    addNode({"N14", "Sacramento & Battery", 37.794143f, -122.400158f, ControlType::StopSign});
    addNode({"N15", "Sacramento & Front", 37.794291f, -122.398966f, ControlType::StopSign});
    addNode({"N16", "California & Kearny", 37.792657f, -122.404423f, ControlType::TrafficLight});
    addNode({"N17", "California & Montgomery", 37.792861f, -122.402786f, ControlType::TrafficLight});
    addNode({"N18", "California & Sansome", 37.793065f, -122.401145f, ControlType::TrafficLight});
    addNode({"N19", "California & Battery", 37.793215f, -122.399973f, ControlType::TrafficLight});
    addNode({"N20", "California & Front", 37.793370f, -122.398798f, ControlType::TrafficLight});
    addNode({"N21", "Pine & Kearny", 37.791700f, -122.404236f, ControlType::StopSign});
    addNode({"N22", "Pine & Montgomery", 37.791911f, -122.402597f, ControlType::StopSign});
    addNode({"N23", "Pine & Sansome", 37.792117f, -122.400958f, ControlType::StopSign});
    addNode({"N24", "Pine & Battery", 37.792266f, -122.399782f, ControlType::StopSign});
    addNode({"N25", "Pine & Front", 37.792413f, -122.398610f, ControlType::StopSign});
    addNode({"N26", "Bush & Kearny", 37.790763f, -122.404054f, ControlType::StopSign});
    addNode({"N27", "Bush & Montgomery", 37.790981f, -122.402409f, ControlType::StopSign});
    addNode({"N28", "Bush & Sansome", 37.791181f, -122.400762f, ControlType::StopSign});
    addNode({"N29", "Bush & Battery", 37.791335f, -122.399605f, ControlType::StopSign});
    addNode({"N31", "Market & Kearny", 37.787687f, -122.403432f, ControlType::TrafficLight});
    addNode({"N32", "Market & Montgomery", 37.788815f, -122.401983f, ControlType::TrafficLight});
    addNode({"N30", "Bush & Market", 37.791057f, -122.399159f, ControlType::TrafficLight});
    addNode({"N33", "Front & Market", 37.792609f, -122.397180f, ControlType::TrafficLight});
    addNode({"N34", "N34", 37.793514f, -122.397630f, ControlType::StopSign});
    addNode({"N35", "N35", 37.790254f, -122.400591f, ControlType::StopSign});
    addNode({"N36", "N36", 37.790044f, -122.402225f, ControlType::StopSign});
    addNode({"N37", "N37", 37.790309f, -122.400106f, ControlType::StopSign});
    addNode({"N38", "N38", 37.792560f, -122.397424f, ControlType::StopSign});
    addNode({"N39", "N39", 37.794449f, -122.397802f, ControlType::StopSign});
    addNode({"N40", "N40", 37.795339f, -122.398006f, ControlType::StopSign});
    addNode({"N41", "N41", 37.796187f, -122.398177f, ControlType::StopSign});
    addNode({"N42", "N42", 37.797093f, -122.398361f, ControlType::StopSign});
    addNode({"N43", "N43", 37.797242f, -122.397201f, ControlType::StopSign});
    addNode({"N44", "N44", 37.796356f, -122.397024f, ControlType::StopSign});
    addNode({"N45", "N45", 37.796632f, -122.395570f, ControlType::StopSign});
    addNode({"N46", "N46", 37.795110f, -122.394412f, ControlType::StopSign});
    addNode({"N47", "N47", 37.794788f, -122.394100f, ControlType::StopSign});
    addNode({"N48", "N48", 37.792609f, -122.391365f, ControlType::StopSign});
    addNode({"N49", "N49", 37.791176f, -122.390646f, ControlType::StopSign});
    addNode({"N50", "N50", 37.789506f, -122.388618f, ControlType::StopSign});
    addNode({"N51", "N51", 37.788844f, -122.389412f, ControlType::StopSign});
    addNode({"N53", "N53", 37.788158f, -122.390324f, ControlType::StopSign});
    addNode({"N54", "N54", 37.795530f, -122.403327f, ControlType::StopSign});
    addNode({"N55", "N55", 37.795330f, -122.404969f, ControlType::StopSign});
    addNode({"N56", "N56", 37.795742f, -122.401696f, ControlType::StopSign});
    addNode({"N57", "N57", 37.795877f, -122.400511f, ControlType::StopSign});
    addNode({"N58", "N58", 37.795479f, -122.396836f, ControlType::StopSign});
    addNode({"N59", "N59", 37.794593f, -122.396654f, ControlType::StopSign});
    addNode({"N60", "N60", 37.793669f, -122.396493f, ControlType::StopSign});
    addNode({"N61", "N61", 37.793291f, -122.396348f, ControlType::StopSign});
    addNode({"N62", "N62", 37.794322f, -122.393339f, ControlType::StopSign});
    addNode({"N63", "N63", 37.793728f, -122.392614f, ControlType::StopSign});
    addNode({"N64", "N64", 37.789370f, -122.391858f, ControlType::StopSign});
    addNode({"N65", "N65", 37.790099f, -122.390957f, ControlType::StopSign});
    addNode({"N66", "N66", 37.791286f, -122.392480f, ControlType::StopSign});
    addNode({"N67", "N67", 37.790625f, -122.393360f, ControlType::StopSign});
    addNode({"N68", "N68", 37.791846f, -122.394969f, ControlType::StopSign});
    addNode({"N69", "N69", 37.792575f, -122.394047f, ControlType::StopSign});
    addNode({"N70", "N70", 37.793258f, -122.393205f, ControlType::StopSign});
    addNode({"N71", "N71", 37.794453f, -122.394819f, ControlType::StopSign});
    addNode({"N72", "N72", 37.793817f, -122.395672f, ControlType::StopSign});
    addNode({"N73", "N73", 37.792015f, -122.391654f, ControlType::StopSign});
    addNode({"N74", "N74", 37.792329f, -122.391230f, ControlType::StopSign});
    addNode({"N75", "N75", 37.790769f, -122.390131f, ControlType::StopSign});
    addNode({"N76", "N76", 37.793122f, -122.396407f, ControlType::StopSign});
    addNode({"N77", "N77", 37.793003f, -122.396273f, ControlType::StopSign});
    addNode({"N78", "N78", 37.794764f, -122.402375f, ControlType::StopSign});
    addNode({"N79", "N79", 37.794326f, -122.402292f, ControlType::StopSign});
    addNode({"N80", "N80", 37.794433f, -122.401421f, ControlType::StopSign});
    addNode({"N81", "N81", 37.794230f, -122.403066f, ControlType::StopSign});
    addNode({"N82", "N82", 37.793887f, -122.402205f, ControlType::StopSign});

    addEdge({"E1", "Jackson St", haversineDistance(nodes["N1"], nodes["N2"]), "N1", "N2", 0.0f, false, 40.0f, false});
    addEdge({"E2", "Jackson St", haversineDistance(nodes["N2"], nodes["N3"]), "N2", "N3", 0.0f, false, 40.0f, false});
    addEdge({"E3", "Jackson St", haversineDistance(nodes["N3"], nodes["N4"]), "N3", "N4", 0.0f, false, 40.0f, false});
    addEdge({"E4", "Jackson St", haversineDistance(nodes["N4"], nodes["N5"]), "N4", "N5", 0.0f, false, 40.0f, false});
    addEdge({"E5", "Clay St", haversineDistance(nodes["N6"], nodes["N7"]), "N6", "N7", 0.0f, false, 40.0f, true});
    addEdge({"E7", "Clay St", haversineDistance(nodes["N8"], nodes["N9"]), "N8", "N9", 0.0f, false, 40.0f, true});
    addEdge({"E8", "Clay St", haversineDistance(nodes["N9"], nodes["N10"]), "N9", "N10", 0.0f, false, 40.0f, true});
    addEdge({"E9", "Sacramento St", haversineDistance(nodes["N15"], nodes["N14"]), "N15", "N14", 0.0f, false, 40.0f, true});
    addEdge({"E10", "Sacramento St", haversineDistance(nodes["N14"], nodes["N13"]), "N14", "N13", 0.0f, false, 40.0f, true});
    addEdge({"E11", "Sacramento St", haversineDistance(nodes["N13"], nodes["N12"]), "N13", "N12", 0.0f, false, 40.0f, true});
    addEdge({"E12", "Sacramento St", haversineDistance(nodes["N12"], nodes["N11"]), "N12", "N11", 0.0f, false, 40.0f, true});
    addEdge({"E13", "California St", haversineDistance(nodes["N16"], nodes["N17"]), "N16", "N17", 0.0f, false, 48.0f, false});
    addEdge({"E14", "California St", haversineDistance(nodes["N17"], nodes["N18"]), "N17", "N18", 0.0f, false, 48.0f, false});
    addEdge({"E15", "California St", haversineDistance(nodes["N18"], nodes["N19"]), "N18", "N19", 0.0f, false, 48.0f, false});
    addEdge({"E16", "California St", haversineDistance(nodes["N19"], nodes["N20"]), "N19", "N20", 0.0f, false, 48.0f, false});
    addEdge({"E17", "Pine St", haversineDistance(nodes["N25"], nodes["N24"]), "N25", "N24", 0.0f, false, 40.0f, true});
    addEdge({"E18", "Pine St", haversineDistance(nodes["N24"], nodes["N23"]), "N24", "N23", 0.0f, false, 40.0f, true});
    addEdge({"E19", "Pine St", haversineDistance(nodes["N23"], nodes["N22"]), "N23", "N22", 0.0f, false, 40.0f, true});
    addEdge({"E20", "Pine St", haversineDistance(nodes["N22"], nodes["N21"]), "N22", "N21", 0.0f, false, 40.0f, true});
    addEdge({"E21", "Bush St", haversineDistance(nodes["N26"], nodes["N27"]), "N26", "N27", 0.0f, false, 40.0f, true});
    addEdge({"E22", "Bush St", haversineDistance(nodes["N27"], nodes["N28"]), "N27", "N28", 0.0f, false, 40.0f, true});
    addEdge({"E23", "Bush St", haversineDistance(nodes["N28"], nodes["N29"]), "N28", "N29", 0.0f, false, 40.0f, true});
    addEdge({"E25", "Market St", haversineDistance(nodes["N31"], nodes["N32"]), "N31", "N32", 0.0f, false, 48.0f, false});
    addEdge({"E54", "Market St", haversineDistance(nodes["N32"], nodes["N30"]), "N32", "N30", 0.0f, false, 48.0f, false});
    addEdge({"E55", "Market St", haversineDistance(nodes["N30"], nodes["N33"]), "N30", "N33", 0.0f, false, 48.0f, false});
    addEdge({"E53", "Battery St", haversineDistance(nodes["N29"], nodes["N30"]), "N29", "N30", 0.0f, false, 40.0f, true});
    addEdge({"E56", "Front St", haversineDistance(nodes["N33"], nodes["N25"]), "N33", "N25", 0.0f, false, 40.0f, true});
    addEdge({"E26", "Kearny St", haversineDistance(nodes["N1"], nodes["N6"]), "N1", "N6", 0.0f, false, 40.0f, true});
    addEdge({"E27", "Kearny St", haversineDistance(nodes["N6"], nodes["N11"]), "N6", "N11", 0.0f, false, 40.0f, true});
    addEdge({"E28", "Kearny St", haversineDistance(nodes["N11"], nodes["N16"]), "N11", "N16", 0.0f, false, 40.0f, true});
    addEdge({"E29", "Kearny St", haversineDistance(nodes["N16"], nodes["N21"]), "N16", "N21", 0.0f, false, 40.0f, true});
    addEdge({"E30", "Kearny St", haversineDistance(nodes["N21"], nodes["N26"]), "N21", "N26", 0.0f, false, 40.0f, true});
    addEdge({"E31", "Kearny St", haversineDistance(nodes["N26"], nodes["N31"]), "N26", "N31", 0.0f, false, 40.0f, true});
    addEdge({"E32", "Montgomery St", haversineDistance(nodes["N2"], nodes["N7"]), "N2", "N7", 0.0f, false, 48.0f, false});
    addEdge({"E33", "Montgomery St", haversineDistance(nodes["N7"], nodes["N12"]), "N7", "N12", 0.0f, false, 48.0f, false});
    addEdge({"E34", "Montgomery St", haversineDistance(nodes["N12"], nodes["N17"]), "N12", "N17", 0.0f, false, 48.0f, false});
    addEdge({"E35", "Montgomery St", haversineDistance(nodes["N17"], nodes["N22"]), "N17", "N22", 0.0f, false, 48.0f, false});
    addEdge({"E36", "Montgomery St", haversineDistance(nodes["N22"], nodes["N27"]), "N22", "N27", 0.0f, false, 48.0f, false});
    addEdge({"E37", "Montgomery St", haversineDistance(nodes["N27"], nodes["N32"]), "N27", "N32", 0.0f, false, 48.0f, false});
    addEdge({"E38", "Sansome St", haversineDistance(nodes["N28"], nodes["N23"]), "N28", "N23", 0.0f, false, 40.0f, true});
    addEdge({"E39", "Sansome St", haversineDistance(nodes["N23"], nodes["N18"]), "N23", "N18", 0.0f, false, 40.0f, true});
    addEdge({"E40", "Sansome St", haversineDistance(nodes["N18"], nodes["N13"]), "N18", "N13", 0.0f, false, 40.0f, true});
    addEdge({"E42", "Sansome St", haversineDistance(nodes["N8"], nodes["N3"]), "N8", "N3", 0.0f, false, 40.0f, true});
    addEdge({"E43", "Battery St", haversineDistance(nodes["N4"], nodes["N9"]), "N4", "N9", 0.0f, false, 40.0f, true});
    addEdge({"E44", "Battery St", haversineDistance(nodes["N9"], nodes["N14"]), "N9", "N14", 0.0f, false, 40.0f, true});
    addEdge({"E45", "Battery St", haversineDistance(nodes["N14"], nodes["N19"]), "N14", "N19", 0.0f, false, 40.0f, true});
    addEdge({"E46", "Battery St", haversineDistance(nodes["N19"], nodes["N24"]), "N19", "N24", 0.0f, false, 40.0f, true});
    addEdge({"E47", "Battery St", haversineDistance(nodes["N24"], nodes["N29"]), "N24", "N29", 0.0f, false, 40.0f, true});
    addEdge({"E49", "Front St", haversineDistance(nodes["N25"], nodes["N20"]), "N25", "N20", 0.0f, false, 40.0f, true});
    addEdge({"E50", "Front St", haversineDistance(nodes["N20"], nodes["N15"]), "N20", "N15", 0.0f, false, 40.0f, true});
    addEdge({"E51", "Front St", haversineDistance(nodes["N15"], nodes["N10"]), "N15", "N10", 0.0f, false, 40.0f, true});
    addEdge({"E1", "Jackson St", haversineDistance(nodes["N1"], nodes["N2"]), "N1", "N2", 0.0f, false, 40.0f, false});
    addEdge({"E2", "Jackson St", haversineDistance(nodes["N2"], nodes["N3"]), "N2", "N3", 0.0f, false, 40.0f, false});
    addEdge({"E3", "Jackson St", haversineDistance(nodes["N3"], nodes["N4"]), "N3", "N4", 0.0f, false, 40.0f, false});
    addEdge({"E4", "Jackson St", haversineDistance(nodes["N4"], nodes["N5"]), "N4", "N5", 0.0f, false, 40.0f, false});
    addEdge({"E5", "Clay St", haversineDistance(nodes["N6"], nodes["N7"]), "N6", "N7", 0.0f, false, 40.0f, true});
    addEdge({"E7", "Clay St", haversineDistance(nodes["N8"], nodes["N9"]), "N8", "N9", 0.0f, false, 40.0f, true});
    addEdge({"E8", "Clay St", haversineDistance(nodes["N9"], nodes["N10"]), "N9", "N10", 0.0f, false, 40.0f, true});
    addEdge({"E9", "Sacramento St", haversineDistance(nodes["N15"], nodes["N14"]), "N15", "N14", 0.0f, false, 40.0f, true});
    addEdge({"E10", "Sacramento St", haversineDistance(nodes["N14"], nodes["N13"]), "N14", "N13", 0.0f, false, 40.0f, true});
    addEdge({"E11", "Sacramento St", haversineDistance(nodes["N13"], nodes["N12"]), "N13", "N12", 0.0f, false, 40.0f, true});
    addEdge({"E12", "Sacramento St", haversineDistance(nodes["N12"], nodes["N11"]), "N12", "N11", 0.0f, false, 40.0f, true});
    addEdge({"E13", "California St", haversineDistance(nodes["N16"], nodes["N17"]), "N16", "N17", 0.0f, false, 48.0f, false});
    addEdge({"E14", "California St", haversineDistance(nodes["N17"], nodes["N18"]), "N17", "N18", 0.0f, false, 48.0f, false});
    addEdge({"E15", "California St", haversineDistance(nodes["N18"], nodes["N19"]), "N18", "N19", 0.0f, false, 48.0f, false});
    addEdge({"E16", "California St", haversineDistance(nodes["N19"], nodes["N20"]), "N19", "N20", 0.0f, false, 48.0f, false});
    addEdge({"E17", "Pine St", haversineDistance(nodes["N25"], nodes["N24"]), "N25", "N24", 0.0f, false, 40.0f, true});
    addEdge({"E18", "Pine St", haversineDistance(nodes["N24"], nodes["N23"]), "N24", "N23", 0.0f, false, 40.0f, true});
    addEdge({"E19", "Pine St", haversineDistance(nodes["N23"], nodes["N22"]), "N23", "N22", 0.0f, false, 40.0f, true});
    addEdge({"E20", "Pine St", haversineDistance(nodes["N22"], nodes["N21"]), "N22", "N21", 0.0f, false, 40.0f, true});
    addEdge({"E21", "Bush St", haversineDistance(nodes["N26"], nodes["N27"]), "N26", "N27", 0.0f, false, 40.0f, true});
    addEdge({"E22", "Bush St", haversineDistance(nodes["N27"], nodes["N28"]), "N27", "N28", 0.0f, false, 40.0f, true});
    addEdge({"E23", "Bush St", haversineDistance(nodes["N28"], nodes["N29"]), "N28", "N29", 0.0f, false, 40.0f, true});
    addEdge({"E25", "Market St", haversineDistance(nodes["N31"], nodes["N32"]), "N31", "N32", 0.0f, false, 48.0f, false});
    addEdge({"E54", "Market St", haversineDistance(nodes["N32"], nodes["N30"]), "N32", "N30", 0.0f, false, 48.0f, false});
    addEdge({"E55", "Market St", haversineDistance(nodes["N30"], nodes["N33"]), "N30", "N33", 0.0f, false, 48.0f, false});
    addEdge({"E53", "Battery St", haversineDistance(nodes["N29"], nodes["N30"]), "N29", "N30", 0.0f, false, 40.0f, true});
    addEdge({"E56", "Front St", haversineDistance(nodes["N33"], nodes["N25"]), "N33", "N25", 0.0f, false, 40.0f, true});
    addEdge({"E26", "Kearny St", haversineDistance(nodes["N1"], nodes["N6"]), "N1", "N6", 0.0f, false, 40.0f, true});
    addEdge({"E27", "Kearny St", haversineDistance(nodes["N6"], nodes["N11"]), "N6", "N11", 0.0f, false, 40.0f, true});
    addEdge({"E28", "Kearny St", haversineDistance(nodes["N11"], nodes["N16"]), "N11", "N16", 0.0f, false, 40.0f, true});
    addEdge({"E29", "Kearny St", haversineDistance(nodes["N16"], nodes["N21"]), "N16", "N21", 0.0f, false, 40.0f, true});
    addEdge({"E30", "Kearny St", haversineDistance(nodes["N21"], nodes["N26"]), "N21", "N26", 0.0f, false, 40.0f, true});
    addEdge({"E31", "Kearny St", haversineDistance(nodes["N26"], nodes["N31"]), "N26", "N31", 0.0f, false, 40.0f, true});
    addEdge({"E32", "Montgomery St", haversineDistance(nodes["N2"], nodes["N7"]), "N2", "N7", 0.0f, false, 48.0f, false});
    addEdge({"E33", "Montgomery St", haversineDistance(nodes["N7"], nodes["N12"]), "N7", "N12", 0.0f, false, 48.0f, false});
    addEdge({"E34", "Montgomery St", haversineDistance(nodes["N12"], nodes["N17"]), "N12", "N17", 0.0f, false, 48.0f, false});
    addEdge({"E35", "Montgomery St", haversineDistance(nodes["N17"], nodes["N22"]), "N17", "N22", 0.0f, false, 48.0f, false});
    addEdge({"E36", "Montgomery St", haversineDistance(nodes["N22"], nodes["N27"]), "N22", "N27", 0.0f, false, 48.0f, false});
    addEdge({"E37", "Montgomery St", haversineDistance(nodes["N27"], nodes["N32"]), "N27", "N32", 0.0f, false, 48.0f, false});
    addEdge({"E38", "Sansome St", haversineDistance(nodes["N28"], nodes["N23"]), "N28", "N23", 0.0f, false, 40.0f, true});
    addEdge({"E39", "Sansome St", haversineDistance(nodes["N23"], nodes["N18"]), "N23", "N18", 0.0f, false, 40.0f, true});
    addEdge({"E40", "Sansome St", haversineDistance(nodes["N18"], nodes["N13"]), "N18", "N13", 0.0f, false, 40.0f, true});
    addEdge({"E42", "Sansome St", haversineDistance(nodes["N8"], nodes["N3"]), "N8", "N3", 0.0f, false, 40.0f, true});
    addEdge({"E43", "Battery St", haversineDistance(nodes["N4"], nodes["N9"]), "N4", "N9", 0.0f, false, 40.0f, true});
    addEdge({"E44", "Battery St", haversineDistance(nodes["N9"], nodes["N14"]), "N9", "N14", 0.0f, false, 40.0f, true});
    addEdge({"E45", "Battery St", haversineDistance(nodes["N14"], nodes["N19"]), "N14", "N19", 0.0f, false, 40.0f, true});
    addEdge({"E46", "Battery St", haversineDistance(nodes["N19"], nodes["N24"]), "N19", "N24", 0.0f, false, 40.0f, true});
    addEdge({"E47", "Battery St", haversineDistance(nodes["N24"], nodes["N29"]), "N24", "N29", 0.0f, false, 40.0f, true});
    addEdge({"E49", "Front St", haversineDistance(nodes["N25"], nodes["N20"]), "N25", "N20", 0.0f, false, 40.0f, true});
    addEdge({"E50", "Front St", haversineDistance(nodes["N20"], nodes["N15"]), "N20", "N15", 0.0f, false, 40.0f, true});
    addEdge({"E51", "Front St", haversineDistance(nodes["N15"], nodes["N10"]), "N15", "N10", 0.0f, false, 40.0f, true});
    addEdge({"E100", "New Rd", haversineDistance(nodes["N35"], nodes["N28"]), "N35", "N28", 0.0f, false, 40.0f, false});
    addEdge({"E101", "New Rd", haversineDistance(nodes["N35"], nodes["N36"]), "N35", "N36", 0.0f, false, 40.0f, false});
    addEdge({"E102", "New Rd", haversineDistance(nodes["N35"], nodes["N37"]), "N35", "N37", 0.0f, false, 40.0f, false});
    addEdge({"E103", "New Rd", haversineDistance(nodes["N34"], nodes["N38"]), "N34", "N38", 0.0f, false, 40.0f, false});
    addEdge({"E104", "New Rd", haversineDistance(nodes["N5"], nodes["N42"]), "N5", "N42", 0.0f, false, 40.0f, false});
    addEdge({"E105", "New Rd", haversineDistance(nodes["N41"], nodes["N42"]), "N41", "N42", 0.0f, false, 40.0f, false});
    addEdge({"E106", "New Rd", haversineDistance(nodes["N43"], nodes["N44"]), "N43", "N44", 0.0f, false, 40.0f, false});
    addEdge({"E107", "New Rd", haversineDistance(nodes["N42"], nodes["N43"]), "N42", "N43", 0.0f, false, 40.0f, false});
    addEdge({"E108", "New Rd", haversineDistance(nodes["N40"], nodes["N10"]), "N40", "N10", 0.0f, false, 40.0f, false});
    addEdge({"E109", "New Rd", haversineDistance(nodes["N41"], nodes["N40"]), "N41", "N40", 0.0f, false, 40.0f, false});
    addEdge({"E110", "New Rd", haversineDistance(nodes["N15"], nodes["N39"]), "N15", "N39", 0.0f, false, 40.0f, false});
    addEdge({"E111", "New Rd", haversineDistance(nodes["N34"], nodes["N20"]), "N34", "N20", 0.0f, false, 40.0f, false});
    addEdge({"E112", "New Rd", haversineDistance(nodes["N34"], nodes["N60"]), "N34", "N60", 0.0f, false, 40.0f, false});
    addEdge({"E113", "New Rd", haversineDistance(nodes["N61"], nodes["N33"]), "N61", "N33", 0.0f, false, 40.0f, false});
    addEdge({"E114", "New Rd", haversineDistance(nodes["N59"], nodes["N60"]), "N59", "N60", 0.0f, false, 40.0f, false});
    addEdge({"E115", "New Rd", haversineDistance(nodes["N39"], nodes["N59"]), "N39", "N59", 0.0f, false, 40.0f, false});
    addEdge({"E116", "New Rd", haversineDistance(nodes["N40"], nodes["N39"]), "N40", "N39", 0.0f, false, 40.0f, false});
    addEdge({"E117", "New Rd", haversineDistance(nodes["N58"], nodes["N59"]), "N58", "N59", 0.0f, false, 40.0f, false});
    addEdge({"E118", "New Rd", haversineDistance(nodes["N40"], nodes["N58"]), "N40", "N58", 0.0f, false, 40.0f, false});
    addEdge({"E119", "New Rd", haversineDistance(nodes["N44"], nodes["N58"]), "N44", "N58", 0.0f, false, 40.0f, false});
    addEdge({"E120", "New Rd", haversineDistance(nodes["N45"], nodes["N44"]), "N45", "N44", 0.0f, false, 40.0f, false});
    addEdge({"E121", "New Rd", haversineDistance(nodes["N46"], nodes["N45"]), "N46", "N45", 0.0f, false, 40.0f, false});
    addEdge({"E122", "New Rd", haversineDistance(nodes["N47"], nodes["N46"]), "N47", "N46", 0.0f, false, 40.0f, false});
    addEdge({"E123", "New Rd", haversineDistance(nodes["N47"], nodes["N62"]), "N47", "N62", 0.0f, false, 40.0f, false});
    addEdge({"E124", "New Rd", haversineDistance(nodes["N63"], nodes["N62"]), "N63", "N62", 0.0f, false, 40.0f, false});
    addEdge({"E125", "New Rd", haversineDistance(nodes["N48"], nodes["N63"]), "N48", "N63", 0.0f, false, 40.0f, false});
    addEdge({"E126", "New Rd", haversineDistance(nodes["N49"], nodes["N48"]), "N49", "N48", 0.0f, false, 40.0f, false});
    addEdge({"E127", "New Rd", haversineDistance(nodes["N50"], nodes["N49"]), "N50", "N49", 0.0f, false, 40.0f, false});
    addEdge({"E128", "New Rd", haversineDistance(nodes["N51"], nodes["N50"]), "N51", "N50", 0.0f, false, 40.0f, false});
    addEdge({"E129", "New Rd", haversineDistance(nodes["N53"], nodes["N51"]), "N53", "N51", 0.0f, false, 40.0f, false});
    addEdge({"E130", "New Rd", haversineDistance(nodes["N53"], nodes["N64"]), "N53", "N64", 0.0f, false, 40.0f, false});
    addEdge({"E131", "New Rd", haversineDistance(nodes["N65"], nodes["N51"]), "N65", "N51", 0.0f, false, 40.0f, false});
    addEdge({"E132", "New Rd", haversineDistance(nodes["N64"], nodes["N65"]), "N64", "N65", 0.0f, false, 40.0f, false});
    addEdge({"E133", "New Rd", haversineDistance(nodes["N66"], nodes["N65"]), "N66", "N65", 0.0f, false, 40.0f, false});
    addEdge({"E134", "New Rd", haversineDistance(nodes["N67"], nodes["N64"]), "N67", "N64", 0.0f, false, 40.0f, false});
    addEdge({"E135", "New Rd", haversineDistance(nodes["N68"], nodes["N67"]), "N68", "N67", 0.0f, false, 40.0f, false});
    addEdge({"E136", "New Rd", haversineDistance(nodes["N69"], nodes["N66"]), "N69", "N66", 0.0f, false, 40.0f, false});
    addEdge({"E137", "New Rd", haversineDistance(nodes["N70"], nodes["N69"]), "N70", "N69", 0.0f, false, 40.0f, false});
    addEdge({"E138", "New Rd", haversineDistance(nodes["N68"], nodes["N69"]), "N68", "N69", 0.0f, false, 40.0f, false});
    addEdge({"E139", "New Rd", haversineDistance(nodes["N73"], nodes["N66"]), "N73", "N66", 0.0f, false, 40.0f, false});
    addEdge({"E141", "New Rd", haversineDistance(nodes["N70"], nodes["N73"]), "N70", "N73", 0.0f, false, 40.0f, false});
    addEdge({"E142", "New Rd", haversineDistance(nodes["N67"], nodes["N66"]), "N67", "N66", 0.0f, false, 40.0f, false});
    addEdge({"E143", "New Rd", haversineDistance(nodes["N70"], nodes["N63"]), "N70", "N63", 0.0f, false, 40.0f, false});
    addEdge({"E144", "New Rd", haversineDistance(nodes["N72"], nodes["N69"]), "N72", "N69", 0.0f, false, 40.0f, false});
    addEdge({"E145", "New Rd", haversineDistance(nodes["N61"], nodes["N72"]), "N61", "N72", 0.0f, false, 40.0f, false});
    addEdge({"E146", "New Rd", haversineDistance(nodes["N71"], nodes["N72"]), "N71", "N72", 0.0f, false, 40.0f, false});
    addEdge({"E147", "New Rd", haversineDistance(nodes["N71"], nodes["N70"]), "N71", "N70", 0.0f, false, 40.0f, false});
    addEdge({"E148", "New Rd", haversineDistance(nodes["N73"], nodes["N74"]), "N73", "N74", 0.0f, false, 40.0f, false});
    addEdge({"E149", "New Rd", haversineDistance(nodes["N65"], nodes["N75"]), "N65", "N75", 0.0f, false, 40.0f, false});
    addEdge({"E151", "New Rd", haversineDistance(nodes["N76"], nodes["N61"]), "N76", "N61", 0.0f, false, 40.0f, false});
    addEdge({"E154", "New Rd", haversineDistance(nodes["N76"], nodes["N77"]), "N76", "N77", 0.0f, false, 40.0f, false});
    addEdge({"E155", "New Rd", haversineDistance(nodes["N77"], nodes["N68"]), "N77", "N68", 0.0f, false, 40.0f, false});
    addEdge({"E170", "New Rd", haversineDistance(nodes["N79"], nodes["N78"]), "N79", "N78", 0.0f, false, 40.0f, false});
    addEdge({"E179", "Clay St", haversineDistance(nodes["N7"], nodes["N78"]), "N7", "N78", 0.0f, false, 40.0f, true});
    addEdge({"E180", "Clay St", haversineDistance(nodes["N78"], nodes["N8"]), "N78", "N8", 0.0f, false, 40.0f, true});
    addEdge({"E181", "New Rd", haversineDistance(nodes["N81"], nodes["N79"]), "N81", "N79", 0.0f, false, 40.0f, false});
    addEdge({"E182", "New Rd", haversineDistance(nodes["N79"], nodes["N80"]), "N79", "N80", 0.0f, false, 40.0f, false});
    addEdge({"E185", "Sansome St", haversineDistance(nodes["N13"], nodes["N80"]), "N13", "N80", 0.0f, false, 40.0f, true});
    addEdge({"E186", "Sansome St", haversineDistance(nodes["N80"], nodes["N8"]), "N80", "N8", 0.0f, false, 40.0f, true});
    addEdge({"E187", "New Rd", haversineDistance(nodes["N82"], nodes["N79"]), "N82", "N79", 0.0f, false, 40.0f, false});
}
