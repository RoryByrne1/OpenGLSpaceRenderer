#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <glm/glm/glm.hpp>

struct PointLight {
	glm::vec3 position;
	glm::vec3 color;
};

struct Material {
	glm::vec3  diffuse;
	glm::vec3  specular;
	float glossiness;
	float reflection;
	float refraction;
	float ior;
};

struct Sphere {
	glm::vec3 position;
	float radius;
	Material material;
};

#endif