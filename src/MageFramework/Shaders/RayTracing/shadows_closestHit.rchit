#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require
#include "global.glsl"

layout(location = 2) rayPayloadInNV bool shadowed;

void main()
{
	shadowed = true;
}