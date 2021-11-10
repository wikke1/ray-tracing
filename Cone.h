#ifndef H_CONE
#define H_CONE

#include <glm/glm.hpp>
#include "SceneObject.h"

class Cone : public SceneObject
{
private:
	glm::vec3 center = glm::vec3(0);
	float radius = 1;
	float height = 1;
	float theta = atan(radius / height);

public:
	Cone() {}; //Default contructor

	Cone(glm::vec3 c, float r, float h) : center(c), radius(r), height(h) {}

	float intersect(glm::vec3 p0, glm::vec3 dir);

	glm::vec3 normal(glm::vec3 p);
};

#endif //!H_CONE