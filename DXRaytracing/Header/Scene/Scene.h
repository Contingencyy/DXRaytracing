#pragma once
#include "Camera.h"

class Scene
{
public:
	Scene();

	void Update(float deltaTime);
	void Render();

	void OnWindowResize(uint32_t width, uint32_t height);

	const Camera& GetCamera() const { return m_SceneCamera; }

private:
	Camera m_SceneCamera;

};
