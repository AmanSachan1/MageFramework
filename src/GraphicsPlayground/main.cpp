#pragma once
#include <global.h>
#include <forward.h>
#include "vulkanInstance.h"
#include "vulkanDevices.h"
#include "vulkanPresentation.h"

#include "camera.h"
#include "Renderer.h"

int window_height = 720;
int window_width = 1284;

VulkanDevices* devices;
VulkanPresentation* presentation;
Camera* camera;

namespace InputUtil
{
	void resizeCallback(GLFWwindow* window, int width, int height)
	{
		if (width == 0 || height == 0) return;

		vkDeviceWaitIdle(devices->GetLogicalDevice());
		presentation->Recreate(width, height);
		//renderer->RecreateOnResize(width, height);
	}

	bool leftMouseDown = false;
	double previousX = 0.0f;
	double previousY = 0.0f;
	float deltaForRotation = 0.25f;
	float deltaForMovement = 10.0f;

	void keyboardInputs(GLFWwindow* window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, true);
		}

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			camera->TranslateAlongLook(deltaForMovement);
		}			
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			camera->TranslateAlongLook(-deltaForMovement);
		}

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			camera->TranslateAlongRight(-deltaForMovement);
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			camera->TranslateAlongRight(deltaForMovement);
		}			

		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			camera->TranslateAlongUp(deltaForMovement);
		}		
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			camera->TranslateAlongUp(-deltaForMovement);
		}

		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
			camera->RotateAboutRight(deltaForRotation);
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			camera->RotateAboutRight(-deltaForRotation);
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
			camera->RotateAboutUp(deltaForRotation);
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
			camera->RotateAboutUp(-deltaForRotation);
		}
	}

	void mouseDownCallback(GLFWwindow* window, int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS) {
				leftMouseDown = true;
				glfwGetCursorPos(window, &previousX, &previousY);
			}
			else if (action == GLFW_RELEASE) {
				leftMouseDown = false;
			}
		}
	}

	void mouseMoveCallback(GLFWwindow* window, double xPosition, double yPosition)
	{
		if (leftMouseDown)
		{
			double sensitivity = 0.1;
			float deltaX = static_cast<float>((previousX - xPosition) * sensitivity);
			float deltaY = static_cast<float>((previousY - yPosition) * sensitivity);
			previousX = xPosition;
			previousY = yPosition;

			camera->RotateAboutUp(deltaX);
			camera->RotateAboutRight(deltaY);
		}
	}

	void scrollCallback(GLFWwindow*, double, double yoffset)
	{
		camera->TranslateAlongLook(static_cast<float>(yoffset) * 0.05f);
	}
}

class GraphicsPlaygroundApplication
{
public:
	void run();

private:
	GLFWwindow* window;
	VulkanInstance* instance;
	Renderer* renderer;
	VkSurfaceKHR vkSurface;

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

	presentation = new VulkanPresentation(devices, vkSurface, window_width, window_height);
}

void GraphicsPlaygroundApplication::initialize()
{
	static constexpr char* applicationName = "Shader Playground";
	initWindow(window_width, window_height, applicationName);
	initVulkan(applicationName);

	camera = new Camera(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 1.0f),
		window_width, window_height, 45.0f, float(window_width) / float(window_height), 0.1f, 1000.0f);

	VkDevice logicalDevice = devices->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = devices->GetPhysicalDevice();
	renderer = new Renderer(logicalDevice, physicalDevice, presentation, camera, window_width, window_height);

	glfwSetWindowSizeCallback(window, InputUtil::resizeCallback);
	glfwSetScrollCallback(window, InputUtil::scrollCallback);
	glfwSetMouseButtonCallback(window, InputUtil::mouseDownCallback);
	glfwSetCursorPosCallback(window, InputUtil::mouseMoveCallback);
}

void GraphicsPlaygroundApplication::mainLoop()
{
	// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
	while (!glfwWindowShouldClose(window))
	{
		//Mouse inputs and window resize callbacks
		glfwPollEvents();
		InputUtil::keyboardInputs(window);

		renderer->renderLoop();
	}
}

void GraphicsPlaygroundApplication::cleanup()
{
	// Wait for the device to finish executing before cleanup
	vkDeviceWaitIdle(devices->GetLogicalDevice());
	
	delete renderer;
	delete presentation;
	vkDestroySurfaceKHR(instance->GetVkInstance(), vkSurface, nullptr);
	delete devices;
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