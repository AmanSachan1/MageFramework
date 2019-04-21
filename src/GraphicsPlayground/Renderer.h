#pragma once

#include <global.h>
#include <forward.h>

#include <Utilities\vulkanInitializers.h>
#include <Utilities\pipelineUtility.h>
#include <Utilities\commandUtility.h>
#include <Utilities\imageUtility.h>
#include <Utilities\shaderUtility.h>
#include <Utilities\renderUtility.h>

#include "vulkanDevices.h"
#include "vulkanPresentation.h"

#include "Camera.h"
#include "Scene.h"
#include "Model.h"

static constexpr unsigned int WORKGROUP_SIZE = 32;

class Renderer 
{
public:
	Renderer() = delete; // To enforce the creation of a the type of renderer we want without leaving the vulkan device, vulkan swapchain, etc as assumptions or nullptrs
	Renderer(GLFWwindow* window, VulkanDevices* devices, VulkanPresentation* presentation, Camera* camera, uint32_t width, uint32_t height);
	~Renderer();

	void initialize();
	void recreate();
	void cleanup();

	void renderLoop();
	
	// Commands
	void recordAllCommandBuffers();
	void recordGraphicsCommandBuffer(VkCommandBuffer& graphicsCmdBuffer, VkFramebuffer& frameBuffer);
	void recordComputeCommandBuffer(VkCommandBuffer& ComputeCmdBuffer);
	void createCommandPoolsAndBuffers();
	
	// Rendering Setup
	void createFrameBuffers();
	void createRenderPass();

	// Pipelines
	void createAllPipelines();
	void createGraphicsPipeline(VkPipeline& graphicsPipeline, VkRenderPass& renderPass, unsigned int subpass);

	// Helpers
	void setResizeFlag(bool value) { m_resizeFrameBuffer = value; }

private:
	GLFWwindow * m_window;
	VulkanDevices* m_devices; // manages both the logical device (VkDevice) and the physical Device (VkPhysicalDevice)
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	VulkanPresentation* m_presentationObject;
	uint32_t m_windowWidth;
	uint32_t m_windowHeight;

	bool m_swapPingPongBuffers = false;
	bool m_resizeFrameBuffer = false;
	Camera* m_camera;

	std::vector<VkFramebuffer> m_frameBuffers;
	VkRenderPass m_renderPass;

	// Pipeline Setup
	VkPipelineLayout m_graphicsPipelineLayout;
	VkPipeline m_graphicsPipeline;

	// Command Buffers
	std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
	std::vector<VkCommandBuffer> m_computeCommandBuffers;

	// Memory Pools
	VkCommandPool m_graphicsCommandPool;
	VkCommandPool m_computeCommandPool;
};