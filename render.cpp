#define _USE_MATH_DEFINES

#include <cmath>
#include <algorithm>
#include "geometry.hpp"
#include "util.hpp"




bool Sphere::ray_intersect(const Vec3f& orig, const Vec3f& dir, float& t0) const
{
	Vec3f L = position - orig;
	float tca = L * dir;
	float d2 = L * L - tca * tca;
	if (d2 > r * r) return false;
	float thc = sqrtf(r * r - d2);
	t0 = tca - thc;
	float t1 = tca + thc;
	if (t0 < 0) t0 = t1;
	if (t0 < 0) return false;
	return true;
}

Vec3f reflect(const Vec3f& I, const Vec3f& N)
{
	return I - N * 2.f * (I * N);
}

Vec3f refract(const Vec3f& I, const Vec3f& N, const float eta_t, const float eta_i = 1.f)// Snell's law
{ 
	float cosi = -std::max(-1.f, std::min(1.f, I * N));
	if (cosi < 0) return refract(I, -N, eta_i, eta_t); // if the ray comes from the inside the object, swap the air and the media
	float eta = eta_i / eta_t;
	float k = 1 - eta * eta * (1 - cosi * cosi);
	return k < 0 ? Vec3f(1, 0, 0) : I * eta + N * (eta * cosi - sqrtf(k)); // k<0 = total reflection, no ray to refract. I refract it anyways, this has no physical meaning
}

	
bool scene_intersect(const Vec3f& orig, const Vec3f& dir, const std::vector<Sphere>& spheres, Vec3f& hit, Vec3f& N, Material &material)
{
	float spheres_dist = std::numeric_limits<float>::max();
	for (size_t i = 0; i < spheres.size(); i++) {
		float dist_i;
		if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < spheres_dist) {
			spheres_dist = dist_i;
			hit = orig + dir * dist_i;
			N = (hit - spheres[i].position).normalize();
			material = spheres[i].material;
		}
	}

	float checkerboard_dist = std::numeric_limits<float>::max();
	if (fabs(dir.y) > 1e-3) {
		float d = -(orig.y + 4) / dir.y; // the checkerboard plane has equation y = -4
		Vec3f pt = orig + dir * d;
		if (d > 0 && fabs(pt.x) < 10 && pt.z<-10 && pt.z>-30 && d < spheres_dist) {
			checkerboard_dist = d;
			hit = pt;
			N = Vec3f(0, 1, 0);
			material.diffuse = (int(.5 * hit.x + 1000) + int(.5 * hit.z)) & 1 ? Vec3f(.3, .3, .3) : Vec3f(.3, .2, .1);
		}
	}
	return std::min(spheres_dist, checkerboard_dist) < 1000;
}

const std::vector<Vec3f> *envmap;
uint32_t envmap_width, envmap_height;

Vec3f cast_ray(const Vec3f& orig, const Vec3f& dir, const std::vector<Sphere>& spheres, const std::vector<Light_t>& lights, size_t depth = 0)
{
	Vec3f point, N;
	Material material;

	if (depth > 4 || !scene_intersect(orig, dir, spheres, point, N, material))
	{
		size_t u = envmap_width * (0.5 + atan2(dir.z, dir.x) / (2 * M_PI));
		size_t v = envmap_height * (0.5 - asin(dir.y) / M_PI);
		return (*envmap)[u + v * envmap_width];
		//return Vec3f(0.2, 0.7, 0.8); // background color
	}

	//Reflections
	Vec3f reflect_dir = reflect(dir, N).normalize();
	Vec3f reflect_orig = reflect_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3; // offset the original point to avoid occlusion by the object itself
	Vec3f reflect_color = cast_ray(reflect_orig, reflect_dir, spheres, lights, depth + 1);
	//Refractions
	Vec3f refract_dir = refract(dir, N, material.refractive).normalize();
	Vec3f refract_orig = refract_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3;
	Vec3f refract_color = cast_ray(refract_orig, refract_dir, spheres, lights, depth + 1);

	float diffuse_light_intensity = 0, specular_light_intensity = 0;
	for (size_t i = 0; i < lights.size(); i++)
	{
		Vec3f light_dir = (lights[i].position - point).normalize();
		float light_distance = (lights[i].position - point).norm();

		//Shadows
		Vec3f shadow_orig = light_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3; // checking if the point lies in the shadow of the lights[i]
		Vec3f shadow_pt, shadow_N;
		Material tmpmaterial;
		if (scene_intersect(shadow_orig, light_dir, spheres, shadow_pt, shadow_N, tmpmaterial) && (shadow_pt - shadow_orig).norm() < light_distance)
			continue;

		diffuse_light_intensity += lights[i].intensity * std::max(0.f, light_dir * N);
		specular_light_intensity += powf(std::max(0.f, -reflect(-light_dir, N) * dir), material.specular_exponent) * lights[i].intensity;
	}

	return material.diffuse * diffuse_light_intensity * material.albedo[0] + 
		Vec3f(1., 1., 1.) * specular_light_intensity * material.albedo[1] +
		reflect_color * material.albedo[2] + refract_color * material.albedo[3];
}

const float fov = M_PI / 2.;

Material      ivory(Vec4f(0.6f, 0.3f, 0.1f, 0.0f), Vec3f(0.4f, 0.4f, 0.3f), 50.0f, 1.0);
Material red_rubber(Vec4f(0.9f, 0.1f, 0.1f, 0.0f), Vec3f(0.3f, 0.1f, 0.1f), 10.0f, 1.0);
Material     mirror(Vec4f(0.0f, 10.0f, 0.8f, 0.0f), Vec3f(1.0f, 1.0f, 1.0f), 1425.f, 1.0);
Material      glass(Vec4f(0.0, 0.5, 0.1, 0.8), Vec3f(0.6, 0.7, 0.8), 125., 1.5);


void render(std::vector<unsigned>& framebuffer, const int width, const int height, const std::vector<Vec3f> *envmap, const int env_width, const int env_height)
{
	::envmap = envmap;
	::envmap_width = env_width;
	::envmap_height = env_height;

	std::vector<Sphere> spheres;
	spheres.push_back(Sphere(Vec3f(-3, 0, -16), 2, ivory));
	spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, glass));
	spheres.push_back(Sphere(Vec3f(1.5, -0.5, -18), 3, red_rubber));
	spheres.push_back(Sphere(Vec3f(7, 5, -18), 4, mirror));

	std::vector<Light_t> lights;
	lights.push_back(Light_t(Vec3f(-20, 20, 20), 1.5f) );
	lights.push_back(Light_t(Vec3f(30, 50, -25), 1.8f) );
	lights.push_back(Light_t(Vec3f(30, 20, 30), 1.7f) );

	for (size_t j = 0; j < height; j++)
	{
		for (size_t i = 0; i < width; i++)
		{
			float x = (2 * (i + 0.5f) / float(width) - 1 ) * tan(fov / 2.0f) * width / float(height);
			float y =-(2 * (j + 0.5f) / float(height) - 1 ) * tan(fov / 2.0f);
			Vec3f dir = Vec3f(x, y, -1).normalize();
			framebuffer[i + j * width] = toColor(cast_ray(Vec3f(0, 0, 0), dir, spheres, lights));
		}
	}
}

/*
std::vector<uint64_t> render2(const int width, const int height )
{

}*/