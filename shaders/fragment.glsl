#version 440 core

const float PI = 3.1415926;

in vec2 vUV;
out vec4 FragColour;

uniform vec2 uResolution;

uniform vec3 uCameraPosition;
uniform vec3 uCameraForward;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;

uniform float uGamma;
uniform uint uMaxBounces;
uniform uint uSamplesPerPixel;
uniform uint uFrame;

layout(rgba32f, binding = 0) uniform image2D uAccumulatedImage;

uniform sampler2D uSkyboxTexture;
uniform int uHasSkybox;
uniform float uSkyboxExposure;

uniform vec3 uSunDirection;
uniform vec3 uSunColour;
uniform float uSunIntensity;
uniform float uSunFocus;

struct Ray {
	vec3 origin;
	vec3 dir;
};

const int FLAG_CHECKERBOARD = 1;

struct Material {
	vec3 colour;
	float smoothness;

	vec3 emissionColour;
	float emissionStrength;

	vec3 specularColour;
	int flag;

	float specularProbability;
	float _pad0;
	float _pad1;
	float _pad2;
};

struct Sphere {
	vec3 position;
	float radius;

	Material material;
};

struct Plane {
	vec3 position;
	float _pad0;

	vec3 normal;
	float _pad1;

	Material material;
};

struct Quad {
	vec3 position;
	float width;

	vec3 normal;
	float height;
	
	vec3 right;
	float _pad0;

	vec3 up;
	float _pad1;

	Material material;
};

struct HitInfo {
	bool hit;
	float dst;
	vec3 hitPoint;
	vec3 normal;
	Material material;
	int hitType;
	vec2 uv;
};

const int HIT_TYPE_NONE = 0;
const int HIT_TYPE_SPHERE = 1;
const int HIT_TYPE_PLANE = 2;
const int HIT_TYPE_QUAD = 3;

layout(std430, binding = 0) readonly buffer Spheres { Sphere spheres[]; };
layout(std430, binding = 1) readonly buffer Planes { Plane planes[]; };
layout(std430, binding = 2) readonly buffer Quads { Quad _quads[]; };

uniform int uNumSpheres;
uniform int uNumPlanes;
uniform int uNumQuads;

vec2 calculateEquirectangularUV(vec3 dir) {
	dir = normalize(dir);

	float phi = atan(dir.z, dir.x);
	float theta = acos(dir.y);

	float u = phi / (2.0 * PI) + 0.5;  // Map to [0, 1]
	float v = theta / PI;  // Map to [0, 1]

	return vec2(u, v);
}

// === RANDOMNESS ===

// PCG (permuted congruential generator). Thanks to:
// www.pcg-random.org and www.reedbeta.com/blog/hash-functions-for-gpu-rendering
uint PCG_Hash(uint state) {
	state = state * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return ((word >> 22u) ^ word);
}

void NextRandomPCG(inout uint state) {
    state = PCG_Hash(state);
}

float RandomValue(inout uint state) {
    state = PCG_Hash(state);
	return float(state) / 4294967295.0;  // 2^32 - 1
}

// Normal Distribution (mean=0, std=1). Thanks to:
// en.wikipedia.org/wiki/Box-Muller_transform
float RandomValueNormalDistribution(inout uint state) {
	float rho = sqrt(-2.0 * log(RandomValue(state)));  // Magnitude
	float theta = 2.0 * PI * RandomValue(state);  // Angle
	return rho * cos(theta);  // Cartesian X from Polar form
}

vec3 RandomUnitVector(inout uint state) {
	float x = RandomValueNormalDistribution(state);
	float y = RandomValueNormalDistribution(state);
	float z = RandomValueNormalDistribution(state);
	return normalize(vec3(x, y ,z ));
}

// === RAYS ===

