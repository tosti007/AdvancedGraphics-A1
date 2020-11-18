#include <math.h>       /* sqrt */

#include "precomp.h" // include (only) this in every .cpp file
#include "primitive.h"

Plane::Plane( vec3 n, float d, Color c ) :
    Primitive(c),
    normal(n.normalized()),
    dist(d)
{
}

bool Plane::Intersect(Ray* r) 
{
    float t = -(dot(r->origin, normal) + dist) / dot(r->direction, normal);

    if (t >= r->t || t <= 0) return false;
    r->t = t;
    r->color = color;
    return true;
}

Sphere::Sphere( vec3 p, float r, Color c ) :
    Primitive(c),
    position(p),
    radius(r)
{
}

bool Sphere::Intersect(Ray* r)
{
    vec3 C = position - r->origin;
    float t = dot(C, r->direction);
    vec3 Q = C - t * r->direction;
    float p2 = dot(Q, Q);
    float r2 = radius * radius;
    if (p2 > r2) return false;
    t -= sqrt(r2 - p2);
    
    if (t >= r->t || t <= 0) return false;
    r->t = t;
    r->color = color;
    return true;
}

Triangle::Triangle( vec3 v0, vec3 v1, vec3 v2, Color c) :
    Primitive(c),
    p0(v0),
    p1(v1),
    p2(v2)
{
}

bool Triangle::Intersect(Ray* r)
{
    // Implementation from:
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
    vec3 p0p1 = p1 - p0;
    vec3 p0p2 = p2 - p0;
    vec3 pvec = r->direction.cross(p0p2);
    float det = p0p1.dot(pvec);

    // ray and triangle are parallel if det is close to 0
    // This should probaby be something smaller, but for now it will do
    if (fabs(det) < 0.001) return false;

    float invDet = 1 / det;

    vec3 tvec = r->origin - p0;
    float u = tvec.dot(pvec) * invDet;
    if (u < 0 || u > 1) return false;

    vec3 qvec = tvec.cross(p0p1);
    float v = r->direction.dot(qvec) * invDet;
    if (v < 0 || u + v > 1) return false;

    float t = p0p2.dot(qvec) * invDet;

    if (t >= r->t || t <= 0) return false;
    r->t = t;
    r->color = color;
    return true;
}
