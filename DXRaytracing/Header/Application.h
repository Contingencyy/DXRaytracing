#pragma once

class Window;

class Application
{
public:
	static void Create();
	static void Destroy();
	static Application& Get();

	void Initialize();
	void Run();
	void Finalize();

	void OnWindowResize(uint32_t width, uint32_t height);

	bool IsInitialized() const { return m_Initialized; }

	Window& GetWindow() const { return *m_Window.get(); };

private:
	Application();
	~Application();

	void PollEvents();
	void Update(float deltaTime);
	void Render();

private:
	std::unique_ptr<Window> m_Window = nullptr;

	bool m_Initialized = false;

};
