#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 1) uniform sampler2D modelTexSampler;
//layout(set = 0, binding = 2) uniform sampler2D computeTexSampler;

layout(location = 0) in vec4 f_color;
layout(location = 1) in vec2 f_uv;
layout(location = 2) in vec3 f_nor;

layout(location = 0) out vec4 outColor;

void main() 
{
	outColor = vec4(texture(modelTexSampler, f_uv).rgb, 1.0);
}