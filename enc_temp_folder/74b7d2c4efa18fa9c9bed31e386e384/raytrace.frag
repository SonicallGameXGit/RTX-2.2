#version 130
#define PI 3.1415926536

#define SUN_COLOR vec3(1.0, 0.8, 0.6) * 5.0
#define SUN_RADIUS 0.05
#define SKY_BRIGHTNESS 0.8

#define NULL_MATERIAL Material(vec3(0.0), 0.0)
#define NULL_HIT_INFO HitInfo(false, 0.0, 0.0, vec3(0.0), NULL_MATERIAL)

in vec2 uv;

uniform vec3 playerPosition;
uniform vec3 playerRotation;
uniform vec3 sunDirection;

uniform vec2 screenResolution;

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

float random(inout int seed) {
    seed = seed * 747796405 + 2891336453;
    
    int result = ((seed >> ((seed >> 28) + 4)) ^ seed) * 277803737;
    result = (result >> 22) ^ result;

	return result / 4294967295.0;
}
float normalDistributedRandom(inout int seed) {
    float value = random(seed);

    float theta = 2.0 * 3.1415926536 * value;
    return sqrt(-2.0 * log(value)) * cos(theta);
}

vec3 randomVector(inout int seed) {
    //for(int i = 0; i < 1000; i++) {
    //    vec3 vector = vec3(normalDistributedRandom(seed), normalDistributedRandom(seed), normalDistributedRandom(seed)) * 2.0 - 1.0;
    //    float squareDistance = dot(vector, vector);
    //    if(squareDistance <= 1.0) return vector / sqrt(squareDistance);
    //}

    //return vec3(0.0);
    return normalize(vec3(normalDistributedRandom(seed), normalDistributedRandom(seed), normalDistributedRandom(seed)) * 2.0 - 1.0);
}
vec3 randomHemisphereVector(inout int seed, vec3 normal) {
    vec3 vector = randomVector(seed);
    return vector * sign(dot(vector, normal));
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

    return skyColor * SKY_BRIGHTNESS + sunColor;
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
    
    HitInfo floorHitInfo = checkBox(ray, Box(vec3(-10.0, -1.0, -10.0), vec3(20.0, 1.0, 20.0), Material(vec3(0.8), 0.1)));
    if(floorHitInfo.hit && floorHitInfo.distance < hitInfo.distance) hitInfo = floorHitInfo;

    HitInfo wallHitInfo = checkBox(ray, Box(vec3(-10.0, 0.0, -10.0), vec3(20.0, 10.0, 1.0), Material(vec3(0.9, 0.2, 0.4), 0.1)));
    if(wallHitInfo.hit && wallHitInfo.distance < hitInfo.distance) hitInfo = wallHitInfo;

    HitInfo sphereHitInfo = checkSphere(ray, Sphere(vec3(4.0, 1.0, 4.0), 1.0, Material(vec3(0.4, 0.2, 0.9), 0.99)));
    if(sphereHitInfo.hit && sphereHitInfo.distance < hitInfo.distance) hitInfo = sphereHitInfo;

    return hitInfo;
}

vec3 rayTrace(Ray ray, inout int seed) {
    vec3 color = vec3(1.0);

    for(int i = 0; i < 32; i++) {
        HitInfo hitInfo = rayCast(ray);
        if(!hitInfo.hit) return color * sky(ray);

        color *= hitInfo.material.color;
        
        vec3 diffuse = randomHemisphereVector(seed, hitInfo.normal);
        vec3 reflected = reflect(ray.direction, hitInfo.normal);

        ray.position += ray.direction * hitInfo.distance;
        ray.direction = mix(reflected, diffuse, 1.0);
    }

    return vec3(0.0);
}

vec3 denoise(Ray ray, inout int seed) {
    vec3 color;
    for(int i = 0; i < 8; i++) {
        color += rayTrace(ray, seed);
    }

    return clamp(color / 8.0, vec3(0.0), vec3(1.0));
}

out vec4 fragColor;

void main() {
    float fov = 0.8;
    float aspect = screenResolution.x / screenResolution.y;

    Ray ray = Ray(playerPosition, normalize(vec3(uv.x * aspect * fov, uv.y * fov, 1.0)));
    ray.direction.yz *= rotate(-playerRotation.x);
    ray.direction.xz *= rotate(-playerRotation.y);
    ray.direction.zy *= rotate(-playerRotation.z);

    ivec2 pixelUv = ivec2(int(uv.x * screenResolution.x * 8.0), int(uv.y * screenResolution.y * 8.0));
    int seed = int(pixelUv.y * screenResolution.x * 8.0 + pixelUv.x * 8.0 + time * 3289493.0);

    fragColor = vec4(denoise(ray, seed), 1.0);
}