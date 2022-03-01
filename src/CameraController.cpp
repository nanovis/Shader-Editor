#include"CameraController.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

CameraController::CameraController(float speed)
{
    this->speed=speed;
    this->is_forward_pressed=false;
    this->is_backward_pressed=false;
    this->is_left_pressed=false;
    this->is_right_pressed=false;
}
CameraController::~CameraController()
{

}
void CameraController::process_events(int event)
{
    switch (event)
    {
        case 1: //w
            this->is_forward_pressed=true;
            break;
        case 2: //s
            this->is_backward_pressed=true;
            break;
        case 3: //a
            this->is_left_pressed=true;
            break;
        case 4: //d
            this->is_right_pressed=true;
            break;
    }

}
void CameraController::update_camera( Camera &camera)
{
    glm::vec3 forward=camera.getcameraTarget()-camera.getcameraPosition();
    glm::vec3 forward_norm=glm::normalize(forward);
    float forward_mag=glm::length(forward);
    if (this->is_forward_pressed && forward_mag>this->speed)
    {
        camera.setcameraPosition(camera.getcameraPosition()+forward_norm*this->speed);
        this->is_forward_pressed=false;
    }
    if (this->is_backward_pressed && forward_mag>this->speed)
    {
        camera.setcameraPosition(camera.getcameraPosition()-forward_norm*this->speed);
        this->is_backward_pressed=false;
    }
    glm::vec3 right=glm::cross(forward_norm,camera.getupVector());

    forward=camera.getcameraTarget()-camera.getcameraPosition();
    forward_mag=glm::length(forward);
    if(this->is_right_pressed)
    {
        camera.setcameraPosition(camera.getcameraTarget()-glm::normalize(forward+right*this->speed)*forward_mag);
        this->is_right_pressed=false;
    }
    if(this->is_left_pressed)
    {
        camera.setcameraPosition(camera.getcameraTarget()-glm::normalize(forward-right*this->speed)*forward_mag);
        this->is_left_pressed=false;
    }

}