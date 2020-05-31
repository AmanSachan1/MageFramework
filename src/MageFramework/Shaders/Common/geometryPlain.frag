#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform MaterialUBO
{
	int activeTextureFlags;
	int alphaMode;
	float alphaCutoff;
	float metallicFactor;
	float roughnessFactor;
	vec4 baseColorFactor;
};
layout(set = 1, binding = 2) uniform sampler2D baseTexSampler;
layout(set = 1, binding = 3) uniform sampler2D normalTexSampler;
layout(set = 1, binding = 4) uniform sampler2D metallicRoughnessTexSampler;
layout(set = 1, binding = 5) uniform sampler2D emissiveTexSampler;
layout(set = 1, binding = 6) uniform sampler2D occlusionTexSampler;

layout(location = 0) in vec2 f_uv;
layout(location = 1) in vec3 f_nor;

layout(location = 0) out vec4 outColor;

void main() 
{
	vec3 baseColor = texture(baseTexSampler, f_uv).rgb;
	vec3 normal = vec3(0.0f);
	if( bitfieldExtract(activeTextureFlags, 1, 1) == 1 )
	{
		normal = texture(normalTexSampler, f_uv).rgb;
	}
	else
	{
		normal = f_nor;
	}
	 
	outColor = vec4(baseColor, 1.0); // baseColorFactor;//
	//outColor = vec4((baseColor + normal)*0.5f, 1.0);
}