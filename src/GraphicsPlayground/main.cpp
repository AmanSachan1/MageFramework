#pragma once
#include <global.h>
#include "vulkanInstance.h"
#include "vulkanDevices.h"

int window_height = 720;
int window_width = 1284;

class GraphicsPlaygroundApplication
{
public:
	void run();

private:
	GLFWwindow* window;
	VulkanInstance* instance;
	VkSurfaceKHR vkSurface;
	VulkanDevices* devices;

	void initialize();
	void initWindow(int width, int height, const char* name);
	void initVulkan(const char* applicationName);

	void mainLoop();
	void cleanup();
};

void GraphicsPlaygroundApplication::initWindow(int width, int height, const char* name)
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

	if (!window) 
	{
		fprintf(stderr, "Failed to initialize GLFW window\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
}

void GraphicsPlaygroundApplication::initVulkan(const char* applicationName)
{
	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Vulkan Instance
	instance = new VulkanInstance(applicationName, glfwExtensionCount, glfwExtensions);
	
	// Create Drawing Surface, i.e. window where things are rendered to
	if (glfwCreateWindowSurface(instance->GetVkInstance(), window, nullptr, &vkSurface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}

	// Create The physical and logical devices required by vulkan
	QueueFlagBits desiredQueues = QueueFlagBit::GraphicsBit | QueueFlagBit::ComputeBit | QueueFlagBit::TransferBit | QueueFlagBit::PresentBit;
	devices = new VulkanDevices(instance, { VK_KHR_SWAPCHAIN_EXTENSION_NAME }, desiredQueues, vkSurface);
}

void GraphicsPlaygroundApplication::initialize()
{
	static constexpr char* applicationName = "Shader Playground";
	initWindow(window_width, window_height, applicationName);
	initVulkan(applicationName);

	//glfwSetWindowSizeCallback(window, resizeCallback);
	//glfwSetScrollCallback(window, scrollCallback);
	//glfwSetMouseButtonCallback(window, mouseDownCallback);	
	//glfwSetCursorPosCallback(window, mouseMoveCallback);
}

void GraphicsPlaygroundApplication::mainLoop()
{
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
	while (!glfwWindowShouldClose(window))
	{
		//Mouse inputs and window resize callbacks
		glfwPollEvents();
	}
}

void GraphicsPlaygroundApplication::cleanup()
{
	// Wait for the device to finish executing before cleanup
	//vkDeviceWaitIdle(device->GetVkDevice());

	vkDestroySurfaceKHR(instance->GetVkInstance(), vkSurface, nullptr);
	delete instance;
	glfwDestroyWindow(window);
	glfwTerminate();
}

void GraphicsPlaygroundApplication::run()
{
	initialize();
	mainLoop();
	cleanup();
}

int main()
{
	GraphicsPlaygroundApplication app;

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