#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform Camera {
	vec3 center;
	vec3 u;
	vec3 v;
} cam;

void main() {
	vec3 p = inPosition - cam.center;
	gl_Position = vec4(dot(cam.u, p), dot(cam.v, p), dot(cross(cam.v, cam.u), p), 1.0);
	fragColor = inColor;
}