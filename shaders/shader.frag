#version 330 core

out vec4 FragColor;

uniform ivec2 viewport;

uniform vec3 cameraPosition;
uniform vec3 cameraForward;
uniform vec3 cameraUp;

struct PointLight {
	vec3 position;
	vec3 color;
};

struct Material {
	vec3  diffuse;
	vec3  specular;
	float glossiness;
	float reflection;
	float refraction;
	float ior;
};

struct Sphere {
	vec3 position;
	float radius;
	Material material;
};

const int lightCount = 1;
const int sphereCount = 1;

struct Scene {
	vec3 ambient;
	PointLight lights[lightCount];
	Sphere spheres[sphereCount];
};

struct Ray {
	vec3 origin;
	vec3 direction;
};

// Contains all information pertaining to a ray/object intersection
struct HitInfo {
	bool hit;
	float t;
	vec3 position;
	vec3 normal;
	Material material;
	bool enteringPrimitive;
};

HitInfo getEmptyHit() {
	return HitInfo(
		false, 
		0.0, 
		vec3(0.0), 
		vec3(0.0), 
		Material(vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, 0.0),
		false);
}

// Sorts the two t values such that t1 is smaller than t2
void sortT(inout float t1, inout float t2) {
	// Make t1 the smaller t
	if(t2 < t1)  {
		float temp = t1;
		t1 = t2;
		t2 = temp;
	}
}

// Tests if t is in an interval
bool isTInInterval(const float t, const float tMin, const float tMax) {
	return t > tMin && t < tMax;
}

// Get the smallest t in an interval.
bool getSmallestTInInterval(float t0, float t1, const float tMin, const float tMax, inout float smallestTInInterval) {
  
	sortT(t0, t1);

	// As t0 is smaller, test this first
	if(isTInInterval(t0, tMin, tMax)) {
		smallestTInInterval = t0;
		return true;
	}

	// If t0 was not in the interval, still t1 could be
	if(isTInInterval(t1, tMin, tMax)) {
		smallestTInInterval = t1;
		return true;
	}  

	// None was
	return false;
}

HitInfo intersectSphere(const Ray ray, const Sphere sphere, const float tMin, const float tMax) {
              
    vec3 to_sphere = ray.origin - sphere.position;
  
    float a = dot(ray.direction, ray.direction);
    float b = 2.0 * dot(ray.direction, to_sphere);
    float c = dot(to_sphere, to_sphere) - sphere.radius * sphere.radius;
    float D = b * b - 4.0 * a * c;
    if (D > 0.0)
    {
		float t0 = (-b - sqrt(D)) / (2.0 * a);
		float t1 = (-b + sqrt(D)) / (2.0 * a);
      
      	float smallestTInInterval;
      	if(!getSmallestTInInterval(t0, t1, tMin, tMax, smallestTInInterval)) {
          return getEmptyHit();
        }
      
      	vec3 hitPosition = ray.origin + smallestTInInterval * ray.direction;      
		
		//Checking if we're inside the sphere by checking if the ray's origin is inside. If we are, then the normal 
		//at the intersection surface points towards the center. Otherwise, if we are outside the sphere, then the normal 
		//at the intersection surface points outwards from the sphere's center. This is important for refraction.
      	vec3 normal = 
          	length(ray.origin - sphere.position) < sphere.radius + 0.001? 
          	-normalize(hitPosition - sphere.position): 
      		normalize(hitPosition - sphere.position);      
		
		//Checking if we're inside the sphere by checking if the ray's origin is inside,
		// but this time for IOR bookkeeping. 
		//If we are inside, set a flag to say we're leaving. If we are outside, set the flag to say we're entering.
		//This is also important for refraction.
		bool enteringPrimitive = 
          	length(ray.origin - sphere.position) < sphere.radius + 0.001 ? 
          	false:
		    true; 

        return HitInfo(
          	true,
          	smallestTInInterval,
          	hitPosition,
          	normal,
          	sphere.material,
			enteringPrimitive);
    }
    return getEmptyHit();
}

HitInfo getBetterHitInfo(const HitInfo oldHitInfo, const HitInfo newHitInfo) {
	if(newHitInfo.hit)
  		if(newHitInfo.t < oldHitInfo.t)  // No need to test for the interval, this has to be done per-primitive
          return newHitInfo;
  	return oldHitInfo;
}

