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

	
bool scene_intersect(const Vec3f& orig, const Vec3f& dir, const Scene_t &scene, Vec3f& hit, Vec3f& N, Material& material)
{
	float spheres_dist = std::numeric_limits<float>::max();
	float duck_dist = std::numeric_limits<float>::max();
	for (int j = 0; j < scene.objects.size(); j++)
	{
		const SceneObject_t* sc_obj = scene.objects[j];
		const Sphere* sphere = dynamic_cast<const Sphere*>(sc_obj);
		const Model *model = dynamic_cast<const Model*>(sc_obj);
		if (sphere)
		{
			float dist_i;
			if (sphere->ray_intersect(orig, dir, dist_i) && dist_i < spheres_dist)
			{
				spheres_dist = dist_i;
				hit = orig + dir * dist_i;
				N = (hit - sphere->position).normalize();
				material = sphere->material;
			}
		}
		else if (model)
		{
			for (int t = 0; t < model->nfaces(); t++)
			{
				float dist;
				if (model->ray_triangle_intersect(t, orig, dir, dist) && dist < duck_dist && dist < spheres_dist)
				{
					duck_dist = dist;
					hit = orig + dir * dist;
					Vec3f v0 = model->point(model->vert(t, 0));
					Vec3f v1 = model->point(model->vert(t, 1));
					Vec3f v2 = model->point(model->vert(t, 2));
					N = cross(v1 - v0, v2 - v0).normalize();
					material = Material(Vec4f(0.3, 1.5, 0.2, 0.5), Vec3f(.24, .21, .09), 125., 1.5);
				}
			}
		}
	}

	float checkerboard_dist = std::numeric_limits<float>::max();
	if (fabs(dir.y) > 1e-3)
	{
		float d = -(orig.y + 4) / dir.y; // the checkerboard plane has equation y = -4
		Vec3f pt = orig + dir * d;
		if (d > 0 && fabs(pt.x) < 10 && pt.z<-10 && pt.z>-30 && d < spheres_dist && d < duck_dist)
		{
			checkerboard_dist = d;
			hit = pt;
			N = Vec3f(0, 1, 0);
			material.diffuse = (int(.5 * hit.x + 1000) + int(.5 * hit.z)) & 1 ? Vec3f(.3, .3, .3) : Vec3f(.3, .2, .1);
		}
	}
	return std::min(duck_dist, std::min(spheres_dist, checkerboard_dist)) < 1000;
}


Vec3f cast_ray(const Vec3f& orig, const Vec3f& dir, const Scene_t &scene, size_t depth = 0)
{
	Vec3f point, N;
	Material material;
	const std::vector<const Light_t*>& lights = scene.lights;
	

	if (depth > 4 || !scene_intersect(orig, dir, scene, point, N, material))
	{
		size_t u = scene.penvmap->width * (0.5 + atan2(dir.z, dir.x) / (2 * M_PI));
		size_t v = scene.penvmap->height * (0.5 - asin(dir.y) / M_PI);
		size_t i = u + v * scene.penvmap->width;//return (*envmap)[u + v * envmap_width];
		unsigned char* pixel = scene.penvmap->pixmap;
		return Vec3f(pixel[3 * i + 0], pixel[3 * i + 1], pixel[3 * i + 2]) * (1.f / 255.f);
		//return Vec3f(0.2, 0.7, 0.8); // background color
	}

	//Reflections
	Vec3f reflect_dir = reflect(dir, N).normalize();
	Vec3f reflect_orig = reflect_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3; // offset the original point to avoid occlusion by the object itself
	Vec3f reflect_color = cast_ray(reflect_orig, reflect_dir, scene, depth + 1);
	//Refractions
	Vec3f refract_dir = refract(dir, N, material.refractive).normalize();
	Vec3f refract_orig = refract_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3;
	Vec3f refract_color = cast_ray(refract_orig, refract_dir, scene, depth + 1);

	float diffuse_light_intensity = 0, specular_light_intensity = 0;
	for (size_t i = 0; i < lights.size(); i++)
	{
		Vec3f light_dir = (lights[i]->position - point).normalize();
		float light_distance = (lights[i]->position - point).norm();

		//Shadows
		Vec3f shadow_orig = light_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3; // checking if the point lies in the shadow of the lights[i]
		Vec3f shadow_pt, shadow_N;
		Material tmpmaterial;
		if (scene_intersect(shadow_orig, light_dir, scene, shadow_pt, shadow_N, tmpmaterial) && (shadow_pt - shadow_orig).norm() < light_distance)
			continue;

		diffuse_light_intensity += lights[i]->intensity * std::max(0.f, light_dir * N);
		specular_light_intensity += powf(std::max(0.f, -reflect(-light_dir, N) * dir), material.specular_exponent) * lights[i]->intensity;
	}

	return material.diffuse * diffuse_light_intensity * material.albedo[0] + 
			Vec3f(1., 1., 1.) * specular_light_intensity * material.albedo[1] +
		reflect_color * material.albedo[2] + refract_color * material.albedo[3];
}


const float fov = M_PI / 2.;


void render2(Scene_t *scene, render_state_t *rstate, const int worker_id)
{
	unsigned int width = rstate->pwindow->width;
	unsigned int height = rstate->pwindow->height;
	unsigned prand_seed = 0;


	for (unsigned p = 0; p < width * height; p += rstate->workers_num)
	{
		unsigned i, j;
		for(; ; )
		{
			prand_seed = mrand_1080n(prand_seed);
			if (prand_seed >= width * height)
				continue;
			if(prand_seed % rstate->workers_num == worker_id)
				break;
		}

		i = prand_seed % width;
		j = (prand_seed - i) / width;

		float x = (2 * (i + 0.5f) / float(width) - 1) * tan(fov / 2.0f) * width / float(height);
		float y = -(2 * (j + 0.5f) / float(height) - 1) * tan(fov / 2.0f);
		Vec3f dir = Vec3f(x, y, -1).normalize();

		unsigned int pixcolor = toColor(cast_ray(Vec3f(0, 0, 0), dir, *scene));
		unsigned int pixindex = i + j * width;
		unsigned long long packed = ((unsigned long long)pixindex << 32) + pixcolor;

		rstate->mx.lock();
		rstate->pixels.push_back(packed);
		if (rstate->terminate)
		{
			rstate->mx.unlock();
			break;
		}
		rstate->mx.unlock();
	}
}
