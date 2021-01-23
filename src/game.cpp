#include <stdarg.h> // For variable number of arguments
#include <math.h>
#include <chrono>

#include "precomp.h" // include (only) this in every .cpp file
#include "game.h"
#include "ray.h"
#include "light.h"
#include "utils.h"
#include "timer.h"

// .obj loader
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

const int KERNEL_CENTER = KERNEL_SIZE / 2;

void Game::InitDefaultScene()
{
	// materials
	nr_materials = 7;
	materials = new Material[nr_materials] {
		Material(0, 0, 1, Color(1, 1, 1), nullptr), // wall
		Material(1, 0, 1, Color(1, 1, 1), nullptr), // mirror
		Material(0.5f, 0.5f, 1.5f, Color(1, 1, 1), nullptr), // glass
		Material(0, 0, 1, Color(1, 0, 0), nullptr), // red
		Material(0, 0, 1, Color(0, 1, 0), nullptr), // green
		Material(0, 0, 1, Color(0, 0, 1), nullptr), // blue
		Material(0, 0, 1, Color(1, 1, 0), nullptr), // yellow
	};

	// triangles
	float room_size = 5;
	float room_depth = 10;
	// loa = links, onder, achter; rbv = rechts, boven, voor
	vec3 loa = vec3(0, 0, 0), lov = vec3(0, 0, room_depth);
	vec3 lba = vec3(0, room_size, 0), lbv = vec3(0, room_size, room_depth);
	vec3 roa = vec3(room_size, 0, 0), rov = vec3(room_size, 0, room_depth);
	vec3 rba = vec3(room_size, room_size, 0), rbv = vec3(room_size, room_size, room_depth);

	nr_triangles = 12;
	triangles = new Triangle[nr_triangles] {
		Triangle(loa, lov, roa, 0), // floor 1
		Triangle(lov, roa, rov, 0), // floor 2
		Triangle(lba, lbv, rba, 0), // roof 1
		Triangle(lbv, rba, rbv, 0), // roof 2
		Triangle(loa, lba, lov, 3), // wall left 1
		Triangle(lba, lov, lbv, 3), // wall left 2
		Triangle(roa, rba, rov, 6), // wall right 1
		Triangle(rba, rov, rbv, 6), // wall right 2
		Triangle(loa, lba, roa, 0), // wall back 1
		Triangle(lba, roa, rba, 0), // wall back 2
		Triangle(lov, lbv, rov, 0), // wall front 1
		Triangle(lbv, rov, rbv, 0), // wall front 2
	};

	// spheres
	float radius = 0.5f;

	nr_spheres = 3;
	spheres = new Sphere[nr_spheres] {
		Sphere(vec3(2 + 0, radius + 0.2f, 2.5f + 0), radius, 4),
		Sphere(vec3(2 + 1, radius + 0.2f, 2.5f + 1), radius, 5),
		Sphere(vec3(2 + 2, radius + 0.2f, 2.5f + 2), radius, 4),
	};

	view = new Camera(vec3(room_size/2, 1, 0.3f), vec3(0, 0, 1));
	sky = nullptr;

	nr_lights = 1;
	lights = new Light *[nr_lights] {
		new SphereLight( vec3( room_size/2, room_size, room_depth/2 ), 0.5f, Color( 200, 200, 200 ) )
	};
}

