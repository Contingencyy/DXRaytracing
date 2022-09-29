#pragma once
#include "Camera.h"

class Scene
{
public:
	Scene();

	void Update(float deltaTime);
	void Render();

	const Camera& GetCamera() const { return m_SceneCamera; }

private:
	Camera m_SceneCamera;

};
