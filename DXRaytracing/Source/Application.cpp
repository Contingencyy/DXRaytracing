#include "Pch.h"
#include "Application.h"
#include "Window.h"
#include "Graphics/Renderer.h"
#include "Scene/Scene.h"

static Application* s_Instance = nullptr;

void Application::Create()
{
	if (!s_Instance)
		s_Instance = new Application();
}

void Application::Destroy()
{
	delete s_Instance;
	s_Instance = nullptr;
}

Application& Application::Get()
{
	return *s_Instance;
}

void Application::Initialize()
{
	WindowProperties windowProps = {};
	windowProps.Title = L"DX Raytracing";
	windowProps.Width = 1280;
	windowProps.Height = 720;

	m_Window = std::make_unique<Window>();
	m_Window->Initialize(windowProps);
	m_Window->Show();

	Renderer::Initialize(m_Window->GetWidth(), m_Window->GetHeight());

	m_Scene = std::make_unique<Scene>();

	m_Initialized = true;
}

void Application::Run()
{
	std::chrono::time_point current = std::chrono::high_resolution_clock::now(), last = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> deltaTime = std::chrono::duration<float>(0.0f);

	while (!m_Window->ShouldClose())
	{
		current = std::chrono::high_resolution_clock::now();
		deltaTime = current - last;

		PollEvents();
		Update(deltaTime.count());
		Render();

		last = current;

		Profiler::Get().Reset();
	}
}

void Application::Finalize()
{
	Renderer::Finalize();
	m_Window->Finalize();
}

void Application::OnWindowResize(uint32_t width, uint32_t height)
{
	width = std::max(width, 1u);
	height = std::max(height, 1u);

	Renderer::OnWindowResize(width, height);
}

Application::Application()
{
}

Application::~Application()
{
}

void Application::PollEvents()
{
	SCOPED_TIMER("Application::PollEvents");
	m_Window->PollEvents();
}

void Application::Update(float deltaTime)
{
	SCOPED_TIMER("Application::Update");

	m_Scene->Update(deltaTime);
}

void Application::Render()
{
	SCOPED_TIMER("Application::Render");

	m_Scene->Render();

	Renderer::BeginScene(m_Scene->GetCamera());
	Renderer::Render();
	Renderer::EndScene();
}