void Game::InitFromTinyObj( const std::string filename )
{
	view = new Camera( vec3( -18, -15, -0.1 ), vec3( 1, 0.25f, 0 ) );


	// load skybox
	sky = new SkyDome();

	// load lights
	// All lights should have atleast one color value != 0
	nr_lights = 1;
	lights = new Light *[nr_lights] {
		new SphereLight( vec3( -5, 10, 0 ), 8, Color( 50, 50, 50 ) )
	};

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> obj_materials;
	std::string warn;
	std::string err;

	std::string basedir;
	size_t found = filename.find_last_of("/\\");
	if (found == std::string::npos) {
		basedir = std::string("");
	} else {
		basedir = filename.substr(0,found + 1);
	}

	bool ret = tinyobj::LoadObj( &attrib, &shapes, &obj_materials, &warn, &err, filename.c_str(), basedir.c_str(), true, false );
	if ( !warn.empty() )
		std::cout << warn << std::endl;
	if ( !err.empty() )
		std::cerr << err << std::endl;
	if ( !ret )
		exit( 1 );

	std::cout << "Loading texture maps" << std::endl;

	// load materials
	nr_materials = obj_materials.size();
	materials = new Material[nr_materials];
	Material* current_mat = materials;
	for (size_t t = 0; t < obj_materials.size(); t++)
	{
		Material::FromTinyObj(current_mat, basedir, obj_materials[t]);
		current_mat++;
	}

	nr_spheres = 0;
	nr_triangles = 0;
    for (size_t s = 0; s < shapes.size(); s++)
		nr_triangles += shapes[s].mesh.indices.size() / 3;

	triangles = new Triangle[nr_triangles];
	Triangle* current = triangles;

	for (size_t s = 0; s < shapes.size(); s++)
	{
		for (size_t f = 0; f < shapes[s].mesh.indices.size() / 3; f++)
		{
			Triangle::FromTinyObj(current, &attrib, &shapes[s].mesh, f, obj_materials);
			current++;
		}
	}
}

// -----------------------------------------------------------
// Initialize the application
// -----------------------------------------------------------
void Game::SetTarget( Surface* surface )
{ 
	screen = surface;
	if (pixelData != nullptr)
		free(pixelData);
	pixelData = new PixelData[screen->GetWidth() * screen->GetHeight()];
	if (pixelColor != nullptr)
		free(pixelColor);
	pixelColor = new Color[screen->GetWidth() * screen->GetHeight()];
	CameraChanged();
}

void Game::Init(int argc, char **argv)
{
	printf("Initializing Game\n");
	default_material = new Material();

	// load model
	switch (argc)
	{
		case 1: // No arguments
			InitDefaultScene();
			break;
		case 2: // An obj file
			InitFromTinyObj(std::string(argv[1]));
			break;
		default:
			std::cout << argc << " arguments not accepted!" << std::endl;
			exit(1);
			break;
	}

	#ifdef USEBVH 
	std::cout << "Creating BVH" << std::endl;
	if ( nr_triangles > 0 )
	{
		bvh = new BVH();

		timer::TimePoint t = timer::get();
		bvh->ConstructBVH( triangles, nr_triangles );
		std::cout << "Construction time: " << timer::elapsed(t) << " ms." << std::endl;

		if (bvh->nr_nodes < 100 && nr_triangles < 100)
			bvh->Print();
	}
	#endif

	GenerateGaussianKernel( 10.0f );
	std::cout << "Done initializing" << std::endl;
}

// -----------------------------------------------------------
// Close down application
// -----------------------------------------------------------
void Game::Shutdown()
{
	printf("Shutting down Game\n");
}

bool Game::CheckOcclusion( Ray *r )
{
	// If any intersection found, return, don't need to know location
	// Check Spheres
	for ( uint i = 0; i < nr_spheres; i++ )
	{
		if (spheres[i].Occludes( r ))
			return true;
	}
	// Check triangles
	bvh->Occludes(r);
	// Check lights
	for ( size_t i = 0; i < nr_lights; i++ )
	{
		if ( lights[i]->Occludes( r ) )
			return true;
	}
	return false;
}

bool Game::Intersect( Ray* r, uint &depth )
{
	bool found = false; 
	
	for (uint i = 0; i < nr_spheres; i++)
		found |= spheres[i].Intersect(r);

	#ifdef USEBVH 
		found |= bvh->Intersect(r, depth);
	#else
		for (uint i = 0; i < nr_triangles; i++)
			found |= triangles[i].Intersect(r);
	#endif

	return found;
}

Light* Game::IntersectLights( Ray* r )
{
	Light* found = nullptr;

	for (uint i = 0; i < nr_lights; i++)
	{
		if(lights[i]->Intersect(r))
			found = lights[i];
	}

	return found;
}

