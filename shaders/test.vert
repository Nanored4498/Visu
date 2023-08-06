#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform Camera {
	vec3 center;
	vec3 u;
	vec3 v;
} cam;

#define PI 3.14159265359

void main() {
	vec3 p = inPosition - cam.center;
	vec3 cam_w = normalize(cross(cam.v, cam.u));
	gl_Position = vec4(dot(cam.u, p), -dot(cam.v, p), atan(dot(cam_w, p)) / PI + .5, 1.0);
	fragColor = inColor * (.5 + .5*dot(cam_w, p));
}