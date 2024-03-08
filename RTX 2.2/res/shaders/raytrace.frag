#version 330
#define PI 3.1415926536

#define SUN_COLOR vec3(1.0, 0.7, 0.4) * 300.0
#define SUN_RADIUS 0.001

#define SKY_BRIGHTNESS 0.8

#define NULL_MATERIAL Material(vec3(0.0), 0.0, 0.0, 0.0, vec4(0.0), false)
#define NULL_HIT_INFO HitInfo(false, 0.0, 0.0, vec3(0.0), vec2(0.0), NULL_MATERIAL)

#define BOXES 7
#define SPHERES 2

in vec2 uv;

uniform vec3 playerPosition;
uniform vec3 playerRotation;
uniform vec3 sunDirection;

uniform vec2 screenResolution;

uniform sampler2D backFrameSampler;
uniform sampler2D albedoSampler;
uniform sampler2D normalSampler;
uniform sampler2D skyboxSampler;

uniform float random;
uniform float backFrameFactor;

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
    float glassReflect;

    vec4 uvInfo;

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
    vec2 uv;

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

float hash(inout float seed) { 
	return fract(sin(dot(vec2(seed += 0.1), vec2(12.9898, 4.1414))) * 43758.5453);
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

    vec3 normal = normalize(ray.position + ray.direction * distance - sphere.position);

    vec2 texUv = vec2(atan(normal.z, normal.x), asin(normal.y) * 2.0);
    texUv = (texUv / PI) / 2.0 + 0.5;
    texUv -= floor(texUv);

    texUv *= sphere.material.uvInfo.zw;
    texUv += sphere.material.uvInfo.xy;

    return HitInfo(distance >= 0.0, distance, -b + h, normal, texUv, sphere.material);
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

    vec3 normal = ray.position + ray.direction * tN - box.position;
    vec3 face = -sign(ray.direction) * step(t1.yzx, t1.xyz) * step(t1.zxy, t1.xyz);

    vec2 texUv;
    if(face.y != 0.0) texUv = normal.xz;
    else if(face.x != 0.0) texUv = normal.zy;
    else texUv = normal.xy;

    texUv -= floor(texUv);

    texUv *= box.material.uvInfo.zw;
    texUv += box.material.uvInfo.xy;

    return HitInfo(tN >= 0.0, tN, tF, face, texUv, box.material);
}

HitInfo rayCast(Ray ray) {
    HitInfo hitInfo = HitInfo(false, 1000000.0, 0.0, vec3(0.0), vec2(0.0), NULL_MATERIAL);
    
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

    for(int i = 0; i < 64; i++) {
        HitInfo hitInfo = rayCast(ray);
        if(!hitInfo.hit) return color * sky(ray);

        color *= hitInfo.material.color;

        if(length(hitInfo.uv) > 0.0) {
            color *= texture2D(albedoSampler, hitInfo.uv).rgb;

            vec3 texturedNormal = texture2D(normalSampler, hitInfo.uv).rgb * 2.0 - 1.0;
            vec3 tangent = hitInfo.normal;
            tangent.yx *= rotate(-90.0);

            vec3 bitangent = hitInfo.normal;
            bitangent.yz *= rotate(-90.0);

            mat3 tangentMatrix = mat3(
                tangent.x, bitangent.x, hitInfo.normal.x,
                tangent.y, bitangent.y, -hitInfo.normal.y,
                tangent.z, bitangent.z, hitInfo.normal.z
            );

            hitInfo.normal = normalize(-texturedNormal * tangentMatrix);
        }

        if(hitInfo.material.emissive) return color;
        
        float fresnel = pow(clamp(1.0 - dot(hitInfo.normal, -ray.direction), 0.0, 1.0), 1.0 + hitInfo.material.glass);
        float reflectChance = hash(seed) * (fresnel + hitInfo.material.glassReflect);
        float sunDirectChance = hash(seed);
        
        if(hitInfo.material.glass > 0.0 && reflectChance < 0.5) {
            ray.position += ray.direction * (hitInfo.farDistance - 0.001);
        
            vec3 refracted = refract(ray.direction, hitInfo.normal, 1.0 - hitInfo.material.glass);
            ray.direction = randomSphereDirection(seed);
            ray.direction *= sign(dot(ray.direction, -hitInfo.normal));
            ray.direction = mix(refracted, ray.direction, hitInfo.material.diffuse);
        } else {
            ray.position += ray.direction * (hitInfo.distance - 0.001);
            
            vec3 reflected = reflect(ray.direction, hitInfo.normal);
            ray.direction = randomSphereDirection(seed);
            ray.direction *= sign(dot(ray.direction, hitInfo.normal));
            ray.direction = mix(reflected, ray.direction, hitInfo.material.diffuse);
        }

        ray.direction = normalize(ray.direction);
    }

    return vec3(0.0);
}

vec3 render(Ray ray, inout float seed, in int raysPerPixel) {
    vec3 color;
    for(int i = 0; i < raysPerPixel; i++) {
        color += rayTrace(ray, seed);
        seed += 394.392 / (float(i) + 1.0);
    }

    return clamp(color / float(raysPerPixel), vec3(0.0), vec3(1.0));
}

out vec4 fragColor;

void main() {
    float seed = (uv.x / (screenResolution.x / screenResolution.y) + uv.y) * 492.38 + random + (playerRotation.x + playerRotation.y + playerRotation.z) / 2.0;

    Ray ray = Ray(vec3(0.0), normalize(vec3(vec2(uv.x * (screenResolution.x / screenResolution.y), uv.y) * tan(radians(fov) / 2.0), 1.0)));

    vec2 randomPoint = randomSphereDirection(seed).xy * dofBlurSize;

    Ray focusRay = Ray(playerPosition, vec3(0.0, 0.0, 1.0));
    focusRay.direction.yz *= rotate(-playerRotation.x);
    focusRay.direction.xz *= rotate(-playerRotation.y);
    focusRay.direction.zy *= rotate(-playerRotation.z);
    
    HitInfo focusHitInfo = rayCast(focusRay);

    vec3 focusPoint = ray.direction * (focusHitInfo.hit ? focusHitInfo.distance : dofFocusDistance);

    ray.position = vec3(randomPoint * (focusHitInfo.hit ? focusHitInfo.distance : dofFocusDistance), 0.0);
    ray.direction = normalize(focusPoint - ray.position);

    ray.position.yx *= rotate(-playerRotation.z);
    ray.position.yz *= rotate(-playerRotation.x);
    ray.position.xz *= rotate(-playerRotation.y);

    ray.direction.yx *= rotate(-playerRotation.z);
    ray.direction.yz *= rotate(-playerRotation.x);
    ray.direction.xz *= rotate(-playerRotation.y);

    ray.position += playerPosition;

    fragColor = vec4(render(ray, seed, 128), 1.0);
    
    vec4 backFrameColor = texture2D(backFrameSampler, uv / 2.0 + 0.5);
    if(backFrameColor.a > 0.0)
        fragColor.rgb = mix(backFrameColor.rgb, fragColor.rgb, backFrameFactor);
}