Color Game::DirectIllumination( vec3 interPoint, vec3 normal )
{
	// accumulated color
	Color total( 0 );
	Ray shadowRay(interPoint, vec3(0));
	float fac;
	// send shadow ray to each light and add its color
	for ( size_t l = 0; l < NR_LIGHT_SAMPLES; l++ )
	{
		// TODO: take size of light into account -> higher chance for lights that are closer or bigger
		size_t i = RandomIndex(nr_lights);
		// compute origin and direction of shadow ray
		shadowRay.direction = lights[i]->PointOnLight() - shadowRay.origin;
		shadowRay.t = shadowRay.direction.length();
		shadowRay.direction *= (1 / shadowRay.t);

		fac = dot( shadowRay.direction, normal );

		// This would mean no additional color so let's early out.
		// TODO: use epsilon?
		if (fac <= 0)
			continue;

		// TODO: Check the normal of the light and the shadowray direction, their dot should also be >0
		// fac_light = ???
		float fac_light = 1;

		if (fac_light <= 0)
			continue;

		// find intersection of shadow ray, check if it is between the light and object
		if ( CheckOcclusion( &shadowRay ) )
			continue;

		// TODO: Find some way of getting the Scene.LIGHTAREA.
		float light_area = 1;

		// angle * distance attenuation * color
		Color thislight = (fac / ( shadowRay.t * shadowRay.t )) * lights[i]->color;
		thislight *= INVPI * nr_lights * fac_light * light_area;
		total += thislight;
	}
	return (1 / NR_LIGHT_SAMPLES) * total;
}