HitInfo intersectScene(const Scene scene, const Ray ray, const float tMin, const float tMax) {
	HitInfo bestHitInfo;
	bestHitInfo.t = tMax;
	bestHitInfo.hit = false;
	
	for (int i = 0; i < sphereCount; ++i) {
		bestHitInfo = getBetterHitInfo(bestHitInfo, intersectSphere(ray, scene.spheres[i], tMin, tMax));
	}
	
	return bestHitInfo;
}

vec3 shadeFromLight(
  const Scene scene,
  const Ray ray,
  const HitInfo hit_info,
  const PointLight light)
{ 
  vec3 hitToLight = light.position - hit_info.position;
  
  vec3 lightDirection = normalize(hitToLight);
  vec3 viewDirection = normalize(hit_info.position - ray.origin);
  vec3 reflectedDirection = reflect(viewDirection, hit_info.normal);
  float diffuse_term = max(0.0, dot(lightDirection, hit_info.normal));
  float specular_term  = pow(max(0.0, dot(lightDirection, reflectedDirection)), hit_info.material.glossiness);

  // VISIBILITY TEST
  float visibility = 1.0;
  Ray hitToLightRay = Ray(hit_info.position, lightDirection);  // uses normalised hitToLight (otherwise tMax is interpreted incorrectly)
  float tMin = 0.001; // accounts for floating point error
  float tMax = length(hitToLight);
  HitInfo shadeHitInfo = intersectScene(scene, hitToLightRay, tMin, tMax);
  if (shadeHitInfo.hit)
	visibility = 0.0;
  return 	visibility * 
    		light.color * (
    		specular_term * hit_info.material.specular +
      		diffuse_term * hit_info.material.diffuse);
}

vec3 background(const Ray ray) {
  // A simple implicit sky that can be used for the background
  return vec3(0);
  // return vec3(0.2) + vec3(0.8, 0.6, 0.5) * max(0.0, ray.direction.y);
}

vec3 shade(const Scene scene, const Ray ray, inout HitInfo hitInfo) {
	
  	if(!hitInfo.hit) {
  		return background(ray);
  	}
  
    vec3 shading = scene.ambient * hitInfo.material.diffuse;
    for (int i = 0; i < lightCount; ++i) {
        shading += shadeFromLight(scene, ray, hitInfo, scene.lights[i]); 
    }
    return shading;
}


Ray getFragCoordRay(const vec2 frag_coord) {
    float sensorDistance = 1.0;
    vec2 sensorMin = vec2(-1.0, -0.5);
    vec2 sensorMax = vec2(1.0, 0.5);
    vec2 pixelSize = (sensorMax - sensorMin) / vec2(viewport);
    vec2 sensorCoord = sensorMin + pixelSize * frag_coord;

    vec3 right = normalize(cross(cameraForward, cameraUp));
    vec3 up = normalize(cross(right, cameraForward));
    vec3 direction = normalize(
        cameraForward * sensorDistance +
        right * sensorCoord.x +
        up * sensorCoord.y
    );

    return Ray(cameraPosition, direction);
}

float fresnel(const vec3 viewDirection, const vec3 normal, const float sourceIOR, const float destIOR) {
	// used Schlick's approximation
	float r0 = pow((sourceIOR - destIOR) / (sourceIOR + destIOR), 2.0);
	float cosAngle = dot(viewDirection, normal) / (length(viewDirection) * length(normal)); // 1.0 when viewDirection parallel with normal, 0.0 when perpendicular
	//return pow(1.0 - cosAngle, 2.0); // single dot product approximation
	return r0 + (1.0 - r0)*pow(1.0 - cosAngle, 3.0); // Schlick's approximation, typically uses 5.0 as the power but i thought 3.0 looked better
}

