#version 120
#define SUN_COLOR vec3(1.0, 0.8, 0.6) * 10.0
#define SUN_RADIUS 0.01
#define NULL_MATERIAL Material(vec3(0.0), 0.0)
#define NULL_HIT_INFO HitInfo(false, 0.0, 0.0, vec3(0.0), NULL_MATERIAL)

varying vec2 uv;

uniform vec3 playerPosition;
uniform vec3 playerRotation;
uniform vec3 sunDirection;

uniform float aspect;
uniform float time;

struct Ray {
    vec3 position;
    vec3 direction;
};

struct Material {
    vec3 color;
    float diffuse;
};
struct Sphere {
    vec3 position;
    float radius;

    Material material;
};
struct Box {
    vec3 position;
    vec3 size;

    Material material;
};

struct HitInfo {
    bool hit;

    float distance;
    float farDistance;

    vec3 normal;

    Material material;
};

float random(vec2 uv) { 
	return fract(sin(dot(uv, vec2(12.9898, 4.1414))) * 43758.5453);
}

mat2 rotate(float angle) {
    float radAngle = radians(angle);

    float sin = sin(radAngle);
    float cos = cos(radAngle);

    return mat2(cos, -sin, sin, cos);
}

vec3 sky(Ray ray) {
    vec3 skyColor = mix(vec3(0.666), vec3(0.7, 0.8, 1.0), ray.direction.y / 2.0 + 0.5);
    vec3 sunColor = mix(vec3(0.0), SUN_COLOR, pow(clamp(dot(ray.direction, normalize(sunDirection)), 0.0, 1.0), 1.0 / SUN_RADIUS));

    return skyColor + sunColor;
}

HitInfo checkSphere(Ray ray, Sphere sphere) {
    vec3 delta = ray.position - sphere.position;

    float b = dot(delta, ray.direction);
    float h = b * b - (dot(delta, delta) - sphere.radius * sphere.radius);
    if(h < 0.0) return NULL_HIT_INFO;
    h = sqrt(h);

    float distance = -b - h;
    return HitInfo(distance >= 0.0, distance, -b + h, normalize(ray.position + ray.direction * distance - sphere.position), sphere.material);
}
HitInfo checkBox(Ray ray, Box box) {
    vec3 delta = ray.position - box.position - box.size / 2.0;

    vec3 m = 1.0 / ray.direction;
    vec3 n = m * delta;
    vec3 k = abs(m) * box.size / 2.0;
    vec3 t1 = -n -k;
    vec3 t2 = -n + k;

    float tN = max(max(t1.x, t1.y), t1.z);
    float tF = min(min(t2.x, t2.y), t2.z);

    if(tN > tF || tF < 0.0) return NULL_HIT_INFO;
    return HitInfo(tN >= 0.0, tN, tF, -sign(ray.direction) * step(t1.yzx, t1.xyz) * step(t1.zxy, t1.xyz), box.material);
}

HitInfo rayCast(Ray ray) {
    HitInfo hitInfo = HitInfo(false, 1000000.0, 0.0, vec3(0.0), NULL_MATERIAL);
    
    HitInfo floorHitInfo = checkBox(ray, Box(vec3(-10.0, -1.0, -10.0), vec3(20.0, 1.0, 20.0), Material(vec3(0.8), 0.3)));
    if(floorHitInfo.hit && floorHitInfo.distance < hitInfo.distance) hitInfo = floorHitInfo;

    HitInfo sphereHitInfo = checkSphere(ray, Sphere(vec3(4.0, 1.0, 4.0), 1.0, Material(vec3(1.0, 0.1, 0.2), 0.8)));
    if(sphereHitInfo.hit && sphereHitInfo.distance < hitInfo.distance) hitInfo = sphereHitInfo;

    return hitInfo;
}

vec3 rayTrace(Ray ray) {
    vec3 color = vec3(1.0);

    for(int i = 0; i < 32; i++) {
        HitInfo hitInfo = rayCast(ray);
        if(!hitInfo.hit) return color * sky(ray);

        color *= hitInfo.material.color;

        ray.position += ray.direction * (hitInfo.distance - 0.001);
        ray.direction = reflect(ray.direction, hitInfo.normal);
    }

    return vec3(0.0);
}

void main() {
    float fov = 0.8;
    Ray ray = Ray(playerPosition, normalize(vec3(uv.x * aspect * fov, uv.y * fov, 1.0)));

    ray.direction.yz *= rotate(-playerRotation.x);
    ray.direction.xz *= rotate(-playerRotation.y);
    ray.direction.zy *= rotate(-playerRotation.z);

    gl_FragColor = vec4(rayTrace(ray), 1.0);
}