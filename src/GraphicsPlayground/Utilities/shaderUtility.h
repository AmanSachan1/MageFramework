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

	inline VkPipelineShaderStageCreateInfo shaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module,	const char* pName)
	{
		VkPipelineShaderStageCreateInfo l_vertexShaderStageCreateInfo{};
		l_vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		l_vertexShaderStageCreateInfo.stage = stage;
		l_vertexShaderStageCreateInfo.module = module;
		l_vertexShaderStageCreateInfo.pName = pName;
		return l_vertexShaderStageCreateInfo;
	}
}