vec3 colorForFragment(const Scene scene, const vec2 fragCoord) {
      
    Ray initialRay = getFragCoordRay(fragCoord);  
  	HitInfo initialHitInfo = intersectScene(scene, initialRay, 0.001, 10000.0);  
  	vec3 result = shade(scene, initialRay, initialHitInfo);
	
  	Ray currentRay;
  	HitInfo currentHitInfo;
  	
  	// Compute the reflection
  	currentRay = initialRay;
  	currentHitInfo = initialHitInfo;
  	
  	// The initial strength of the reflection
  	float reflectionWeight = 1.0;
	
	// The initial medium is air
  	float currentIOR = 1.0;
	
    float sourceIOR = 1.0;
	float destIOR = 1.0;
  	
  	const int maxReflectionStepCount = 2;
  	for(int i = 0; i < maxReflectionStepCount; i++) {
      
      if(!currentHitInfo.hit) break;
      
	  reflectionWeight *= currentHitInfo.material.reflection;
      
	  // FRESNEL REFLECTION
	  if (currentHitInfo.enteringPrimitive) { // calculate sourceIOR and destIOR for Schlick approximation
		  sourceIOR = currentIOR;
		  destIOR = currentHitInfo.material.ior;
	  }
	  else {
		  sourceIOR = currentHitInfo.material.ior;
		  destIOR = 1.0; // assumes exiting an object implies entering air
	  }
	  float fresnelReflection = fresnel(-currentRay.direction, currentHitInfo.normal, sourceIOR, destIOR);
	  reflectionWeight *= fresnelReflection;
      
	  // REFLECTED RAY
      Ray nextRay;
	  nextRay.origin = currentHitInfo.position;
	  nextRay.direction = normalize(currentRay.direction - 2.0 * dot(currentRay.direction, currentHitInfo.normal) * currentHitInfo.normal);

      currentRay = nextRay;
      
      currentHitInfo = intersectScene(scene, currentRay, 0.001, 10000.0);      
            
      result += reflectionWeight * shade(scene, currentRay, currentHitInfo);
    }
  
  	// Compute the refraction
  	currentRay = initialRay;  
  	currentHitInfo = initialHitInfo;
   
  	// The initial strength of the refraction.
  	float refractionWeight = 1.0;
  
  	const int maxRefractionStepCount = 2;
  	for(int i = 0; i < maxRefractionStepCount; i++) {
      
	  if (!currentHitInfo.hit) break;
	  refractionWeight *= currentHitInfo.material.refraction;
	  
	  // FRESNEL REFRACTION
	  if (currentHitInfo.enteringPrimitive) {
		  sourceIOR = currentIOR;
		  destIOR = currentHitInfo.material.ior;
	  }
	  else {
		  sourceIOR = currentHitInfo.material.ior;
		  destIOR = 1.0; // assumes exiting an object implies entering air
	  }
	  float fresnelReflection = fresnel(-currentRay.direction, currentHitInfo.normal, sourceIOR, destIOR);
	  refractionWeight *= 1.0 - fresnelReflection;    

	  // REFRACTED RAY
      Ray nextRay;

	  float cosAlpha = dot(-currentRay.direction, currentHitInfo.normal) / (length(currentRay.direction) * length(currentHitInfo.normal)); // division not necessary as normalized?
	  float n12 = sourceIOR / destIOR;
	  float k = 1.0 + pow(n12, 2.0) * (pow(cosAlpha, 2.0) - 1.0);
	  if (k < 0.0)
	    break; // total internal reflection

	  nextRay.origin = currentHitInfo.position;
	  nextRay.direction = normalize(n12 * currentRay.direction + currentHitInfo.normal * (n12 * cosAlpha - sqrt(k)));
	
	  currentIOR = destIOR;
	  currentRay = nextRay;

      currentHitInfo = intersectScene(scene, currentRay, 0.001, 10000.0);
            
      result += refractionWeight * shade(scene, currentRay, currentHitInfo);
      
      if(!currentHitInfo.hit) break;
    }
  return result;
}

Material getPaperMaterial() {
    return Material(vec3(0.7, 0.7, 0.7), vec3(0, 0, 0), 5.0, 0.0, 0.0, 0.0);
}

vec3 tonemap(const vec3 radiance) {
  const float monitorGamma = 2.0;
  return pow(radiance, vec3(1.0 / monitorGamma));
}

void main()
{
  Scene scene;

  PointLight star;
  star.color = vec3(1.0, 0.95, 0.85);
  star.position = vec3(0, 0, 0);

  Sphere planet;
  planet.position = vec3(-1.0, 0.0, -5.0);
  planet.material = getPaperMaterial();
  planet.radius = 1.0;

  scene.lights[0] = star;

  scene.spheres[0] = planet;

  FragColor.rgb = tonemap(colorForFragment(scene, gl_FragCoord.xy));
  FragColor.a = 1.0;
}