#include "Pch.h"
#include "Scene/Camera.h"
#include "InputHandler.h"
#include "Application.h"
#include "Window.h"

Camera::Camera(const glm::vec3& pos, float fov, float width, float height, float near, float far)
	: m_FOV(fov), m_Near(near), m_Far(far)
{
	m_AspectRatio = width / height;

	m_Transform.Translate(pos);

	m_ViewMatrix = glm::inverse(m_Transform.GetTransformMatrix());
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_Near, m_Far);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

Camera::~Camera()
{
}

void Camera::Update(float deltaTime)
{
	bool translated = UpdateMovement(deltaTime);
	bool rotated = UpdateRotation(deltaTime);

	if (translated || rotated)
	{
		m_ViewMatrix = glm::inverse(m_Transform.GetTransformMatrix());
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
}

void Camera::ResizeProjection(float width, float height)
{
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_Near, m_Far);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	m_AspectRatio = width / height;
}

bool Camera::UpdateMovement(float deltaTime)
{
	glm::vec3 velocity = glm::vec3(0.0f);
	const Transform& invTransform = m_Transform.GetInverse();

	velocity += InputHandler::GetInputAxis1D(KeyCode::W, KeyCode::S) * invTransform.Forward();
	velocity += InputHandler::GetInputAxis1D(KeyCode::D, KeyCode::A) * invTransform.Right();
	velocity += InputHandler::GetInputAxis1D(KeyCode::SPACEBAR, KeyCode::CTRL) * invTransform.Up();

	float speedMultiplier = InputHandler::IsKeyPressed(KeyCode::SHIFT) ? 10.0f : 1.0f;

	if (glm::length(velocity) > 0.0f)
	{
		velocity = glm::normalize(velocity);
		m_Transform.Translate(velocity * m_Speed * speedMultiplier * deltaTime);

		return true;
	}

	return false;
}

bool Camera::UpdateRotation(float deltaTime)
{
	if (InputHandler::IsKeyPressed(KeyCode::RIGHT_MOUSE))
	{
		if (m_SetAnchorPointOnClick)
		{
			m_RotationAnchorPoint = InputHandler::GetMousePositionAbs();
			m_SetAnchorPointOnClick = false;
		}

		glm::vec2 mouseDelta = InputHandler::GetMousePositionAbs() - m_RotationAnchorPoint;
		float mouseDeltaLength = glm::length(mouseDelta) - m_RotationDeadZone;

		if (mouseDeltaLength > 0)
		{
			glm::vec2 mouseDirectionFromCenter = glm::normalize(mouseDelta);
			float rotationStrength = sqrt(sqrt(mouseDeltaLength));

			float yawSign = m_Transform.Up().y < 0.0f ? -1.0f : 1.0f;
			m_Yaw += yawSign * mouseDirectionFromCenter.x * m_RotationSpeed * rotationStrength * deltaTime;
			m_Pitch += mouseDirectionFromCenter.y * m_RotationSpeed * rotationStrength * deltaTime;

			m_Transform.SetRotation(glm::vec3(m_Pitch, m_Yaw, 0.0f));

			return true;
		}
	}
	else if (!m_SetAnchorPointOnClick)
	{
		m_SetAnchorPointOnClick = true;
	}

	/*if (InputHandler::IsKeyPressed(KeyCode::Q) || InputHandler::IsKeyPressed(KeyCode::E))
	{
		float rollInputAxis = InputHandler::GetInputAxis1D(KeyCode::Q, KeyCode::E);
		glm::vec3 rotation(0.0f);
		rotation.z = rollInputAxis;

		m_Transform.Rotate(rotation);
		rotated = true;
	}*/

	return false;
}
