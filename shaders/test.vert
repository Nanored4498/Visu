#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV;

layout(binding = 0) uniform Camera {
	vec3 center;
	vec3 u, v, w;
} cam;

#define PI 3.14159265359

void main() {
	vec3 p = inPosition - cam.center;
	gl_Position = vec4(dot(cam.u, p), -dot(cam.v, p), atan(dot(cam.w, p)) / PI + .5, 1.0);
	outNormal = inNormal;
	outUV = inUV;
}