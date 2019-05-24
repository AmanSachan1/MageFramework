#pragma once

#include <global.h>
#include <forward.h>

#include <Utilities\pipelineUtility.h>
#include <Utilities\commandUtility.h>
#include <Utilities\shaderUtility.h>
#include <Utilities\renderUtility.h>
#include <Utilities\generalUtility.h>

#include "vulkanDevices.h"
#include "vulkanPresentation.h"

#include "Camera.h"
#include "Scene.h"
#include "Model.h"
#include "Texture.h"

static constexpr unsigned int WORKGROUP_SIZE = 32;

enum class RenderAPI { VULKAN, DX12 };

struct RendererOptions 
{
	RenderAPI renderAPI;
	bool MSAA; // Geometry Anti-Aliasing
	bool enableSampleRateShading; // Shading Anti-Aliasing (enables processing more than one sample per fragment)
	float minSampleShading; // value between 0.0f and 1.0f --> closer to one is smoother
	bool enableAnisotropy; // Anisotropic filtering -- image sampling will use anisotropic filter
	float anisotropy; //controls level of anisotropic filtering
};

class Renderer 
{
public:
	Renderer() = delete; // To enforce the creation of a the type of renderer we want without leaving the vulkan device, vulkan swapchain, etc as assumptions or nullptrs
	Renderer(GLFWwindow* window, RendererOptions rendererOptions, VulkanDevices* devices, VulkanPresentation* presentation, Camera* camera, uint32_t width, uint32_t height);
	~Renderer();

	void initialize();
	void recreate();
	void cleanup();

	void renderLoop();
	
	// Commands
	void recordAllCommandBuffers();
	void recordGraphicsCommandBuffer(VkCommandBuffer& graphicsCmdBuffer, VkFramebuffer& frameBuffer, unsigned int frameIndex);
	void recordComputeCommandBuffer(VkCommandBuffer& ComputeCmdBuffer, unsigned int frameIndex);
	void createCommandPoolsAndBuffers();
	
	// Rendering Setup
	void createFrameBuffers();
	void createRenderPass();
	void createDepthResources();
	void setupMSAA();

	// Descriptors
	void setupDescriptorSets();

	// Pipelines
	void createAllPipelines();
	void createGraphicsPipeline(VkPipeline& graphicsPipeline, VkPipelineLayout graphicsPipelineLayout, VkRenderPass& renderPass, unsigned int subpass);
	void createComputePipeline(VkPipeline& computePipeline, VkPipelineLayout computePipelineLayout, const std::string &pathToShader);
	void createPostProcessPipelines(VkRenderPass& renderPass, unsigned int subpass);

	// Helpers
	void setResizeFlag(bool value) { m_resizeFrameBuffer = value; }

private:
	GLFWwindow* m_window;
	VulkanDevices* m_devices; // manages both the logical device (VkDevice) and the physical Device (VkPhysicalDevice)
	VkDevice m_logicalDevice;
	VkPhysicalDevice m_physicalDevice;
	VulkanPresentation* m_presentationObject;
	VkQueue	m_graphicsQueue;
	VkQueue	m_computeQueue;
	uint32_t m_windowWidth;
	uint32_t m_windowHeight;

	bool m_swapPingPongBuffers = false;
	bool m_resizeFrameBuffer = false;
	Camera* m_camera;

	Model* m_model;

	Texture* computeTexture;
	Texture* currentFrameTexture;
	Texture* previousFrameTexture;

	std::vector<VkFramebuffer> m_frameBuffers;
	VkRenderPass m_renderPass;
	VkImage m_depthImage;
	VkDeviceMemory m_depthImageMemory;
	VkImageView m_depthImageView;

	// For MSAA
	VkImage m_MSAAcolorImage;
	VkDeviceMemory m_MSAAcolorImageMemory;
	VkImageView m_MSAAcolorImageView;

	// Pipeline Setup
	VkPipelineLayout m_graphicsPipelineLayout;
	VkPipeline m_graphicsPipeline;

	VkPipelineLayout m_computePipelineLayout;
	VkPipeline m_computePipeline;

	VkPipelineCache m_postProcessPipeLineCache;
	VkPipelineLayout m_postProcess_ToneMap_PipelineLayout;
	VkPipelineLayout m_postProcess_TXAA_PipelineLayout;
	VkPipeline m_postProcess_ToneMap_PipeLine;
	VkPipeline m_postProcess_TXAA_PipeLine;

	// Command Buffers
	std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
	std::vector<VkCommandBuffer> m_computeCommandBuffers;

	// Memory Pools
	VkCommandPool m_graphicsCommandPool;
	VkCommandPool m_computeCommandPool;

	// Descriptor Pools, sets, and layouts
	VkDescriptorPool descriptorPool;
	
	//graphics
	VkDescriptorSetLayout m_DSL_graphics;
	std::vector<VkDescriptorSet> m_DS_graphics;	

	//compute
	VkDescriptorSetLayout m_DSL_compute;
	std::vector<VkDescriptorSet> m_DS_compute;

	//Tone Map
	VkDescriptorSetLayout m_DSL_toneMap;
	std::vector<VkDescriptorSet> m_DS_toneMap;

	// Descriptor Sets for pingPonged TXAA
	VkDescriptorSetLayout m_DSL_TXAA;
	std::vector<VkDescriptorSet> m_DS_TXAA;
};