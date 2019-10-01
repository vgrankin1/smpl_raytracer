#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>

#include "util.hpp"

// fills verts and faces arrays, supposes .obj file to have "f " entries without slashes
Model::Model(const char *filename) : verts(), faces()
{
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail())
	{
        std::cerr << "Failed to open " << filename << std::endl;
        return;
    }
    std::string line;
    while (!in.eof())
	{
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v "))//Geometric vertices
		{
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v[i];
            verts.push_back(v);
		}
		else if (!line.compare(0, 2, "vn"))//Vertex normals
		{
			iss >> trash;
		}
		else if (!line.compare(0, 2, "f "))//Face
		{
            Vec3i f;//faces
            int idx, cnt=0;
			iss >> trash;
			int matches = 0;

			int fn0, fn1, fn2;//Normals
			int ft0, ft1, ft2;//Textures cords
			
			if ((matches = sscanf(line.c_str(), "f %d %d %d\n", &f[0], &f[1], &f[2])) != 3)
			if((matches = sscanf(line.c_str(), "f %d/%d/%d\n", &f[0], &f[1], &f[2])) != 3)
			if((matches = sscanf(line.c_str(), "f %d//%d %d//%d %d//%d\n", &f[0], &fn0, &f[1], &fn1, &f[2], &fn2)) != 6)
			if ((matches = sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d\n", &f[0], &ft0, &fn0,		&f[1], &ft1, &fn1,	&f[2], &ft2, &fn2)) != 9)
			{
				std::cerr << "Cannot read file: " << filename << std::endl;
				throw std::string("Cannot read file: ") + filename;
			}

			f[0]--;// in wavefront obj all indices start at 1, not zero
			f[1]--;
			f[2]--;
			faces.push_back(f);
        }
    }
    std::cerr << "# v# " << verts.size() << " f# "  << faces.size() << std::endl;

	calc_bbox();
}

void Model::calc_bbox()
{
	bb_min = bb_max = verts[0];
	for (size_t i = 1; i < verts.size(); ++i)
	{
		for (int j = 0; j < 3; j++)
		{
			bb_min[j] = std::min(bb_min[j], verts[i][j]);
			bb_max[j] = std::max(bb_max[j], verts[i][j]);
		}
	}
	std::cerr << "bbox: [" << bb_min << " : " << bb_max << "]" << std::endl;
}

bool Model::ray_bbox_intersect(const Vec3f& orig, const Vec3f& dir) const
{
	Vec3f p1, p2, p3, p4;//front
	Vec3f p5, p6, p7, p8;//rear
	Vec3f p9, p10, p11, p12;//left
	Vec3f p13, p14, p15, p16;//right
	float dist;
	int ret = 0;
	
	p1 = Vec3f(bb_min.x, bb_min.y, bb_min.z);
	p2 = Vec3f(bb_min.x, bb_max.y, bb_min.z);
	p3 = Vec3f(bb_max.x, bb_max.y, bb_min.z);
	p4 = Vec3f(bb_max.x, bb_min.y, bb_min.z);
	ret += RayIntersectsTriangle(orig, dir, dist, p1, p2, p3);
	ret += RayIntersectsTriangle(orig, dir, dist, p1, p4, p3);
	
	p5 = Vec3f(bb_min.x, bb_min.y, bb_max.z);
	p6 = Vec3f(bb_min.x, bb_max.y, bb_max.z);
	p7 = Vec3f(bb_max.x, bb_max.y, bb_max.z);
	p8 = Vec3f(bb_max.x, bb_min.y, bb_max.z);
	ret += RayIntersectsTriangle(orig, dir, dist, p5, p7, p6);
	ret += RayIntersectsTriangle(orig, dir, dist, p5, p8, p7);
	
	p9 = Vec3f(bb_min.x, bb_min.y, bb_min.z);
	p10 = Vec3f(bb_min.x, bb_max.y, bb_min.z);
	p11 = Vec3f(bb_min.x, bb_max.y, bb_max.z);
	p12 = Vec3f(bb_min.x, bb_min.y, bb_max.z);
	ret += RayIntersectsTriangle(orig, dir, dist, p9, p10, p11);
	ret += RayIntersectsTriangle(orig, dir, dist, p9, p11, p12);

	p13 = Vec3f(bb_max.x, bb_min.y, bb_min.z);
	p14 = Vec3f(bb_max.x, bb_max.y, bb_min.z);
	p15 = Vec3f(bb_max.x, bb_max.y, bb_max.z);
	p16 = Vec3f(bb_max.x, bb_min.y, bb_max.z);
	ret += RayIntersectsTriangle(orig, dir, dist, p13, p14, p15);
	ret += RayIntersectsTriangle(orig, dir, dist, p13, p15, p16);

	return ret;
}




bool Model::ray_triangle_intersect(const int fi, const Vec3f& orig, const Vec3f& dir, float& tnear) const
{

	return RayIntersectsTriangle(orig, dir, tnear, verts[faces[fi][0]], verts[faces[fi][1]], verts[faces[fi][2]] );
}

bool Model::ray_intersect(const Vec3f& orig, const Vec3f& dir, float &dist, Vec3f& N, Material& material) const
{
	bool intersect = false;
	for (int i = 0; i < faces.size(); i++)
	{
		Vec3f v0 = verts[faces[i][0]];
		Vec3f v1 = verts[faces[i][1]];
		Vec3f v2 = verts[faces[i][2]];
		float cur_dist;
		if (RayIntersectsTriangle(orig, dir, cur_dist, v0, v1, v2) && cur_dist < dist)
		{
			dist = cur_dist;
			intersect = true;
			N = cross(v1 - v0, v2 - v0).normalize();
			material = Material(Vec4f(0.3, 1.5, 0.2, 0.5), Vec3f(.20, .21, .2), 125., 1.5);
		}
	}
	return intersect;
}

int Model::nverts() const
{
    return (int)verts.size();
}

int Model::nfaces() const
{
    return (int)faces.size();
}

void Model::get_bbox(Vec3f &min, Vec3f &max)
{
	min = bb_min;
	max = bb_max;
}

const Vec3f &Model::point(int i) const 
{
    assert(i>=0 && i<nverts());
    return verts[i];
}

Vec3f &Model::point(int i) 
{
    assert(i>=0 && i<nverts());
    return verts[i];
}

int Model::vert(int fi, int li) const 
{
    assert(fi>=0 && fi<nfaces() && li>=0 && li<3);
    return faces[fi][li];
}

std::ostream& operator<<(std::ostream& out, Model &m) 
{
    for (int i=0; i<m.nverts(); i++) 
	{
        out << "v " << m.point(i) << std::endl;
    }
    for (int i=0; i<m.nfaces(); i++) 
	{
        out << "f ";
        for (int k=0; k<3; k++)
		{
            out << (m.vert(i,k)+1) << " ";
        }
        out << std::endl;
    }
    return out;
}


/*
	Möller–Trumbore intersection algorithm
*/
bool RayIntersectsTriangle(const Vec3f& orig, const Vec3f& dir, float& dist, const Vec3f& v0, const Vec3f& v1, const Vec3f& v2)
{
	Vec3f pvec, tvec, q;
	Vec3f edge1 = v1 - v0;
	Vec3f edge2 = v2 - v0;
	const float eps = 1e-7;
	float det, u, v;

	pvec = cross(dir, edge2);//vector normal to plane 
	det = edge1 * pvec;
	if (det > -eps && det < eps)// This ray is parallel to this triangle.
		return false;
	det = 1. / det;

	tvec = orig - v0;
	u = det * (tvec * pvec);
	if (u < 0.0 || u > 1.0)
		return false;

	q = cross(tvec, edge1);
	v = det * (dir * q);
	if (v < 0.0 || u + v > 1.0)
		return false;

	// At this stage we can compute t to find out where the intersection point is on the line.
	dist = det * (edge2 * q);
	//IntersectionPoint = rayOrigin + dir * dist;
	return dist > eps;// This means that there is a line intersection but not a ray intersection.
}