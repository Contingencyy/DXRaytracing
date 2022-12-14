#pragma once
#include "Transform.h"

class Camera
{
public:
	Camera() = default;
	Camera(const glm::vec3& pos, float fov, float width, float height, float near = 0.1f, float far = 10000.0f);
	~Camera();

	void Update(float deltaTime);
	void ResizeProjection(float width, float height);

	float GetExposure() const { return m_Exposure; }
	float GetGamma() const { return m_Gamma; }

	glm::mat4 GetViewMatrix() const { return m_ViewMatrix; }
	glm::mat4 GetProjectionMatrix() const { return m_ProjectionMatrix; }
	glm::mat4 GetViewProjection() const { return m_ViewProjectionMatrix; };
	const Transform& GetTransform() const { return m_Transform; }

private:
	bool UpdateMovement(float deltaTime);
	bool UpdateRotation(float deltaTime);

private:
	Transform m_Transform;

	glm::mat4 m_ViewMatrix = glm::identity<glm::mat4>();
	glm::mat4 m_ProjectionMatrix = glm::identity<glm::mat4>();
	glm::mat4 m_ViewProjectionMatrix = glm::identity<glm::mat4>();

	float m_Speed = 2.0f;// 250.0f;
	float m_RotationSpeed = 0.5f, m_RotationDeadZone = 25.0f;
	float m_FOV = 60.0f, m_AspectRatio = 16.0f / 9.0f;
	float m_Yaw = 0.0f, m_Pitch = 0.0f;
	float m_Exposure = 1.5f;
	float m_Gamma = 2.2f;
	float m_Near = 0.1f;
	float m_Far = 1000.0f;

	glm::vec2 m_RotationAnchorPoint = glm::vec2(0.0f);
	bool m_SetAnchorPointOnClick = true;

};
