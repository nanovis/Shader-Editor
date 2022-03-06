#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
Camera::Camera()
{
    this->cameraPosition = glm::vec3(0.0f, 0.0f, 5.0f);  //eye
    this->cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    this->upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    this->nearPlane = 1.0f;
    this->farPlane = 100.0f;
    this->fieldOfView = 45.0f;
    this->aspectRatio = 1.33f;
    updateProjectionMatrix();
    updateViewMatrix();
    glm::mat4 model_matrix=glm::mat4(1.0f);
    updatetransformMatrix(model_matrix);
}
Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 upVector)
{
    this->cameraPosition = position;  //eye
    this->cameraTarget = target;
    this->upVector = upVector;

    this->nearPlane = 1.0f;
    this->farPlane = 100.0f;
    this->fieldOfView = 45.0f;
    this->aspectRatio = 1.33f;
    glm::mat4 model_matrix=glm::mat4(1.0f);
    updateProjectionMatrix();
    updateViewMatrix();
    updatetransformMatrix(model_matrix);
}
Camera::~Camera()
{

}
void Camera::updateViewMatrix()
{
    this->viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);

}

glm::mat4 Camera::getViewMatrix()
{
    return viewMatrix;
}

void Camera::updateProjectionMatrix()
{
    this->projectionMatrix=glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane);
}

glm::mat4 Camera::getProjectionMatrix()
{
    return projectionMatrix;
}


float Camera::getAspectRatio()
{
    return aspectRatio;
}

float Camera::getFov()
{
    return fieldOfView;
}
void Camera::setnearPlane(const float nearPlane)
{
    this->nearPlane=nearPlane;
    updateProjectionMatrix();
}
void Camera::setfarPlane(const float farPlane)
{
    this->farPlane=farPlane;
    updateProjectionMatrix();
}
void Camera::setfieldOfView(const float fieldOfView)
{
    this->fieldOfView=fieldOfView;
    updateProjectionMatrix();
}
void Camera::setaspectRatio(const float aspectRatio)
{
    this->aspectRatio=aspectRatio;
    updateProjectionMatrix();
}
void Camera::setcameraPosition(const glm::vec3 cameraPosition)
{
    this->cameraPosition=cameraPosition;
    updateViewMatrix();
}
void Camera::setupVector(const glm::vec3 upVector)
{
    this->upVector=upVector;
    updateViewMatrix();
}
void Camera::setcameraTarget(const glm::vec3 cameraTarget)
{
    this->cameraTarget=cameraTarget;
    updateViewMatrix();
}
glm::vec3 Camera::getcameraPosition()
{
    return this->cameraPosition;
}
glm::vec3 Camera::getcameraTarget()
{
    return this->cameraTarget;
}
glm::vec3 Camera::getupVector()
{
    return this->upVector;
}
void Camera::updatetransformMatrix(const glm::mat4 modelmatrix)
{
    this->transformMatrix=this->projectionMatrix * this->viewMatrix * modelmatrix;
}
