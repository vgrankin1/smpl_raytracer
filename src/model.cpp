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
            /**//*
			while (iss >> idx) 
			{
                idx--; // in wavefront obj all indices start at 1, not zero
                f[cnt++] = idx;
            }
            if (3==cnt) faces.push_back(f);/**/
			/**/
			f[0]--;// in wavefront obj all indices start at 1, not zero
			f[1]--;
			f[2]--;
			faces.push_back(f);
        }
    }
    std::cerr << "# v# " << verts.size() << " f# "  << faces.size() << std::endl;

    Vec3f min, max;
    get_bbox(min, max);
}

// Moller and Trumbore
bool Model::ray_triangle_intersect(const int &fi, const Vec3f &orig, const Vec3f &dir, float &tnear) const
{
    Vec3f edge1 = point(vert(fi,1)) - point(vert(fi,0));
    Vec3f edge2 = point(vert(fi,2)) - point(vert(fi,0));
    Vec3f pvec = cross(dir, edge2);
    float det = edge1*pvec;
    if (det<1e-5) return false;

    Vec3f tvec = orig - point(vert(fi,0));
    float u = tvec*pvec;
    if (u < 0 || u > det) return false;

    Vec3f qvec = cross(tvec, edge1);
    float v = dir*qvec;
    if (v < 0 || u + v > det) return false;

    tnear = edge2*qvec * (1./det);
    return tnear>1e-5;
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
    min = max = verts[0];
    for (int i=1; i<(int)verts.size(); ++i) 
	{
        for (int j=0; j<3; j++) 
		{
            min[j] = std::min(min[j], verts[i][j]);
            max[j] = std::max(max[j], verts[i][j]);
        }
    }
    std::cerr << "bbox: [" << min << " : " << max << "]" << std::endl;
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

