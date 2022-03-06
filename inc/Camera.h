#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
	Camera();
	Camera(glm::vec3 &position, glm::vec3 &target, glm::vec3 &upVector);

	~Camera();
	glm::mat4 transformMatrix;
	float getAspectRatio();

	float getFov();
	glm::mat4 getViewMatrix();
	glm::mat4 getProjectionMatrix();
	void setnearPlane(const float nearPlane);
	void setfarPlane(const float farPlane);
	void setfieldOfView(const float fieldOfView);
	void setaspectRatio(const float aspectRatio);
	void setcameraPosition(const glm::vec3 cameraPosition);
	void setupVector(const glm::vec3 upVector);
	void setcameraTarget(const glm::vec3 cameraTarget);
	glm::vec3 getcameraPosition();
	glm::vec3 getcameraTarget();
	glm::vec3 getupVector();
	void updatetransformMatrix(const glm::mat4 modelmatrix);
private:
	void updateViewMatrix();
	void updateProjectionMatrix();

	float nearPlane;
	float farPlane;
	float fieldOfView;
	float aspectRatio;

	glm::vec3 cameraPosition;
	glm::vec3 upVector;
	glm::vec3 cameraTarget;

	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
	
};