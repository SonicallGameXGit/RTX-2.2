#version 330
#define PI 3.1415926536

#define SUN_COLOR vec3(1.0, 0.8, 0.6) * 5.0
#define SUN_RADIUS 0.0025
#define SKY_BRIGHTNESS 0.8

#define NULL_MATERIAL Material(vec3(0.0), 0.0, 0.0, 0.0, false)
#define NULL_HIT_INFO HitInfo(false, 0.0, 0.0, vec3(0.0), NULL_MATERIAL)

#define BOXES 13
#define SPHERES 4

in vec2 uv;

uniform vec3 playerPosition;
uniform vec3 playerRotation;
uniform vec3 sunDirection;

uniform vec2 screenResolution;

uniform sampler2D lastFrameSampler;
uniform sampler2D skyboxSampler;

uniform float randomOffset;
uniform float denoiseFactor;

uniform float dofFocusDistance;
uniform float dofBlurSize;
uniform float fov;

struct Ray {
    vec3 position;
    vec3 direction;
};

struct Material {
    vec3 color;

    float diffuse;
    float glass;
    float glassReflectivity;

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

uniform Box boxes[BOXES];
uniform Sphere spheres[SPHERES];

mat2 rotate(float angle) {
    float radAngle = radians(angle);

    float sin = sin(radAngle);
    float cos = cos(radAngle);

    return mat2(cos, -sin, sin, cos);
}

mat4 lookAt(vec3 position, vec3 target, vec3 up) {
    vec3 front = normalize(target - position);
    vec3 right = normalize(cross(up, front));

    up = cross(front, right);

    return mat4(
        right.x, up.x, front.x, position.x,
        right.y, up.y, front.y, position.y,
        right.z, up.z, front.z, position.z,
        0.0, 0.0, 0.0, 1.0
    );
}

float hash(inout float seed) { 
	return fract(sin(dot(vec2(seed += 0.8), vec2(12.9898, 4.1414))) * 43758.5453);
}

vec2 hash2(inout float seed) {
    return vec2(hash(seed), hash(seed));
}

vec3 hash3(inout float seed) {
    return vec3(hash(seed), hash(seed), hash(seed));
}

vec3 randomSphereDirection(inout float seed) {
    vec2 h = hash2(seed) * vec2(2.0, 6.28318530718) - vec2(1,0);
    float phi = h.y;
	return vec3(sqrt(1.0 -h.x * h.x) * vec2(sin(phi), cos(phi)), h.x);
}

vec3 randomHemisphereDirection(const vec3 n, inout float seed) {
	vec3 dr = randomSphereDirection(seed);
	return dot(dr, n) * dr;
}


vec3 sky(Ray ray) {
    //vec3 skyColor = mix(vec3(0.666), vec3(0.7, 0.8, 1.0), ray.direction.y / 2.0 + 0.5);
    vec2 skyUv = vec2(atan(ray.direction.z, ray.direction.x), asin(ray.direction.y) * 2.0);
    skyUv = (skyUv / PI) / 2.0 + 0.5;

    vec3 skyColor = texture2D(skyboxSampler, skyUv).rgb;
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
    
    for(int i = 0; i < BOXES; i++) {
        HitInfo boxHitInfo = checkBox(ray, boxes[i]);
        if(boxHitInfo.hit && boxHitInfo.distance < hitInfo.distance)
            hitInfo = boxHitInfo;
    }
    for(int i = 0; i < SPHERES; i++) {
        HitInfo sphereHitInfo = checkSphere(ray, spheres[i]);
        if(sphereHitInfo.hit && sphereHitInfo.distance < hitInfo.distance)
            hitInfo = sphereHitInfo;
    }

    return hitInfo;
}

vec3 rayTrace(Ray ray, inout float seed) {
    vec3 color = vec3(1.0);

    for(int i = 0; i < 32; i++) {
        HitInfo hitInfo = rayCast(ray);
        if(!hitInfo.hit) return color * sky(ray);
        //
        //color *= hitInfo.material.color;
        //
        // Kakaya-to dich, neponyatnaya dlya moego mosga
        //ray.position += ray.direction * (hitInfo.distance - 0.001);
        //ray.direction = normalize(mix(sunDirection * 10.0, hash3(seed) * 2.0 - 1.0, 0.9));
        //
        //hitInfo = rayCast(ray);
        //if(hitInfo.hit) color *= dot(hitInfo.normal, ray.direction);
        //return color;

        color *= hitInfo.material.color;
        if(hitInfo.material.emissive) return color;
        
        float fresnel = pow(clamp(1.0 - dot(hitInfo.normal, -ray.direction), 0.0, 1.0), 1.0 + hitInfo.material.glass);
        float reflectChance = hash(seed) * (fresnel + hitInfo.material.glassReflectivity);
        
        if(hitInfo.material.glass > 0.0 && reflectChance < 0.5) {
            ray.position += ray.direction * (hitInfo.farDistance - 0.001);
        
            vec3 refracted = refract(ray.direction, hitInfo.normal, 1.0 - hitInfo.material.glass);
            ray.direction = hash3(seed) * 2.0 - 1.0;
            ray.direction *= sign(dot(ray.direction, -hitInfo.normal));
            ray.direction = mix(refracted, ray.direction, hitInfo.material.diffuse);
        } else {
            ray.position += ray.direction * (hitInfo.distance - 0.001);
        
            vec3 reflected = reflect(ray.direction, hitInfo.normal);
            ray.direction = hash3(seed) * 2.0 - 1.0;
            ray.direction *= sign(dot(ray.direction, hitInfo.normal));
            ray.direction = mix(reflected, ray.direction, hitInfo.material.diffuse);
        }
    }

    return vec3(0.0);
}

vec3 denoise(Ray ray, inout float seed) {
    vec3 color;
    for(int i = 0; i < 3; i++)
        color += rayTrace(ray, seed);

    return clamp(color / 3.0, vec3(0.0), vec3(1.0));
}

out vec4 fragColor;

void main() {
    float seed = (uv.x / (screenResolution.x / screenResolution.y) + uv.y) * 392.38 + 3.43121412313 + denoiseFactor;

    Ray ray = Ray(vec3(0.0), normalize(vec3(vec2(uv.x * (screenResolution.x / screenResolution.y), uv.y) * tan(radians(fov) / 2.0), 1.0)));

    vec2 randomPoint = randomSphereDirection(seed).xy * dofBlurSize;

    vec3 focalPoint = ray.direction * dofFocusDistance;
    vec3 finalDirection = normalize(focalPoint - vec3(randomPoint, 0.0));

    ray.position = vec3(randomPoint, 0.0);
    ray.direction = finalDirection;

    ray.position.yz *= rotate(-playerRotation.x);
    ray.position.xz *= rotate(-playerRotation.y);
    ray.position.zy *= rotate(-playerRotation.z);

    ray.direction.yz *= rotate(-playerRotation.x);
    ray.direction.xz *= rotate(-playerRotation.y);
    ray.direction.zy *= rotate(-playerRotation.z);

    ray.position += playerPosition;

    fragColor = vec4(denoise(ray, seed), 1.0);

    vec4 lastFrameColor = texture2D(lastFrameSampler, uv / 2.0 + 0.5);
    if(lastFrameColor.a != 0.0)
        fragColor = mix(lastFrameColor, fragColor, denoiseFactor);
}