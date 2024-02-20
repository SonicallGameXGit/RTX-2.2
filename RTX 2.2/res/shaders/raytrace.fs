#version 120
#define SUN_COLOR vec3(1.0, 0.8, 0.6) * 10.0
#define SUN_DIRECTION normalize(vec3(0.3, 0.4, 0.3))
#define SUN_RADIUS 0.01
#define PI 3.1415926536

varying vec2 uv;

uniform vec3 playerPosition;
uniform vec3 playerRotation;

uniform float aspect;
uniform float time;

struct Ray {
    vec3 position;
    vec3 direction;
};

struct Material {
    vec3 color;
    float diffuse;

    bool emissive;
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
	return fract(sin(dot(uv + time * 10.0, vec2(12.9898, 4.1414))) * 43758.5453);
}

mat2 rotate(float angle) {
    float radAngle = radians(angle);

    float sin = sin(radAngle);
    float cos = cos(radAngle);

    return mat2(cos, -sin, sin, cos);
}

vec3 sky(Ray ray) {
    vec3 skyColor = mix(vec3(0.666), vec3(0.7, 0.8, 1.0), ray.direction.y / 2.0 + 0.5);
    vec3 sunColor = mix(vec3(0.0), SUN_COLOR, pow(clamp(dot(ray.direction, SUN_DIRECTION), 0.0, 1.0), 1.0 / SUN_RADIUS));

    return skyColor + sunColor;
}

HitInfo checkSphere(Ray ray, Sphere sphere) {
    vec3 delta = ray.position - sphere.position;

    float b = dot(delta, ray.direction);
    float h = b * b - (dot(delta, delta) - sphere.radius * sphere.radius);
    if(h < 0.0) return HitInfo(false, 0.0, 0.0, vec3(0.0), Material(vec3(0.0), 0.0, false));
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

    if(tN > tF || tF < 0.0) return HitInfo(false, 0.0, 0.0, vec3(0.0), Material(vec3(0.0), 0.0, false));
    return HitInfo(tN >= 0.0, tN, tF, -sign(ray.direction) * step(t1.yzx, t1.xyz) * step(t1.zxy, t1.xyz), box.material);
}

HitInfo raycast(Ray ray) {
    HitInfo nearestInfo = HitInfo(false, 1000000.0, 0.0, vec3(0.0), Material(vec3(0.0), 0.0, false));

    Sphere sphere = Sphere(vec3(0.0, 0.0, 6.0), 1.0, Material(vec3(0.9, 0.0, 0.0), 0.0, false));
    HitInfo sphereInfo = checkSphere(ray, sphere);

    if(sphereInfo.hit && sphereInfo.distance < nearestInfo.distance) nearestInfo = sphereInfo;

    Box box = Box(vec3(-10.0, -1.2, -10.0), vec3(20.0, 0.2, 20.0), Material(vec3(0.8), 0.99, false));
    HitInfo boxInfo = checkBox(ray, box);

    if(boxInfo.hit && boxInfo.distance < nearestInfo.distance) nearestInfo = boxInfo;

    return nearestInfo;
}

vec3 raytrace(Ray ray, float randomOffset) {
    vec3 color = vec3(1.0);

    for(int i = 0; i < 64; i++) {
        HitInfo raycastInfo = raycast(ray);
        if(!raycastInfo.hit) return color * sky(ray);

        color *= raycastInfo.material.color;
        if(raycastInfo.material.emissive) return color;

        ray.position += ray.direction * (raycastInfo.distance - 0.001);

        float theta = 2.0 * PI * random(ray.direction.xz + ray.direction.y + randomOffset);
        float phi = PI * random(ray.direction.xy + ray.direction.z + randomOffset);

        vec3 diffuse = vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
        diffuse = normalize(diffuse);

        ray.direction = mix(reflect(ray.direction, raycastInfo.normal), diffuse, raycastInfo.material.diffuse);
    }

    return vec3(0.0);
}

void main() {
    Ray ray = Ray(playerPosition, normalize(vec3(uv.x * aspect, uv.y, 1.0)));
    ray.direction.yz *= rotate(-playerRotation.x);
    ray.direction.xz *= rotate(-playerRotation.y);
    ray.direction.zy *= rotate(-playerRotation.z);

    gl_FragColor.a = 1.0;
    for(int i = 0; i < 32; i++) {
        gl_FragColor.rgb += raytrace(ray, i);
    }

    gl_FragColor.rgb /= 32.0;
}