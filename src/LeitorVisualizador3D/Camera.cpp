#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

using namespace glm;
using namespace std;

Camera::Camera(vec3 startPos, vec3 startUp, float startYaw, float startPitch)
    : front(vec3(0.0f, 0.0f, -1.0f)), movementSpeed(5.0f), mouseSensitivity(0.1f)
{
    position = startPos;
    worldUp  = startUp;
    yaw      = startYaw;
    pitch    = startPitch;
    updateCameraVectors();
}

mat4 Camera::getViewMatrix()
{
    return lookAt(position, position + front, up);
}

void Camera::processKeyboard(const string& direction, float deltaTime)
{
    float velocity = movementSpeed * deltaTime;
    if (direction == "FORWARD")  position += front * velocity;
    if (direction == "BACKWARD") position -= front * velocity;
    if (direction == "LEFT")     position -= right * velocity;
    if (direction == "RIGHT")    position += right * velocity;
    if (direction == "UP")       position += up * velocity;
    if (direction == "DOWN")     position -= up * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if (constrainPitch)
    {
        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    vec3 newFront;
    newFront.x = cos(radians(yaw)) * cos(radians(pitch));
    newFront.y = sin(radians(pitch));
    newFront.z = sin(radians(yaw)) * cos(radians(pitch));
    front = normalize(newFront);
    right = normalize(cross(front, worldUp));
    up    = normalize(cross(right, front));
}
