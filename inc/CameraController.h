#pragma once
#include <glm/glm.hpp>
#include "Camera.h"
class CameraController
{
public:
    CameraController(float speed);
    ~CameraController();
    void process_events(int event);
    void update_camera( Camera &camera);

private:
    bool is_forward_pressed;
    bool is_backward_pressed;
    bool is_left_pressed;
    bool is_right_pressed;
    float speed;
};