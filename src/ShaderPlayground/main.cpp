#pragma once
#include "global.h"
#include "VulkanInstance.h"

int window_height = 720;
int window_width = 1284;

class ShaderPlaygroundApplication 
{
public:
	void run();

private:
	GLFWwindow* window;
	VulkanInstance* instance;

	void initialize();
	void initWindow(int width, int height, const char* name);
	void initVulkan(const char* applicationName);

	void mainLoop();
	void cleanup();
};

void ShaderPlaygroundApplication::initWindow(int width, int height, const char* name)
{
	if (!glfwInit()) 
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		exit(EXIT_FAILURE);
	}

	if (!glfwVulkanSupported()) 
	{
		fprintf(stderr, "Vulkan not supported\n");
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	window = glfwCreateWindow(width, height, name, nullptr, nullptr);

	if (!window) {
		fprintf(stderr, "Failed to initialize GLFW window\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
}

void ShaderPlaygroundApplication::initVulkan(const char* applicationName)
{
	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Vulkan Instance
	instance = new VulkanInstance(applicationName, glfwExtensionCount, glfwExtensions);

}

void ShaderPlaygroundApplication::initialize()
{
	static constexpr char* applicationName = "Shader Playground";
	initWindow(window_width, window_height, applicationName);
	initVulkan(applicationName);

	//glfwSetWindowSizeCallback(window, resizeCallback);
	//glfwSetScrollCallback(window, scrollCallback);
	//glfwSetMouseButtonCallback(window, mouseDownCallback);	
	//glfwSetCursorPosCallback(window, mouseMoveCallback);
}

void ShaderPlaygroundApplication::mainLoop()
{
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
	while (!glfwWindowShouldClose(window))
	{
		//Mouse inputs and window resize callbacks
		glfwPollEvents();
	}
}

void ShaderPlaygroundApplication::cleanup()
{	
	// Wait for the device to finish executing before cleanup
	//vkDeviceWaitIdle(device->GetVkDevice());


	delete instance;
	glfwDestroyWindow(window);
	glfwTerminate();
}

void ShaderPlaygroundApplication::run()
{
	initialize();
	mainLoop();
	cleanup();
}

int main() 
{
	ShaderPlaygroundApplication app;

	try 
	{
		app.run();
	}
	catch (const std::exception& e) 
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}