HitInfo RaySphereIntersect(Ray ray, Sphere sphere) {
	HitInfo hit;
    hit.hit = false;
    hit.dst = 1e20;
	hit.hitType = HIT_TYPE_NONE;

	vec3 oc = ray.origin - sphere.position;
	float a = dot(ray.dir, ray.dir);
	float b = 2.0 * dot(oc, ray.dir);
	float c = dot(oc, oc) - sphere.radius * sphere.radius;
	float discriminant = b * b - 4.0 * a * c;

	if (discriminant >= 0.0) {
		hit.hit = true;
		hit.dst = (-b - sqrt(discriminant)) / (2.0 * a);
		hit.hitPoint = ray.origin + ray.dir * hit.dst;
		hit.normal = normalize(hit.hitPoint - sphere.position);
		hit.material = sphere.material;
		hit.hitType = HIT_TYPE_SPHERE;
	}

	return hit;
}

HitInfo RayPlaneIntersect(Ray ray, Plane plane) {
	HitInfo hit;
    hit.hit = false;
    hit.dst = 1e20;
	hit.hitType = HIT_TYPE_NONE;

	float denominator = dot(plane.normal, ray.dir);
	if (denominator < 0.0) {
		hit.hit = true;
		hit.dst = dot(plane.normal, plane.position - ray.origin) / denominator;
		hit.hitPoint = ray.origin + ray.dir * hit.dst;
		hit.normal = plane.normal;
		hit.material = plane.material;
		hit.hitType = HIT_TYPE_PLANE;
	}

	return hit;
}

HitInfo RayQuadIntersect(Ray ray, Quad quad) {
	HitInfo hit;
    hit.hit = false;
    hit.dst = 1e20;
	hit.hitType = HIT_TYPE_NONE;

	float denominator = dot(quad.normal, ray.dir);
	if (denominator < 0.0) {
		hit.hit = true;
		hit.dst = dot(quad.normal, quad.position - ray.origin) / denominator;
		hit.hitPoint = ray.origin + ray.dir * hit.dst;
		hit.normal = quad.normal;
		hit.material = quad.material;
		hit.hitType = HIT_TYPE_QUAD;

		vec3 localHitPoint =  hit.hitPoint - quad.position;

		float u = dot(localHitPoint, quad.right);
		float v = dot(localHitPoint, quad.up);

		float halfWidth = quad.width * 0.5;
		float halfHeight = quad.height * 0.5;

		if (u < -halfWidth || u > halfWidth || v < -halfHeight || v > halfHeight) 
			hit.hit = false;
	}

	return hit;
}

HitInfo CalculateRayCollision(Ray ray) {
	HitInfo closestHit;
	closestHit.hit = false;
	closestHit.dst = 1e20;

	for (int i = 0; i < uNumSpheres; ++i) {
		HitInfo currentHit = RaySphereIntersect(ray, spheres[i]);
		if (currentHit.hit && currentHit.dst > 0.0 && currentHit.dst < closestHit.dst)
			closestHit = currentHit;	
	}
	
	for (int i = 0; i < uNumPlanes; ++i) {
		HitInfo currentHit = RayPlaneIntersect(ray, planes[i]);
		if (currentHit.hit && currentHit.dst > 0.0 && currentHit.dst < closestHit.dst)
			closestHit = currentHit;	
	}
	
	for (int i = 0; i < uNumQuads; ++i) {
		HitInfo currentHit = RayQuadIntersect(ray, _quads[i]);
		if (currentHit.hit && currentHit.dst > 0.0 && currentHit.dst < closestHit.dst)
			closestHit = currentHit;	
	}

	if (closestHit.hitType == HIT_TYPE_SPHERE && closestHit.material.flag != 0)
		closestHit.uv = calculateEquirectangularUV(closestHit.normal);

	return closestHit;
}

// === SKYBOX / ENVIRONMENT ===

vec3 GetEnvironmentLight(Ray ray) {
	vec3 environmentLight = vec3(0.0);

	if (uHasSkybox == 1) {
		vec2 uv = calculateEquirectangularUV(ray.dir);
		environmentLight = texture(uSkyboxTexture, uv).rgb * uSkyboxExposure;
	}

	if (uSunIntensity > 0.0 && uSunFocus > 0.0) {
		float sunDot = max(0.0, dot(ray.dir, uSunDirection));
		float sunSpot = pow(sunDot, uSunFocus);
		environmentLight += uSunColour * uSunIntensity * sunSpot;
	}

	return environmentLight;
}

