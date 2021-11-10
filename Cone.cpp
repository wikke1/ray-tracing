#include "Cone.h"
#include <math.h>

/**
* Cone's intersection method.  The input is a ray.
*/
float Cone::intersect(glm::vec3 p0, glm::vec3 dir)
{
    float rh = (radius / height) * (radius / height);
    float a = dir.x * dir.x + dir.z * dir.z - rh * dir.y * dir.y;
    float b = 2 * (dir.x * (p0.x - center.x) + dir.z * (p0.z - center.z) + rh * dir.y * (height - p0.y + center.y));
    float c = (center.x - p0.x) * (center.x - p0.x) + (center.z - p0.z) * (center.z - p0.z) -
        rh * (height - p0.y + center.y) * (height - p0.y + center.y);
    float delta = b * b - (4 * a * c);

    if (fabs(delta) < 0.001) return -1.0;
    if (delta < 0.0) return -1.0;

    float t1 = (-b - sqrt(delta)) / (2 * a);
    float t2 = (-b + sqrt(delta)) / (2 * a);
    if (fabs(t1) < 0.001) return -1;
    if (fabs(t2) < 0.001) return -1;

    float p1, p2;
    
    if (t1 < t2) p1 = t1, p2 = t2;
    else p1 = t2, p2 = t1;

    glm::vec3 closest_intersect_point = p0 + p1 * dir;
    if (closest_intersect_point.y > center.y + height  || closest_intersect_point.y < center.y)
    {
        glm::vec3 second_intersect_point = p0 + p2 * dir;
        if (second_intersect_point.y > center.y + height  || second_intersect_point.y < center.y) return -1.0;
        else return p2;
    }
    return p1;
}


glm::vec3 Cone::normal(glm::vec3 p)
{
    float alpha = atan((p.x - center.x) / (p.z - center.z));
    glm::vec3 n = glm::vec3(sin(alpha)*cos(theta), sin(theta), cos(alpha)*cos(theta));
    n = glm::normalize(n);
    return n;
}
