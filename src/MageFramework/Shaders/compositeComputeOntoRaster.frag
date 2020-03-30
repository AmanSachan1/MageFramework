#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform sampler2D computeTexSampler;
layout(set = 0, binding = 1) uniform sampler2D renderedGeomTexSampler;

layout(location = 0) in vec2 f_uv;

layout(location = 0) out vec4 outColor;

void main()
{
	vec3 color = texture(renderedGeomTexSampler, f_uv).rgb * vec3(0.7f) + texture(computeTexSampler, f_uv).rgb * vec3(0.3f);
	outColor = vec4(color, 1.0f);
}