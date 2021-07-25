#pragma once

#include "frustum.h"

class Camera
{
public:
    Camera();

    void setFieldOfView(float fov);
    float fieldOfView() const { return m_fov; }

    void setAspectRatio(float aspectRatio);
    float aspectRatio() const { return m_aspectRatio; }

    void setZNear(float zNear);
    float zNear() const { return m_zNear; }

    void setZFar(float zFar);
    float zFar() const { return m_zFar; }

    void setEye(const glm::vec3 &eye);
    glm::vec3 eye() const { return m_eye; }

    void setCenter(const glm::vec3 &center);
    glm::vec3 center() const { return m_center; }

    void setUp(const glm::vec3 &up);
    glm::vec3 up() const { return m_up; }

    glm::mat4 projectionMatrix() const { return m_projectionMatrix; }
    glm::mat4 viewMatrix() const { return m_viewMatrix; }

    const Frustum &frustum() const { return m_frustum; }

private:
    void updateProjectionMatrix();
    void updateViewMatrix();
    void updateFrustum();

    float m_fov;
    float m_aspectRatio;
    float m_zNear;
    float m_zFar;
    glm::vec3 m_eye;
    glm::vec3 m_center;
    glm::vec3 m_up;
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    Frustum m_frustum;
};