// === TRACE ===

// Trace light-ray path (camera to light), accounting for reflections
vec3 Trace(Ray ray, inout uint rngState) {
	vec3 incomingLight = vec3(0.0);
	vec3 rayColour = vec3(1.0);

	for (uint i = 0; i < uMaxBounces; i++) {
		HitInfo hit = CalculateRayCollision(ray);

		if (!hit.hit) {
			incomingLight += GetEnvironmentLight(ray) * rayColour;
			break;
		}
		
		Material material = hit.material;

		if (material.flag == FLAG_CHECKERBOARD) {
			float x = hit.hitPoint.x;
			float z = hit.hitPoint.z;

			if (hit.uv != vec2(0.0)) {
				x = hit.uv.x * 20.0;
				z = hit.uv.y * 10.0;
			}

			int ix = int(floor(x));
			int iz = int(floor(z));

			bool isEvenSquare = (ix % 2 == iz % 2);
			material.colour = isEvenSquare ? material.colour : material.emissionColour;
		}
		
		// Accumulate light
		incomingLight += material.emissionColour * material.emissionStrength * rayColour;

		// Calculate next ray
		ray.origin = hit.hitPoint;

		bool isSpecular = material.specularProbability >= RandomValue(rngState);
		if (isSpecular) {
			vec3 specularDir = reflect(ray.dir, hit.normal);
			ray.dir = normalize(specularDir + RandomUnitVector(rngState) * (1.0 - material.smoothness));
			rayColour *= material.specularColour;
		} else {
			vec3 diffuseDir = normalize(hit.normal + RandomUnitVector(rngState));
			if (dot(diffuseDir, hit.normal) < 0.0) diffuseDir = -diffuseDir;
				ray.dir = diffuseDir;
			rayColour *= material.colour;
		}

		// "Russian roulette" to exit early if rayColour is nearly 0 (little contribution)
		float p = max(rayColour.r, max(rayColour.g, rayColour.b));
		if (RandomValue(rngState) >= p)
			break;
		rayColour /= p;
	}

	return incomingLight;
}

void main() {
	// RNG seed
	ivec2 pixelCoords = ivec2(gl_FragCoord.xy);
	uint pixelIndex = uint(pixelCoords.y) * uint(uResolution.x) + uint(pixelCoords.x);
    uint rngState = pixelIndex + uFrame * 719393u;
	
	vec3 frameSampleAccumulator = vec3(0.0);

	for (uint s = 0; s < uSamplesPerPixel; ++s) {        
		uint sampleRngState = PCG_Hash(rngState + s * 131071u);

        vec2 jitteredScreenUV = (gl_FragCoord.xy + vec2(RandomValue(sampleRngState), RandomValue(sampleRngState))) / uResolution;
        vec2 jitteredUV = jitteredScreenUV * 2.0 - 1.0; // Convert [0,1] to [-1,1]
	    jitteredUV.x *= uResolution.x / uResolution.y;

	    // Path Tracing
		Ray ray;
		ray.origin = uCameraPosition;
		ray.dir = normalize(uCameraForward + jitteredUV.x * uCameraRight + jitteredUV.y * uCameraUp);

		vec3 incomingLight = Trace(ray, sampleRngState);

		frameSampleAccumulator += incomingLight;
	}
	
    vec3 currentFrameColour = frameSampleAccumulator / float(uSamplesPerPixel);

    // Accumulation (progressive rendering)
    vec3 finalAccumulated;
    if (uFrame == 1) {
        finalAccumulated = currentFrameColour;
    } else {
        vec4 prevAccumulated = imageLoad(uAccumulatedImage, pixelCoords);
        finalAccumulated = (prevAccumulated.rgb * float(uFrame - 1) + currentFrameColour) / float(uFrame);
    }

    imageStore(uAccumulatedImage, pixelCoords, vec4(finalAccumulated, 1.0));
    
    // Gamma Correction
    finalAccumulated = pow(finalAccumulated, vec3(1.0 / uGamma));

    FragColour = vec4(finalAccumulated, 1.0);
}