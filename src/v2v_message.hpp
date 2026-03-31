#pragma once

struct V2VMessage {
    int senderId;
    float x;
    float y;
    float speed;
    bool hasHazard;
    float heading;
    int currentEdgeIdx;
};