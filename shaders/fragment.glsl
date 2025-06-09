#version 440 core

const float PI = 3.1415926;

in vec2 vUV;
out vec4 FragColour;

uniform vec2 uResolution;

uniform vec3 uCameraPosition;
uniform vec3 uCameraForward;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;

uniform uint uMaxBounces;
uniform uint uFrame;

layout(rgba32f, binding = 0) uniform image2D uAccumulatedImage;

struct Ray {
	vec3 origin;
	vec3 dir;
};

struct Material {
	vec3 colour;
	float _pad0;

	vec3 emissionColour;
	float emissionStrength;
};

struct Sphere {
	vec3 position;
	float radius;
	Material material;
};

struct HitInfo {
	bool hit;
	float dst;
	vec3 hitPoint;
	vec3 normal;
	Material material;
};

layout(std430, binding = 0) buffer Spheres {
	Sphere spheres[];
};

uniform int uNumSpheres;

// PCG (permuted congruential generator). Thanks to:
// www.pcg-random.org and www.reedbeta.com/blog/hash-functions-for-gpu-rendering
uint NextRandomPCG(inout uint state) {
	state = state * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return ((word >> 22u) ^ word);
}

float RandomValue(inout uint state) {
	return NextRandomPCG(state) / 4294967295.0;  // 2^32 - 1
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

bool RaySphereIntersect(Ray ray, vec3 centre, float radius, out float dst) {
	vec3 oc = ray.origin - centre;
	float a = dot(ray.dir, ray.dir);
	float b = 2.0 * dot(oc, ray.dir);
	float c = dot(oc, oc) - radius * radius;
	float discriminant = b * b - 4.0 * a * c;

	if (discriminant < 0.0)
		return false;

	dst = (-b - sqrt(discriminant)) / (2.0 * a);
	return dst >= 0.0;
}

HitInfo CalculateRayCollision(Ray ray) {
	HitInfo closestHit;
	closestHit.hit = false;
	closestHit.dst = 1e20;

	// Find closest intersection
	for (int i = 0; i < uNumSpheres; ++i) {
		float dst;
		if (RaySphereIntersect(ray, spheres[i].position, spheres[i].radius, dst)) {
			if (dst > 0.0 && dst < closestHit.dst) {
				closestHit.hit = true;
				closestHit.dst = dst;
				closestHit.hitPoint = ray.origin + ray.dir * dst;
				closestHit.normal = normalize(closestHit.hitPoint - spheres[i].position);
				closestHit.material = spheres[i].material;
			}
		}
	}

	return closestHit;
}

vec3 CalculateLighting(HitInfo hit) {
	const float EPSILON = 1e-3;
	vec3 lightColour = vec3(0.0);

	// Find all light sources
	for (int i = 0; i < uNumSpheres; ++i) {
		if (spheres[i].material.emissionStrength != 0.0) {
			vec3 lightDir = normalize(spheres[i].position - hit.hitPoint);
			float lightCentreDst = length(spheres[i].position - hit.hitPoint);

			Ray shadowRay;
			shadowRay.origin = hit.hitPoint + hit.normal * EPSILON;
			shadowRay.dir = lightDir;
			
			HitInfo shadowHit = CalculateRayCollision(shadowRay);

			if (shadowHit.hit && shadowHit.dst > EPSILON && shadowHit.dst < (lightCentreDst - spheres[i].radius - EPSILON)) {
				continue;
			}

			// Calculate lighting contribution
			float dotProduct = max(dot(hit.normal, lightDir), 0.0);
			lightColour += spheres[i].material.emissionColour * spheres[i].material.emissionStrength * hit.material.colour * dotProduct;
		}
	}

	return lightColour;
}

// Trace light-ray path (camera to light), accounting for reflections
vec3 Trace(Ray ray, inout uint rngState) {
	vec3 incomingLight = vec3(0.0);
	vec3 rayColour = vec3(1.0);

	for (uint i = 0; i < uMaxBounces; i++) {
		HitInfo hit = CalculateRayCollision(ray);

		if (!hit.hit)
			break;

		// Calculate next ray
		ray.origin = hit.hitPoint + hit.normal * 0.0001;

		vec3 dir = normalize(hit.normal + RandomUnitVector(rngState));
		if (dot(dir, hit.normal) < 0.0) dir = -dir;
		ray.dir = dir;

		// Accumulate light
		Material material = hit.material;
		incomingLight += material.emissionColour * material.emissionStrength * rayColour;
		rayColour *= material.colour;

		// "Russian roulette" to exit early if rayColour is nearly 0 (little contribution)
		float p = max(rayColour.r, max(rayColour.g, rayColour.b));
		if (RandomValue(rngState) >= p)
			break;
		rayColour /= p;
	}

	return incomingLight;
}

void main() {
	vec2 uv = vUV * 2.0 - 1.0;  // Normalise uv to [-1,1]
	uv.x *= uResolution.x / uResolution.y;

	// RNG seed
	ivec2 pixelCoords = ivec2(gl_FragCoord.xy);
	uint pixelIndex = uint(pixelCoords.y) * uint(uResolution.x) + uint(pixelCoords.x);
	uint rngState = pixelIndex + uFrame * 719393u;
	
    // Path Tracing
    Ray ray;
    ray.origin = uCameraPosition;
    ray.dir = normalize(uCameraForward + uv.x * uCameraRight + uv.y * uCameraUp);
    vec3 incomingLight = Trace(ray, rngState);

    // Accumulation (progressive rendering)
    vec3 finalAccumulated;
    if (uFrame == 1) {
        finalAccumulated = incomingLight;
    } else {
        vec4 prevAccumulated = imageLoad(uAccumulatedImage, pixelCoords);
        finalAccumulated = (prevAccumulated.rgb * float(uFrame - 1) + incomingLight) / float(uFrame);
    }

    imageStore(uAccumulatedImage, pixelCoords, vec4(finalAccumulated, 1.0));
    
    // Gamma Correction
    finalAccumulated = pow(finalAccumulated, vec3(1.0/2.2));

    FragColour = vec4(finalAccumulated, 1.0);
}