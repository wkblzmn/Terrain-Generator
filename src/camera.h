#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f, 80.0f, 0.0f),
           glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f, float pitch = -20.0f)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f))
        , MovementSpeed(40.0f)
        , MouseSensitivity(0.10f)
        , Zoom(60.0f)
        , WorldUp(up)
        , Yaw(yaw)
        , Pitch(pitch)
        , Position(position)
    {
        updateVectors();
    }

    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(Camera_Movement dir, float dt) {
        float v = MovementSpeed * dt;
        if (dir == FORWARD)  Position += Front   * v;
        if (dir == BACKWARD) Position -= Front   * v;
        if (dir == LEFT)     Position -= Right   * v;
        if (dir == RIGHT)    Position += Right   * v;
        if (dir == UP)       Position += WorldUp * v;
        if (dir == DOWN)     Position -= WorldUp * v;
    }

    void ProcessMouseMovement(float xoff, float yoff, bool clampPitch = true) {
        Yaw   += xoff * MouseSensitivity;
        Pitch += yoff * MouseSensitivity;
        if (clampPitch) {
            if (Pitch >  89.0f) Pitch =  89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }
        updateVectors();
    }

    void ProcessMouseScroll(float yoff) {
        Zoom -= yoff;
        Zoom = glm::clamp(Zoom, 10.0f, 90.0f);
    }

private:
    void updateVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};
