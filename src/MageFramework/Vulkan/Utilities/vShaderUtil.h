#pragma once
#include <global.h>
#include <fstream>

namespace ShaderUtil
{
	inline std::vector<char> readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}

	// Wrap the shaders in shader modules
	inline VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice logicalDevice)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module");
		}

		return shaderModule;
	}
	inline VkShaderModule createShaderModule(const std::string& filename, VkDevice logicalDevice)
	{
		return createShaderModule(readFile(filename), logicalDevice);
	}

	inline VkPipelineShaderStageCreateInfo shaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module, const char* pName)
	{
		VkPipelineShaderStageCreateInfo l_vertexShaderStageCreateInfo{};
		l_vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_vertexShaderStageCreateInfo.stage = stage;
		l_vertexShaderStageCreateInfo.module = module;
		l_vertexShaderStageCreateInfo.pName = pName;
		return l_vertexShaderStageCreateInfo;
	}

	inline void createRayTracingShaderStageInfo(VkPipelineShaderStageCreateInfo& rayTraceShaderStageInfo,
		const std::string& shaderName, VkShaderStageFlagBits stage, VkDevice& logicalDevice)
	{
		const std::string pathToRayTracingShader = "MageFramework/shaders/" + shaderName + ".spv";
		VkShaderModule rayTraceModule = ShaderUtil::createShaderModule(pathToRayTracingShader, logicalDevice);
		// Assign shader module to the appropriate stage in the pipeline
		rayTraceShaderStageInfo = ShaderUtil::shaderStageCreateInfo(stage, rayTraceModule, "main");
	}

	inline void createComputeShaderStageInfo(VkPipelineShaderStageCreateInfo& compShaderStageInfo,
		const std::string shaderName, VkShaderModule& compModule, VkDevice& logicalDevice)
	{
		const std::string pathToCompShader = "MageFramework/shaders/" + shaderName + ".comp.spv";
		// Create vert shader module
		compModule = ShaderUtil::createShaderModule(pathToCompShader, logicalDevice);
		// Assign shader module to the appropriate stage in the pipeline
		compShaderStageInfo = ShaderUtil::shaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, compModule, "main");
	}
	inline void createVertShaderStageInfo(VkPipelineShaderStageCreateInfo& vertShaderStageInfo,
		const std::string shaderName, VkShaderModule& vertModule, VkDevice& logicalDevice)
	{
		const std::string pathToVertShader = "MageFramework/Shaders/" + shaderName + ".vert.spv";
		// Create vert shader module
		vertModule = ShaderUtil::createShaderModule(pathToVertShader, logicalDevice);
		// Assign shader module to the appropriate stage in the pipeline
		vertShaderStageInfo = ShaderUtil::shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main");
	}
	inline void createFragShaderStageInfo(VkPipelineShaderStageCreateInfo& fragShaderStageInfo,
		const std::string shaderName, VkShaderModule& fragModule, VkDevice& logicalDevice)
	{
		const std::string pathToFragShader = "MageFramework/Shaders/" + shaderName + ".frag.spv";
		// Create frag shader module
		fragModule = ShaderUtil::createShaderModule(pathToFragShader, logicalDevice);
		// Assign shader module to the appropriate stage in the pipeline
		fragShaderStageInfo = ShaderUtil::shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main");
	}
	inline void createShaderStageInfos(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
		const std::string shaderName, VkShaderModule& vertModule, VkShaderModule& fragModule, VkDevice& logicalDevice)
	{
		// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules
		createVertShaderStageInfo(shaderStages[0], shaderName, vertModule, logicalDevice);
		createFragShaderStageInfo(shaderStages[1], shaderName, fragModule, logicalDevice);
	}
	inline void createShaderStageInfos_RenderToQuad(std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
		const std::string shaderName, VkShaderModule& vertModule, VkShaderModule& fragModule, VkDevice& logicalDevice)
	{
		// Reference: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules
		createVertShaderStageInfo(shaderStages[0], "screenSpaceQuad_VS", vertModule, logicalDevice);
		createFragShaderStageInfo(shaderStages[1], shaderName, fragModule, logicalDevice);
	}
}