Color Game::Sample(Ray r, bool specularRay, uint depth, uint pixelId)
{
	if (depth > MAX_NR_ITERATIONS)
		return Color(0, 0, 0);

	Light* light = IntersectLights( &r );

	uint bvhDepth = 0;
	bool found = Intersect( &r, bvhDepth );

	#ifdef VISUALIZEBVH
		return Color( 0, std::min( 0.02f * bvhDepth, 1.0f ), 0 );
	#endif

	// No intersection point found
	if ( !found )
	{
		if ( light != nullptr )
		#ifdef USENEE
			if (specularRay)
				return light->color;
			else
				return Color(0, 0, 0);
		#else
			return light->color;
		#endif
		if (sky != nullptr)
			return sky->FindColor(r.direction);
		return SKYDOME_DEFAULT_COLOR;
	}

	// As al our debuging objects are close, 1000 is a safe value.
	// assert(r.t <= 1000);
	assert(r.obj != nullptr);
	assert(r.obj->material >= -1);
	assert(r.obj->material < (int)nr_materials);

	// intersection point found
	vec3 interPoint = r.origin + r.t * r.direction;
	vec3 interNormal = r.obj->NormalAt( interPoint );

	Color albedo = r.obj->ColorAt( materials, interPoint );
	Color BRDF = albedo * INVPI;
	float angle = -dot( r.direction, interNormal );
	bool backfacing = angle < 0.0f;
	if ( backfacing )
	{
		interNormal *= -1;
		angle *= -1;
	}

	Material* mat = default_material;
	if (r.obj->material >= 0) 
		mat = &materials[r.obj->material];

	// Save data for filtering
	if (depth == 0)
	{
		PixelData &pixel = pixelData[pixelId];
		pixel.interNormal = interNormal;
		pixel.firstIntersect = interPoint;
		pixel.materialIndex = r.obj->material;
		pixel.albedo = albedo;

		albedo = Color(1, 1, 1);
		BRDF = albedo * INVPI;
	}

	bool reflect = false;
	bool refract = false;

	if (mat->HasRefraction())
	{
		refract = RandomFloat() < mat->GetRefraction();
	} else if (mat->HasReflection())
	{
		reflect = RandomFloat() < mat->GetReflection();
	}

	if (refract) {
		float n = mat->GetIoR();
		if (backfacing) n = 1.0f / n;
		float k = 1 - ( n * n * ( 1 - angle * angle ) );

		if (k < 0)
		{
			reflect = true;
		} else {
			// Calculate the refractive ray, and its color
			vec3 refractDir = n * r.direction + interNormal * ( n * angle - sqrtf( k ) );
			refractDir.normalize();
			Ray refractiveRay( interPoint, refractDir );
			refractiveRay.Offset( 1e-3 );
			// Does the specular bool need to be here true?
			return albedo * Sample( refractiveRay, true, depth + 1, pixelId );
		}
	}

	if (reflect)
	{
		// Total Internal Reflection
		angle *= -1; // TODO: why does inverting the angle fix it?
		Ray reflectRay = Ray( interPoint, r.direction );
		reflectRay.Reflect( interPoint, interNormal, angle );
		reflectRay.Offset( 1e-3 );
		Color reflectCol = Sample( reflectRay, true, depth + 1, pixelId );
		return albedo * reflectCol;
	}

	#ifdef USENEE
	// Direct light for NEE
	Light *rLight = lights[RandomIndex( nr_lights )];
	vec3 rLightPoint = rLight->PointOnLight();
	vec3 rLightNormal = rLight->NormalAt(rLightPoint);
	vec3 rLightDir = rLightPoint - interPoint;
	float rLightDist = rLightDir.length();
	rLightDir *= 1 / rLightDist;

	Color rLightCol = Color(0, 0, 0);
	float cos_i = interNormal.dot(rLightDir);
	float cos_o = rLightNormal.dot(-rLightDir);
	if (cos_i > 0 && cos_o > 0)
	{
		Ray rLightRay = Ray( interPoint, rLightDir );
		rLightRay.t = rLightDist;
		if (!CheckOcclusion(&rLightRay))
		{
			float rLightArea = rLight->Area();
			float solidAngle = (cos_o * rLightArea) / (rLightDist * rLightDist);
			rLightCol = rLight->color * solidAngle * BRDF * cos_i;
		}
	}
	#endif

	// Random bounce
	Ray randomRay = Ray( interPoint, RandomPointOnHemisphere( 1, interNormal ) );
	randomRay.Offset(1e-3);

	// irradiance
	Color ei = Sample( randomRay, false, depth + 1, pixelId ) * dot( interNormal, randomRay.direction );
	Color result = PI * 2.0f * BRDF * ei;

	#ifdef USENEE
	result += rLightCol;
	#endif

	return result;
}

void Game::GenerateGaussianKernel( float sigma )
{
	std::cout << "Generating kernel with size " << KERNEL_SIZE << 'x' << KERNEL_SIZE << "..." << std::endl;
	if (kernel != nullptr)
		free(kernel);
	kernel = new float[KERNEL_SIZE * KERNEL_SIZE];
	//float sigma = 10.0;
	float r, s = 2.0 * sigma * sigma;
	for ( int x = -KERNEL_CENTER; x <= KERNEL_CENTER; x++ )
	{
		for ( int y = -KERNEL_CENTER; y <= KERNEL_CENTER; y++ )
		{
			int kernel_id = ( x + KERNEL_CENTER ) + ( y + KERNEL_CENTER ) * KERNEL_SIZE;
			r = sqrt( x * x + y * y );
			kernel[kernel_id] = ( expf( -( r * r ) / s ) ) / ( PI * s );
		}
	}
}

float ComputeWeightRaw(const float sigma, const float value)
{
	return expf( -value / (2 * sigma * sigma) );
}

float ComputeWeight(const float sigma, const float a, const float b)
{
	const float value = a - b;
	return ComputeWeightRaw(sigma, value*value);
}

float ComputeWeight_Distance(const float sigma, const vec3 a, const vec3 b)
{
	return ComputeWeightRaw(sigma, (a - b).sqrLentgh());
}

