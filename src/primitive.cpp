#include <math.h>       /* sqrt */

#include "precomp.h" // include (only) this in every .cpp file
#include "primitive.h"

bool Primitive::Intersect(Ray* r)
{
    float t = IntersectionDistance(r);
    if (t <= 0 || t >= r->t) return false;
    r->t = t;
    return true;
}

bool Primitive::Occludes(Ray* r)
{
    float t = IntersectionDistance(r);
    if (t <= 0 || t >= r->t) return false;
    return true;
}

Color Primitive::ColorAt( vec3 point )
{
    if (texture == nullptr) {
        return color;
    }
    int idx = TextureAt(point);
    if (idx < 0 || idx >= texture->GetWidth() * texture->GetHeight())
        return DEFAULT_OBJECT_COLOR;
    return texture->GetBuffer()[idx];
}

Plane::Plane( vec3 n, float d, Color c, Material* m ) :
    Primitive( c, m ),
	normal( n.normalized() ),
	dist( d )
{
}

float Plane::IntersectionDistance(Ray* r) 
{
    return -(dot(r->origin, normal) + dist) / dot(r->direction, normal);
}

vec3 Plane::NormalAt( vec3 point )
{
	return normal;
}

int Plane::TextureAt ( vec3 point )
{
    return -1;
}

Sphere::Sphere( vec3 p, float r, Color c, Material* m ) :
    Primitive( c, m ),
	position( p ),
	radius( r )
{
}

float Sphere::IntersectionDistance(Ray* r)
{
    vec3 C = position - r->origin;
    float t = dot(C, r->direction);
    vec3 Q = C - t * r->direction;
    float p2 = dot(Q, Q);
    float r2 = radius * radius;
    if (p2 > r2) return -1;
    t -= sqrt(r2 - p2);
    return t;
}

vec3 Sphere::NormalAt( vec3 point )
{
	return (1 / radius) * (point - position);
}

int Sphere::TextureAt ( vec3 point )
{
    vec3 direction = point - position;
    float u = (1 + atan2f( direction.x, -direction.z ) * INVPI) / 2;
    float v = acosf( direction.y ) * INVPI;
    uint x = texture->GetWidth() * u;
    uint y = texture->GetHeight() * v;
    return y * texture->GetWidth() + x;
}

Triangle::Triangle( vec3 v0, vec3 v1, vec3 v2, vec3 n, Color c, Material* m ) :
    Primitive( c, m ),
	p0( v0 ),
	p1( v1 ),
	p2( v2 ),
	normal( n ),
	t0(0, 0), 
	t1(1, 0), 
	t2(0, 1)
{
}

float Triangle::IntersectionDistance(Ray* r)
{
    // Implementation from:
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
    vec3 p0p1 = p1 - p0;
    vec3 p0p2 = p2 - p0;
    vec3 pvec = r->direction.cross(p0p2);
    float det = p0p1.dot(pvec);

    // ray and triangle are parallel if det is close to 0
    // This should probaby be something smaller, but for now it will do
	if ( fabs( det ) < 0.0000001 ) return -1;

    float invDet = 1 / det;

    vec3 tvec = r->origin - p0;
    float u = tvec.dot(pvec) * invDet;
    if (u < 0 || u > 1) return -1;

    vec3 qvec = tvec.cross(p0p1);
    float v = r->direction.dot(qvec) * invDet;
    if (v < 0 || u + v > 1) return -1;

    float t = p0p2.dot(qvec) * invDet;

    return t;
}

vec3 Triangle::NormalAt( vec3 point )
{
	return normal;
}

vec3 Triangle::ComputeNormal( vec3 v0, vec3 v1, vec3 v2 )
{
    return cross(v1 - v0, v2 - v0).normalized();
}

