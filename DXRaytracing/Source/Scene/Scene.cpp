#include "Pch.h"
#include "Scene/Scene.h"
#include "Graphics/Renderer.h"

Scene::Scene()
{
	glm::vec2 resolution = Renderer::GetResolution();
	m_SceneCamera = Camera(glm::vec3(0.0f, 0.0f, 2.0f), 60.0f, resolution.x, resolution.y, 0.1f, 1000.0f);
}

void Scene::Update(float deltaTime)
{
	m_SceneCamera.Update(deltaTime);
}

void Scene::Render()
{
}

void Scene::OnWindowResize(uint32_t width, uint32_t height)
{
	m_SceneCamera.ResizeProjection(width, height);
}
