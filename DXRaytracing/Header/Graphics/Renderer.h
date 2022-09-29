#pragma once

class Camera;

class Renderer
{
public:
	static void Initialize(uint32_t resX, uint32_t resY);
	static void Finalize();
	
	static void BeginScene(const Camera& sceneCamera);
	static void Render();
	static void EndScene();

	static void OnWindowResize(uint32_t width, uint32_t height);
	static void ToggleVSync();
	
	static glm::vec2 GetResolution();

private:
	Renderer();
	~Renderer();

	static void CreateRenderPasses();
	static void CreateBLAS();
	static void CreateTLAS();

};
