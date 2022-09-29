#pragma once

class Camera
{
public:
	Camera() = default;
	Camera(const glm::vec3& pos, float fov, float width, float height, float near, float far);

	glm::mat4 GetViewProjection() const;

private:
	glm::mat4 m_ViewMatrix = glm::identity<glm::mat4>();
	glm::mat4 m_ProjectionMatrix = glm::identity<glm::mat4>();

	glm::vec3 m_Position = glm::vec3(0.0f);

	float m_FOV = 60.0f;
	float m_AspectRatio = 16.0f / 9.0f;
	float m_Near = 0.1f;
	float m_Far = 1000.0f;

};
