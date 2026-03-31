#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <cstring>
#include <zmq.hpp>
#include "fleetcomm.pb.h"
#include "traffic_simulation.hpp"

static void spawnCars(TrafficSimulation& sim, std::mt19937& rng) {
    const std::vector<std::string> nodeIds = {
        "N1","N2","N3","N4","N5","N6","N7","N8","N9","N10","N11","N12",
        "N13","N14","N15","N16","N17","N18","N19","N20","N21","N22","N23",
        "N24","N26","N27","N28","N29","N30","N31","N32","N33"
    };
    const std::vector<CarModel> models = {
        {"Tesla Model 3", 0.0045f, 0.0020f},
        {"Ford Transit",  0.0055f, 0.0022f},
        {"Toyota Prius",  0.0046f, 0.0018f},
        {"BMW i3",        0.0040f, 0.0018f},
        {"Honda Civic",   0.0045f, 0.0018f},
    };

    std::uniform_int_distribution<size_t> nodeDist(0, nodeIds.size() - 1);

    for (int i = 1; i <= 72; ++i) {
        std::string from, to;
        do {
            from = nodeIds[nodeDist(rng)];
            to   = nodeIds[nodeDist(rng)];
        } while (from == to);

        sim.spawnCar(i, models[i % models.size()], from, to);
    }
}

static void publishFrame(zmq::socket_t& pub, TrafficSimulation& sim) {
    SimFrame f = sim.latestFrame();

    fleetcomm::WorldState ws;
    ws.set_tick(f.tick);
    ws.set_mode(sim.v2vEnabled ? "v2v" : "human");
    for (const auto& cf : f.cars) {
        auto* cs = ws.add_cars();
        cs->set_id(cf.id);
        cs->set_lat(cf.lat);   // double
        cs->set_lng(cf.lng);   // double
        cs->set_speed(cf.speed);
        cs->set_heading(cf.heading);
        cs->set_status(static_cast<int32_t>(cf.status));
        float limit = sim.getEdgeSpeedLimit(cf.currentEdgeIdx);
        cs->set_braking_intensity(1.0f - std::min(cf.speed / limit, 1.0f));
    }

    std::string data = ws.SerializeAsString();
    zmq::message_t msg(data.size());
    std::memcpy(msg.data(), data.data(), data.size());
    pub.send(msg, zmq::send_flags::none);
}

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    zmq::context_t zmqCtx(1);

    // PUB — world state to Go gateway
    zmq::socket_t pub(zmqCtx, zmq::socket_type::pub);
    pub.bind("tcp://*:5555");

    // PULL — mode commands from Go gateway
    zmq::socket_t cmdPull(zmqCtx, zmq::socket_type::pull);
    cmdPull.bind("tcp://*:5556");
    cmdPull.set(zmq::sockopt::rcvtimeo, 0); // non-blocking

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::mt19937 rng(std::random_device{}());

    TrafficSimulation sim;
    spawnCars(sim, rng);

    std::cout << "FleetComm publishing on tcp://*:5555  |  commands on tcp://*:5556\n";

    for (int tick = 1; ; ++tick) {
        if (tick == 500) sim.triggerBreakdown(3);

        // Poll for mode command — non-blocking
        zmq::message_t cmdMsg;
        if (cmdPull.recv(cmdMsg, zmq::recv_flags::dontwait)) {
            std::string cmd(static_cast<char*>(cmdMsg.data()), cmdMsg.size());
            if (cmd == "v2v")   { sim.v2vEnabled = true;  std::cout << "[MODE] V2V\n"; }
            if (cmd == "human") { sim.v2vEnabled = false; std::cout << "[MODE] Human\n"; }
        }

        sim.tick();
        publishFrame(pub, sim);

        if (tick % 300 == 0) sim.printMetrics();

        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
