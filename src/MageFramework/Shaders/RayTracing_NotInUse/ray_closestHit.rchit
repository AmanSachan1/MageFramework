#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};
layout(location = 0) rayPayloadInNV RayPayload rayPayload;
layout(location = 2) rayPayloadNV bool shadowed;
hitAttributeNV vec3 hitAttribs;

layout(binding = 0, set = 0) uniform accelerationStructureNV topLevelAS;
layout(binding = 2, set = 0) uniform CameraUBO
{
    mat4 view;
    mat4 proj;
    mat4 viewInverse;
    mat4 projInverse;
    vec4 eye;
    vec2 tanFovBy2;
} cam;
layout(binding = 3, set = 0) uniform LightsUBO
{
    vec4 lightPos;
} lights;
layout(binding = 4, set = 0) buffer Vertices { vec4 v[]; } vertices;
layout(binding = 5, set = 0) buffer Indices { uint i[]; } indices;

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec2 uv;
};

Vertex unpack(uint index)
{
	vec4 data0 = vertices.v[3 * index + 0];
	vec4 data1 = vertices.v[3 * index + 1];

	Vertex v;
	v.pos = data0.xyz;
	v.normal = vec3(data0.w, data1.x, data1.y);
	v.uv = vec2(data1.zw);
	return v;
}

void main() 
{
    ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID + 0], 
						indices.i[3 * gl_PrimitiveID + 1], 
						indices.i[3 * gl_PrimitiveID + 2]);

	Vertex v0 = unpack(index.x);
	Vertex v1 = unpack(index.y);
	Vertex v2 = unpack(index.z);

    const vec3 barycentricCoords = vec3(1.0f - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);
    vec3 normal = normalize(v0.normal * barycentricCoords.x + 
							v1.normal * barycentricCoords.y + 
							v2.normal * barycentricCoords.z);

	// Basic lighting
	vec3 vertexColor = vec3(1.0f, 0.0f, 1.0f); 
	vec3 lightVector = normalize(lights.lightPos.xyz);
	float dot_product = max(dot(lightVector, normal), 0.6);
	rayPayload.color = vertexColor * vec3(dot_product);
	rayPayload.distance = gl_RayTmaxNV;
	rayPayload.normal = normal;

	// Objects with full white vertex color are treated as reflectors
	rayPayload.reflector = 0.0f; // Do a material or color based test for this //1 for reflective, 0 otherwise 

	// Shadow casting
	{
		vec3 rayOrigin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
		vec3 rayDirection = lightVector;
		shadowed = true; 

		// Offset indices to match shadow hit/miss index
		const uint rayFlags = gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsOpaqueNV|gl_RayFlagsSkipClosestHitShaderNV;
		const uint cullMask = 0xFF;
		const uint sbtRecordOffset = 1;
		const uint sbtRecordStride = 0;
		const uint missIndex = 1;
		const float tmin = 0.001f;
		const float tmax = 100.0f;
		const int payloadLocation = 2;
		traceNV(topLevelAS, rayFlags, cullMask, 
			sbtRecordOffset, sbtRecordStride, missIndex,
			rayOrigin, tmin, rayDirection, tmax, payloadLocation);
	
		if (shadowed) 
		{
			rayPayload.color *= 0.3;
		}
	}
}