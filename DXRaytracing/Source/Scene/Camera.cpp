#include "Pch.h"
#include "Scene/Camera.h"

Camera::Camera(const glm::vec3& pos, float fov, float width, float height, float near, float far)
	: m_FOV(fov), m_Near(near), m_Far(far)
{
	m_AspectRatio = width / height;

	m_ViewMatrix = glm::translate(m_ViewMatrix, pos);
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_Near, m_Far);
}

glm::mat4 Camera::GetViewProjection() const
{
	return m_ProjectionMatrix * m_ViewMatrix;
}
