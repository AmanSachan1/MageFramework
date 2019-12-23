#pragma once

#include <global.h>
#include <forward.h>
#include <Utilities\pipelineUtility.h>
#include <Utilities\commandUtility.h>
#include <Utilities\shaderUtility.h>
#include <Utilities\renderUtility.h>
#include <Utilities\generalUtility.h>

#include "Vulkan\vulkanManager.h"
#include "Vulkan\vulkanRendererBackend.h"
#include "UIManager.h"

#include "Camera.h"
#include "Scene.h"
#include "SceneElements\model.h"
#include "SceneElements\texture.h"

static constexpr unsigned int WORKGROUP_SIZE = 32;

class Renderer 
{
public:
	Renderer() = delete; // To enforce the creation of a the type of renderer we want without leaving the vulkan device, vulkan swapchain, etc as assumptions or nullptrs
	Renderer(GLFWwindow* window, VulkanManager* vulkanObject, Camera* camera, uint32_t width, uint32_t height);
	~Renderer();

	void recreate();
	void renderLoop();
		
private:
	void initialize();	
	void cleanup();

	// Render Loop Helpers
	void updateRenderState();
	void acquireNextSwapChainImage();
	void presentCurrentImageToSwapChainImage();

	// Commands
	void recordAllCommandBuffers();
	void recordGraphicsCommandBuffer(VkCommandBuffer& graphicsCmdBuffer, unsigned int frameIndex);
	void recordComputeCommandBuffer(VkCommandBuffer& ComputeCmdBuffer, unsigned int frameIndex);
		
	// Descriptors
	void setupDescriptorSets();
	void writeToAndUpdateDescriptorSets();

	// Pipelines
	void createAllPipelines();	

private:
	GLFWwindow* m_window;
	VulkanManager* m_vulkanObj;
	VulkanRendererBackend* m_rendererBackend;
	UIManager* m_UI;
	Camera* m_camera;
	Scene* m_scene;
};