#version 120

struct Material {
	bool chtoto;
};
struct Sphere {
	vec3 position;
};
struct Box {
	bool chtoto;
};

uniform Box[16] boxes;
uniform Sphere[16] spheres;

void main() {
	Sphere sphere;
	sphere.position = vec3(0.0);
}