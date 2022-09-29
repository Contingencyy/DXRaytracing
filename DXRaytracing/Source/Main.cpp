#include "Pch.h"
#include "Application.h"

int main()
{
	Application::Create();
	Application::Get().Initialize();
	Application::Get().Run();
	Application::Get().Finalize();
	Application::Destroy();

	return 0;
}