#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
    : m_fov(glm::radians(45.0f))
    , m_aspectRatio(1.0f)
    , m_zNear(0.1f)
    , m_zFar(100.0f)
    , m_eye(glm::vec3(1, 0, 0))
    , m_center(glm::vec3(0, 0, 0))
    , m_up(glm::vec3(0, 1, 0))
{
    updateProjectionMatrix();
    updateViewMatrix();
    updateFrustum();
}

void Camera::setFieldOfView(float fov)
{
    m_fov = fov;
    updateProjectionMatrix();
    updateFrustum();
}

void Camera::setAspectRatio(float aspectRatio)
{
    m_aspectRatio = aspectRatio;
    updateProjectionMatrix();
    updateFrustum();
}

void Camera::setZNear(float zNear)
{
    m_zNear = zNear;
    updateProjectionMatrix();
    updateFrustum();
}

void Camera::setZFar(float zFar)
{
    m_zFar = zFar;
    updateProjectionMatrix();
    updateFrustum();
}

void Camera::setEye(const glm::vec3 &eye)
{
    m_eye = eye;
    updateViewMatrix();
    updateFrustum();
}

void Camera::setCenter(const glm::vec3 &center)
{
    m_center = center;
    updateViewMatrix();
    updateFrustum();
}

void Camera::setUp(const glm::vec3 &up)
{
    m_up = up;
    updateViewMatrix();
    updateFrustum();
}

void Camera::updateProjectionMatrix()
{
    m_projectionMatrix = glm::perspective(m_fov, m_aspectRatio, m_zNear, m_zFar);
}

void Camera::updateViewMatrix()
{
    m_viewMatrix = glm::lookAt(m_eye, m_center, m_up);
}

void Camera::updateFrustum()
{
    const auto tang = glm::tan(0.5f * m_fov);

    const auto nearHeight = m_zNear * tang;
    const auto nearWidth = nearHeight * m_aspectRatio;

    const auto farHeight = m_zFar * tang;
    const auto farWidth = farHeight * m_aspectRatio;

    // TODO could get these from m_viewMatrix
    const auto viewZ = glm::normalize(m_eye - m_center);
    const auto viewX = glm::normalize(glm::cross(m_up, viewZ));
    const auto viewY = glm::cross(viewZ, viewX);

    const auto nearCenter = m_eye - viewZ * m_zNear;
    const auto farCenter = m_eye - viewZ * m_zFar;

    const auto ntl = nearCenter + viewY * nearHeight - viewX * nearWidth;
    const auto ntr = nearCenter + viewY * nearHeight + viewX * nearWidth;
    const auto nbl = nearCenter - viewY * nearHeight - viewX * nearWidth;
    const auto nbr = nearCenter - viewY * nearHeight + viewX * nearWidth;

    const auto ftl = farCenter + viewY * farHeight - viewX * farWidth;
    const auto ftr = farCenter + viewY * farHeight + viewX * farWidth;
    const auto fbl = farCenter - viewY * farHeight - viewX * farWidth;
    const auto fbr = farCenter - viewY * farHeight + viewX * farWidth;

    m_frustum.planes[0] = Plane(ntr, ntl, ftl);
    m_frustum.planes[1] = Plane(nbl, nbr, fbr);
    m_frustum.planes[2] = Plane(ntl, nbl, fbl);
    m_frustum.planes[3] = Plane(nbr, ntr, fbr);
    m_frustum.planes[4] = Plane(ntl, ntr, nbr);
    m_frustum.planes[5] = Plane(ftr, ftl, fbl);
}