int Triangle::TextureAt( vec3 point )
{
    // TODO: triangles that are "upside down" result in bad values
    vec3 p0p1 = p1 - p0;
    vec3 p0p2 = p2 - p0;
    point = point - p0;

    p0p1.normalized();
    p0p2.normalized();
    point.normalized();

    float u = dot(p0p1, point);
    float v = dot(p0p2, point);

    if (u < 0)
        u *= -1;
    if (v < 0)
        v *= -1;

    if (u > 1)
        u = 1 / u;
    if (v > 1)
        v = 1 / v;

    vec2 uv = t0 + t1 * u + t2 * v;

    int x = uv.x * texture->GetWidth();
    int y = uv.y * texture->GetHeight();

    return x + y * texture->GetWidth();
}

vec3 TinyObjGetVector3(int idx, std::vector<tinyobj::real_t>* values) {
    assert(idx >= 0);
    // I think there are better ways than to use "at", but im lazy for now.
	tinyobj::real_t vx = values->at(3 * idx + 0);
	tinyobj::real_t vy = values->at(3 * idx + 1);
	tinyobj::real_t vz = values->at(3 * idx + 2);
    return vec3(vx, vy, vz);
}

vec2 TinyObjGetVector2(int idx, std::vector<tinyobj::real_t>* values) {
    assert(idx >= 0);
    // I think there are better ways than to use "at", but im lazy for now.
	tinyobj::real_t vx = values->at(2 * idx + 0);
	tinyobj::real_t vy = values->at(2 * idx + 1);
    return vec2(vx, vy);
}

void Triangle::FromTinyObj( Triangle *tri, tinyobj::attrib_t *attrib, tinyobj::mesh_t *mesh, size_t f, std::vector<tinyobj::material_t> materials, std::map<std::string, Surface*> textures )
{
    // This MUST hold for our custom Triangle implementaion, if it isnt, then this face is no triangle, but e.g. a quad.
    assert(mesh->num_face_vertices[f] == 3);

    tinyobj::index_t idx0 = mesh->indices[3 * f + 0];
    tinyobj::index_t idx1 = mesh->indices[3 * f + 1];
    tinyobj::index_t idx2 = mesh->indices[3 * f + 2];

    // Read material types from file
	if ( mesh->material_ids[f] >= 0 )
	{
        auto thismat = materials[mesh->material_ids[f]];
        std::cout << "NAME: " << thismat.name << std::endl;
		auto diffuse = thismat.diffuse;
		tri->color = Color( diffuse[0], diffuse[1], diffuse[2] );

		//Material mat = Material( thismat.specular[0], thismat.transmittance[0], thismat.emission[0] );
		//tri->material = &mat;

        auto tex = textures.find(thismat.diffuse_texname);
        if (tex != textures.end()) {
           tri->texture = tex->second;
        }
	}
	else
		tri->color = DEFAULT_OBJECT_COLOR;

    tri->p0 = TinyObjGetVector3(idx0.vertex_index, &attrib->vertices);
    tri->p1 = TinyObjGetVector3(idx1.vertex_index, &attrib->vertices);
    tri->p2 = TinyObjGetVector3(idx2.vertex_index, &attrib->vertices);

    if (idx0.texcoord_index >= 0 && idx1.texcoord_index >= 0 && idx2.texcoord_index >= 0)
    {
        tri->t0 = TinyObjGetVector2(idx0.texcoord_index, &attrib->texcoords);
        tri->t1 = TinyObjGetVector2(idx1.texcoord_index, &attrib->texcoords);
        tri->t2 = TinyObjGetVector2(idx2.texcoord_index, &attrib->texcoords);

	    if ( mesh->material_ids[f] >= 0 )
        {
            // Lets just only support diffuse
            auto matoffset = materials[mesh->material_ids[f]].diffuse_texopt.origin_offset;
            vec2 offset(matoffset[0], matoffset[1]);
            tri->t0 += offset;
            tri->t1 += offset;
            tri->t2 += offset;
        }

        // Let's make these relative to t0
        tri->t1 -= tri->t0;
        tri->t2 -= tri->t0;
    }

    // I think colors are defined on a per-vertex base.
    // Since we need only the full face normal let's just compute it ourselves.
    tri->normal = ComputeNormal(tri->p0, tri->p1, tri->p2);

    // I think colors are defined on a per-vertex base, hence I don't currently know how to handle this. 
    // Color c = (Color)TinyObjGetVector(idx.vertex_index, &attrib.colors);
}
