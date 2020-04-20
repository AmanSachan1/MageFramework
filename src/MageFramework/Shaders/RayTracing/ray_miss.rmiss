#version 460
#extension GL_NV_ray_tracing : require

struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};
layout(location = 0) rayPayloadInNV RayPayload rayPayload;

void main() 
{
	rayPayload.color = vec3(0.412f, 0.796f, 1.0f);
	rayPayload.distance = -1.0f;
	rayPayload.normal = vec3(0.0f);
	rayPayload.reflector = 0.0f;
}