// a and b are assumed to be normalized.
float ComputeWeight_Angle(const float sigma, const vec3 a, const vec3 b)
{
	// A large dot product implies a small difference and vica verse, hence we invert the value
	const float value = 1 - dot(a, b);
	return ComputeWeightRaw(sigma, value*value);
}

Color Game::Filter( uint pixelId, int pixelX, int pixelY )
{
	PixelData &centerPixel = pixelData[pixelId];
#if KERNEL_SIZE > 0

	// Compute weights
	float weights[KERNEL_SIZE * KERNEL_SIZE];
	float totalWeight = 0;
	for ( size_t y = 0; y < KERNEL_SIZE; y++ )
		for ( size_t x = 0; x < KERNEL_SIZE; x++ )
		{
			size_t kernel_id = x + y * KERNEL_SIZE;
			int x2 = x - KERNEL_CENTER;
			int y2 = y - KERNEL_CENTER;
			// If pixel is out of screen
			if ((pixelX + x2) < 0 || (pixelY + y2) < 0 || (pixelX + x2) >= screen->GetWidth() || (pixelY + y2) >= screen->GetHeight())
			{
				weights[kernel_id] = 0.0f;
				continue;
			}
			size_t otherPixelId = pixelId + x2 + y2 * screen->GetWidth();
			PixelData &otherPixel = pixelData[otherPixelId];

			// This is the first part of the formula, i.e. the part that uses P_i and P_j.
			float weight = kernel[kernel_id];

			// Illumination difference
			weight *= ComputeWeight(25.0f, otherPixel.color.Max(), centerPixel.color.Max());
			// weight *= ComputeWeight_Distance(25.0f, otherPixel.color.ToVec(), centerPixel.color.ToVec());

			// Intersection point distance
			weight *= ComputeWeight_Distance(2.0f, otherPixel.firstIntersect, centerPixel.firstIntersect);

			// Intersection normal angle
			weight *= ComputeWeight_Angle(0.5f, otherPixel.interNormal, centerPixel.interNormal);

			// Material index difference
			// If the materials are the same the bool will be true, and thus the total value will be 0
			// If they are not the same the value will evaluate to 1.
			// We do not need to do a square as the values can only be 0 or 1, thus we can call raw.
			weight *= ComputeWeightRaw(0.5f, 1 - (otherPixel.materialIndex == centerPixel.materialIndex));

			// I am unsure how to implement this yet.
			// float BRDFWeight = centerPixel.BRDF.dot( otherPixel.BRDF );

			weights[kernel_id] = weight;
			totalWeight += weight;
		}

	// Apply kernel
	Color result = Color( 0, 0, 0 );
	for ( size_t y = 0; y < KERNEL_SIZE; y++ )
		for (size_t x = 0; x < KERNEL_SIZE; x++)
		{
			if ( weights[x + y * KERNEL_SIZE] == 0 ) continue;

			result += weights[x + y * KERNEL_SIZE] * pixelData[pixelId + (x - KERNEL_CENTER) + (y - KERNEL_CENTER) * screen->GetWidth()].color;
		}

	return result * (1 / totalWeight);

#else
	return centerPixel.color;
#endif
}
void Game::Print(size_t buflen, uint yline, const char *fmt, ...) {
	char buf[128];
	va_list va;
    va_start(va, fmt);
    vsprintf(buf, fmt, va);
    va_end(va);
	screen->Print(buf, 2, 2 + yline * 7, 0xffff00);
}

// floats for anti aliasing
float randArray[8] = {
	Rand( 1 ),
	Rand( 1 ),
	Rand( 1 ),
	Rand( 1 ),
	Rand( 1 ),
	Rand( 1 ),
	Rand( 1 ),
	Rand( 1 ) };

Ray ComputePrimaryRay(Surface* screen, Camera* view, int x, int y, size_t i, size_t j)
{
	float u = x, v = y;

#ifdef SSAA
	u += randArray[i];
	v += randArray[i + j];
#else
#ifdef USESTRATIFICATION
	u += RandomFloat();
	v += RandomFloat();
#endif
#endif

	u /= screen->GetWidth();
	v /= screen->GetHeight();
	vec3 dir = view->topLeft + u * view->right + v * view->down;
	dir.normalize();
	return Ray( view->position, dir );
}

// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
void Game::Tick()
{
	timer::TimePoint dt = timer::get();

	#ifdef USEVIGNETTING
		int dist_x_max = screen->GetWidth() / 2;
		int dist_y_max = screen->GetHeight() / 2;
		float dist_total_max = 1 / sqrtf(dist_x_max * dist_x_max + dist_y_max * dist_y_max);
	#endif
	
	unmoved_frames++;
	// uncomment to render just one frame 
	//if (unmoved_frames > 1) return;

	float sigma = 10.0;
	float r, s = 2.0 * sigma * sigma;
	for ( int x = -KERNEL_CENTER; x <= KERNEL_CENTER; x++ )
	{
		for ( int y = -KERNEL_CENTER; y <= KERNEL_CENTER; y++ )
		{
			int kernel_id = ( x + KERNEL_CENTER ) + ( y + KERNEL_CENTER ) * KERNEL_SIZE;
			r = sqrt( x * x + y * y );
			kernel[kernel_id] = ( exp( -( r * r ) / s ) ) / ( PI * s );
		}
	}
	#pragma omp parallel for schedule( dynamic ) num_threads(8)
	for (int y = 0; y < screen->GetHeight(); y++)
	for (int x = 0; x < screen->GetWidth(); x++)
	{
		uint id = x + y * screen->GetWidth();

		#ifdef SSAA
			// 4 rays with random offsett, then compute average
			Color color(0, 0, 0);
			for ( size_t i = 0; i < 4; i++ )
			{
				Ray r = ComputePrimaryRay(screen, view, x, y, i, i + 4);
				Color rayColor = Sample( r, true, 0 );
				color += rayColor;
			}
			color *= 0.25;
		#else
			Ray r = ComputePrimaryRay(screen, view, x, y, 0, 0);
			Color color = Sample( r, true, 0, id );
			//if ( !isinf( r.t ) )
			//	firstInters[id] = r.origin + r.t * r.direction;
		#endif

		pixelColor[id] += color;
		pixelData[id].color = pixelColor[id] * (1.0f / unmoved_frames);
	}

	// Apply filter technique
	#pragma omp parallel for schedule( dynamic ) num_threads(8)
	for (int y = 0; y < screen->GetHeight(); y++)
	for (int x = 0; x < screen->GetWidth(); x++)
	{
		uint id = x + y * screen->GetWidth();

		Color result = Filter( id, x, y );

		result *= pixelData[id].albedo;

		result.GammaCorrect();
		//color.ChromaticAbberation( { u, v } );

		#ifdef USEVIGNETTING
			result.Vignetting( ( x - screen->GetWidth() / 2 ), ( y - screen->GetHeight() / 2 ), dist_total_max );
		#endif

		screen->GetBuffer()[id] = result.ToPixel();
	}

	// Write debug output
	Print(32, 0, "Pos: %f %f %f", view->position.x, view->position.y, view->position.z);
	
	Print(32, 1, "Dir: %f %f %f", view->direction.x, view->direction.y, view->direction.z);
	
	Print(32, 2, "Rgt: %f %f %f", view->right.x, view->right.y, view->right.z);
	
	Print(32, 3, "Dwn: %f %f %f", view->down.x, view->down.y, view->down.z);
	
	float elapsed = timer::elapsed(dt);
	frames_time += elapsed;
	if (frames_time > 500)
	{
		frames_fps = 1000.0f / elapsed;
		frames_time = 0;
		std::cout << "FPS: " << frames_fps << std::endl;
	}

	Print(32, 4, "FPS: %f", frames_fps);
}

void Game::CameraChanged()
{
	unmoved_frames = 0;
	int max = screen->GetWidth() * screen->GetHeight();
	for ( int i = 0; i < max; i++ )
		pixelColor[i] = Color(0.0f, 0.0f, 0.0f);
}
