#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

using namespace glm;
using namespace std;

class Camera {
public:
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;

    Camera(vec3 startPos = vec3(0.0f, 0.0f, 3.0f),
           vec3 startUp  = vec3(0.0f, 1.0f, 0.0f),
           float startYaw     = -90.0f,
           float startPitch   = 0.0f);

    mat4 getViewMatrix();
    void processKeyboard(const string& direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

private:
    void updateCameraVectors();
};

#endif
