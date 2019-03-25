#pragma once

#include <global.h>
#include <forward.h>
#include <utility.h>

#include "VulkanInitializers.h"
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
	Renderer(VkDevice& logicalDevice, VkPhysicalDevice& physicalDevice,
		VulkanPresentation* presentation,
		Camera* camera, uint32_t width, uint32_t height);
	~Renderer();

	void initialize();
	void recreate(uint32_t width, uint32_t height);
	void renderLoop();

	void destroyOnWindowResize();
	
	//Render Passes
	void createRenderPass();

	// Pipelines
	void createAllPipelines();
	void createGraphicsPipeline(VkPipeline& graphicsPipeline, VkRenderPass& renderPass, unsigned int subpass);

private:
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	VulkanPresentation* m_presentationObject;
	uint32_t m_windowWidth;
	uint32_t m_windowHeight;

	bool m_swapPingPongBuffers = false;
	Camera* m_camera;

	VkRenderPass m_renderPass;

	VkPipelineLayout m_graphicsPipelineLayout;
	VkPipeline m_graphicsPipeline;
};