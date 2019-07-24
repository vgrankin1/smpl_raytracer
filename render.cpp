
#define _USE_MATH_DEFINES

#include <cmath>
#include <algorithm>
#include "geometry.h"



inline unsigned toColori(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255)
{
	return (r << 24) + (g << 16) + (b << 8) + a;
}
inline unsigned toColor(const float r, const float g, const float b, const float a = 1.0)
{
	return toColori(uint8_t(255 * std::max(0.0f, std::min(1.0f, r)) ),
					uint8_t(255 * std::max(0.0f, std::min(1.0f, g)) ),
					uint8_t(255 * std::max(0.0f, std::min(1.0f, b)) ));
}
inline unsigned toColor(const Vec3f &color)
{
	return toColor(color.r, color.g, color.b);
}


struct Light
{
	Light(const Vec3f &position, const float& intensity)
		: position(position), intensity(intensity) {}
	Vec3f position;
	float intensity;
};

/*
Material(const Vec2f& a, const Vec3f& color, const float& spec) : albedo(a), diffuse_color(color), specular_exponent(spec) {}
Material() : albedo(1, 0), diffuse_color(), specular_exponent() {}
Vec2f albedo;
Vec3f diffuse_color;
float specular_exponent;*/


struct Material
{
	Material(const Vec3f &albedo, const Vec3f& diffuse, const float specular) 
			: albedo(albedo), diffuse(diffuse), specular_exponent(specular) {}
	Material() : albedo(1, 0, 0), diffuse(), specular_exponent() {}
	Vec3f albedo;
	Vec3f diffuse;//diffuse color
	float specular_exponent;
};

struct Sphere
{
	Vec3f p;
	float r;
	Material material;

	Sphere(const Vec3f &position, const float &radius, const Material &material)
		: p(position), r(radius), material(material) {}

	bool ray_intersect(const Vec3f& orig, const Vec3f& dir, float& t0) const
	{
		Vec3f L = p - orig;
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
};

Vec3f reflect(const Vec3f& I, const Vec3f& N)
{
	return I - N * 2.f * (I * N);
}

bool scene_intersect(const Vec3f& orig, const Vec3f& dir, const std::vector<Sphere>& spheres, Vec3f& hit, Vec3f& N, Material &material)
{
	float spheres_dist = std::numeric_limits<float>::max();
	for (size_t i = 0; i < spheres.size(); i++) {
		float dist_i;
		if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < spheres_dist) {
			spheres_dist = dist_i;
			hit = orig + dir * dist_i;
			N = (hit - spheres[i].p).normalize();
			material = spheres[i].material;
		}
	}
	return spheres_dist < 1000;
}

Vec3f cast_ray(const Vec3f& orig, const Vec3f& dir, const std::vector<Sphere> &spheres, const std::vector<Light> &lights, size_t depth = 0)
{
	Vec3f point, N;
	Material material;

	if (depth > 4 || !scene_intersect(orig, dir, spheres, point, N, material))
	{
		return Vec3f(0.2f, 0.7f, 0.8f); // background color
	}

	Vec3f reflect_dir = reflect(dir, N).normalize();
	Vec3f reflect_orig = reflect_dir * N < 0 ? point - N * 1e-3f : point + N * 1e-3f; // offset the original point to avoid occlusion by the object itself
	Vec3f reflect_color = cast_ray(reflect_orig, reflect_dir, spheres, lights, depth + 1);


	float diffuse_light_intensity = 0, specular_light_intensity = 0;
	for (size_t i = 0; i < lights.size(); i++)
	{
		Vec3f light_dir = (lights[i].position - point).normalize();
		float light_distance = (lights[i].position - point).norm();

		Vec3f shadow_orig = light_dir * N < 0 ? point - N * 1e-3f : point + N * 1e-3f; // checking if the point lies in the shadow of the lights[i]
		Vec3f shadow_pt, shadow_N;
		Material tmpmaterial;
		if (scene_intersect(shadow_orig, light_dir, spheres, shadow_pt, shadow_N, tmpmaterial) && (shadow_pt - shadow_orig).norm() < light_distance)
			continue;
		diffuse_light_intensity += lights[i].intensity * std::max(0.f, light_dir * N);
		specular_light_intensity += powf(std::max(0.f, -reflect(-light_dir, N) * dir), material.specular_exponent) * lights[i].intensity;
	}
	return material.diffuse * diffuse_light_intensity * material.albedo[0] + Vec3f(1., 1., 1.) * specular_light_intensity * material.albedo[1] + reflect_color * material.albedo[2];
}



const float fov = float(M_PI) / 2.f;

Material      ivory(Vec3f(0.6f, 0.3f, 0.1f), Vec3f(0.4f, 0.4f, 0.3f), 50.0f);
Material red_rubber(Vec3f(0.9f, 0.1f, 0.1f), Vec3f(0.3f, 0.1f, 0.1f), 10.0f);
Material     mirror(Vec3f(0.0f, 10.0f, 0.8f), Vec3f(1.0f, 1.0f, 1.0f), 1425.f);



void render(std::vector<unsigned>& framebuffer, const int width, const int height)
{
	std::vector<Sphere> spheres;
	spheres.push_back(Sphere(Vec3f(-3, 0, -16), 2, ivory));
	spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2, red_rubber));
	spheres.push_back(Sphere(Vec3f(1.5, -0.5, -18), 3, mirror));
	spheres.push_back(Sphere(Vec3f(7, 5, -18), 4, mirror));

	std::vector<Light>  lights;
	lights.push_back(Light(Vec3f(-20, 20, 20), 1.5f));
	lights.push_back(Light(Vec3f(30, 50, -25), 1.8f));
	lights.push_back(Light(Vec3f(30, 20, 30), 1.7f));

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
