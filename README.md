# ray-tracing

## Ray Tracing Maths

### Fundamental Concepts

-   Equation for Ray

    -   Ray(dst) = Position + Direction × dst
    -   Ray(dst) is a point on the ray at distance `dst`
    -   `Position` is the origin vector of the ray
    -   `Direction` is a unit vector (magnitude = 1) where the ray points towards
    -   `dst` is a scalar distance along the ray

### Intersection of a Sphere

-   **Equation for Sphere, centred at origin (0, 0, 0)**

    -   x² + y² + z² = r²
    -   When a ray intersects the boundary of the sphere, this equation is true
        -   ||Position + Direction × dst||² = r²

-   **Equation for Sphere, centred at C = (Cx, Cy, Cz)**

    -   (x - Cx)² + (y - Cy)² + (z - Cz)² = r²
    -   When a ray intersects the boundary of the sphere, this equation is true
        -   ||(Position + Direction × dst) - C||² = r²

-   **Re-arrange to solve a quadratic for `dst`**

    -   dst² + 2 × (OC · Direction) × dst + (OC · OC - r²) = 0
        -   Where OC = Position - C
    -   Discriminant = **b² - 4ac**
        -   = (OC · Direction)² - (OC · OC - r²)
        -   b² - 4ac > 0, two solutions, ray enters and exits the sphere
        -   b² - 4ac = 0, one solution, ray just touches the sphere (tangent)
        -   b² - 4ac < 0, no real solutions, no intersection

### Intersection of a Plane (infinite)

-   **Equation of a Plane (dot product form)**

    -   n · (r - a) = 0
        -   **n** is the normal vector of the plane
        -   **r** is any arbitrary point that lines on the plane
        -   **a** is a known point that lies on the plane
    -   n · (Ray(dst) - a) = 0
    -   n · ((Position + Direction × dst) - a) = 0

-   **Re-arrange to solve for `dst`**

    -   n · (Position - a + Direction × dst) = 0
    -   n · (Position - a) + dst × (n · Direction) = 0
    -   dst × (n · Direction) = -n · (Position - a)
    -   dst = -n · (Position - a) / (n · Direction)
        -   n · Direction > 0, ray points at the back side of the plane, don't render
        -   n · Direction ≈ 0, ray is parallel to the plane, no intersection
        -   n · Direction < 0, ray points at the front side of the plane, render

### Lighting

-   **Lambertian (Diffuse) Lighting**
    -   Reflects light uniformly in all directions from a matte surface
    -   Brightness depends on the angle between the surface normal and the light direction
    -   Diffuse = LightColour × SurfaceColour × max(Normal · LightDir, 0)
-   **Path Tracing**
    -   Simulates bouncing of light rays throughout a scene
    -   Accumulates light by tracing multiple paths of rays and summing up the contributions
    -   **Emissive Materials:**
        -   Objects that emit light directly contribute to the scene's illumination
        -   `incomingLight += material.emissionColour * material.emissionStrength * rayColour;`
    -   **Ray Colour Accumulation:**
        -   Colour is accumulated as a ray bounces
        -   The ray's colour is attenuated by the material's colour, simulating how the surface absorbs some light and reflects the rest
        -   `rayColour *= material.colour;`
    -   **Diffuse Reflection Sampling:**
        -   For diffuse surfaces, light is scattered in all directions
        -   Cosine-weighted hemisphere sampling is used to bias samples towards the normal, improving convergence
        -   `vec3 dir = normalize(hit.normal + RandomUnitVector(rngState));`
    -   **Specular Reflection:**
        -   Occurs on smooth, polished surfaces, reflecting light in a single, concentrated direction
        -   ReflectedDir = LightDir - 2 × (LightDir · Normal) × Normal
        -   `vec3 specularDir = reflect(ray.dir, hit.normal);`
    -   **Environmental Lighting:**
        -   If a ray does not hit an object in the scene, light is sampled from the environment (skybox & procedural sun)
        -   This provides ambient illumination for the scene
        -   `environmentLight = texture(uSkyboxTexture, uv).rgb * uSkyboxExposure;`
        -   `environmentLight += uSunColour * uSunIntensity * pow(max(0.0, dot(ray.dir, uSunDirection)), uSunFocus);`
    -   **Russian Roulette:**
        -   A method to reduce variance in Monte Carlo path tracing
        -   Prevents paths from becoming excessively long, while remaining unbiased
        -   Improves performance by terminating rays that are unlikely to contribute much to the final render
        -   ```glsl
                float p = max(rayColour.r, max(rayColour.g, rayColour.b));
                if (RandomValue(rngState) >= p) break;
                rayColour /= p;
            ```

### Equirectangular Projection

-   Unwraps a sphere onto a 2D rectangular image, used for HDR skybox / environment maps
-   **Unit vector:** D = (x, y, z)
-   **Spherical Coordinates (in my project)**
    -   Longitude (φ) - The angle around the Y-axis
        -   φ = atan2(z, x), φ ∈ [−π, π]
    -   Latitude (θ) - The angle from the Y-axis downwards
        -   θ = acos(y), θ ∈ [0, π]
-   **Mapping to UV Coordinates**
    -   u = ϕ / (2π) + 0.5, u ∈ [0, 1]
    -   v = θ / π, v ∈ [0, 1]

### EV (Exposure Value)

-   **Exposure Value (EV)** is a logarithmic scale representing a scene's overall brightness
-   Used to adjust the linear intensity of the scene or skybox / environment map to simulate photographic exposure settings
-   LinearMultiplier = 2^EV
-   **Examples:**
    -   EV = 0: LinearMultiplier = 2^0 = 1 (original brightness)
    -   EV = 1: LinearMultiplier = 2^1 = 2 (double brightness)
    -   EV = -1: LinearMultiplier = 2^-1 = 0.5 (half brightness)

### Gamma Correction

-   **Gamma correction** accounts for the non-linear way human eyes perceive light and colour
    -   Monitors don't output light linearly
    -   Our eyes are more sensitive to changes in darker tones than brighter ones
    -   This means a linearly calculated image would look dark and muddy if displayed directly
-   Gamma correction is a non-linear (power function) transformation applied to linear light values to correct the brightness and contrast of the rendered image
-   DisplayValue = LinearValue^(1/γ)
-   **Standard Gamma:** γ = 2.2
    -   1 / 2.2 ≈ 0.4545
