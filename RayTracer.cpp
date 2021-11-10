#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include "Sphere.h"
#include "SceneObject.h"
#include "Ray.h"
#include "Plane.h"
#include "Cylinder.h"
#include "Cone.h"
#include "TextureBMP.h"
#include <GL/freeglut.h>

using namespace std;

const float WIDTH = 20.0;  
const float HEIGHT = 20.0;
const float EDIST = 40.0;		//The distance of the image plane from the camera/origin
const int NUMDIV = 500;			//The number of cells(subdivisions of the image plane) along xand y directions
const int MAX_STEPS = 4;
const float XMIN = -WIDTH * 0.5;
const float XMAX =  WIDTH * 0.5;
const float YMIN = -HEIGHT * 0.5;
const float YMAX =  HEIGHT * 0.5;

vector<SceneObject*> sceneObjects;
TextureBMP texture1;
TextureBMP texture2;

//---------------------------------------------------------------------------------- 
//   Computes the colour value obtained by tracing a ray and finding its 
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
	glm::vec3 backgroundCol(1);
	glm::vec3 lightPos(20, 40, -20);
	glm::vec3 spotlightPos(20, 30, -100);
	glm::vec3 spotlightDir(-20, -30, 15);
	float cutoff = 12;
	float z1 = -40, z2 = -140, y1=50, y2=-15;
	glm::vec3 color(0);
	SceneObject* obj;

    ray.closestPt(sceneObjects);					//Compare the ray with all objects in the scene
    if(ray.index == -1) return backgroundCol;		//no intersection
	obj = sceneObjects[ray.index];					//object on which the closest point of intersection is found

	if (ray.index == 0) //floor plane
	{   //chequered pattern  
		int width = 5;   
		int iz = (ray.hit.z) / width;
		int ix = (ray.hit.x - 50) / width;
		int k = iz % 2, k2 = ix % 2;
		if (k == k2) color = glm::vec3(0.5, 0, 0);
		else color = glm::vec3(1, 0.84, 0);
		obj->setColor(color);
	} 
	else if (ray.index == 1) //back plane
	{
		float texcoords = (ray.hit.x + 40) / (40+40);
		float texcoordt = (ray.hit.y + 16) / (30+16);
		if (texcoords > 0 && texcoords < 1 && texcoordt > 0 && texcoordt < 1)
		{
			color = texture1.getColorAt(texcoords, texcoordt);
			obj->setColor(color);
		}
	}
	else if (ray.index == 2) //earth sphere
	{
		glm::vec3 d = ray.hit - glm::vec3(-5.0, -1.0, -80.0); //earth location
		d = glm::normalize(d);
		float texcoords = 0.5 + atan2(d.x, d.z) / (2 * 3.14159);
		float texcoordt = 0.5 - asin(-d.y) / 3.14159;
		if (texcoords > 0 && texcoords < 1 && texcoordt > 0 && texcoordt < 1)
		{
			color = texture2.getColorAt(texcoords, texcoordt);
			obj->setColor(color);
		}
	}

	Ray R(ray.hit, spotlightPos - ray.hit);
	R.closestPt(sceneObjects);
	if (R.index == -1) color = obj->lighting(lightPos, spotlightPos, spotlightDir, cutoff, -ray.dir, ray.hit);
	else color = obj->lighting(lightPos, -ray.dir, ray.hit);

	//creates shadow ray from point of intersection to light source
	glm::vec3 lightVec = lightPos - ray.hit; 
	Ray shadowRay(ray.hit, lightVec);
	shadowRay.closestPt(sceneObjects);
	if (shadowRay.index > -1 && shadowRay.dist < glm::length(lightVec)) {
		SceneObject* shadowObj = sceneObjects[shadowRay.index];
		if (shadowObj->isTransparent() || shadowObj->isRefractive()) color *= 0.6f;
		else color *= 0.2f;
	}

	//creates reflection ray
	if (obj->isReflective() && step < MAX_STEPS) 
	{ 
		glm::vec3 normalVec = obj->normal(ray.hit);
		if ((ray.index != 3 && ray.index != 4) || normalVec.z > 0) {
			float rho = obj->getReflectionCoeff();
			glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
			Ray reflectedRay(ray.hit, reflectedDir);
			glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
			color = color + (rho * reflectedColor);
		}
	}

	//creates straight transparent ray
	if (obj->isTransparent() && !obj->isRefractive() && step < MAX_STEPS)
	{
		float trans_c = obj->getTransparencyCoeff();
		Ray transparentRay(ray.hit, ray.dir);
		glm::vec3 transColor = trace(transparentRay, step + 1);
		color = color + (trans_c * transColor);
	}

	//creates refracted ray for sphere
	if (obj->isRefractive() && obj->isTransparent() && step < MAX_STEPS)
	{
		float refrac_c = obj->getRefractionCoeff();
		float refrac_i = obj->getRefractiveIndex();
		glm::vec3 n = obj->normal(ray.hit);
		glm::vec3 g = glm::refract(ray.dir, n, 1/refrac_i);
		Ray refractedRay(ray.hit, g);
		refractedRay.closestPt(sceneObjects);
		glm::vec3 m = obj->normal(refractedRay.hit);
		glm::vec3 h = glm::refract(refractedRay.dir, -m, refrac_i); // 1/eta
		Ray refractedRay2(refractedRay.hit, h);
		glm::vec3 refractedColor = trace(refractedRay2, step + 1);
		color = color + (refrac_c * refractedColor);
	}

	float t = (ray.hit.z - z1) / (z2 - z1);
	float s = (ray.hit.y - y1) / (y2 - y1);
	color = (2 - s - t) * color + s * t * glm::vec3(1);
	return color;
}

//---The main display module -----------------------------------------------------------
// In a ray tracing application, it just displays the ray traced image by drawing
// each cell as a quad.
//---------------------------------------------------------------------------------------
void display()
{
	float xp, yp;  //grid point
	float cellX = (XMAX-XMIN)/NUMDIV;  //cell width
	float cellY = (YMAX-YMIN)/NUMDIV;  //cell height
	float cells[8] = { -0.25, -0.25, 0.25, -0.25, -0.25, 0.25, 0.25, 0.25 };
	glm::vec3 eye(0., 0., 0.);

	glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glBegin(GL_QUADS);  //Each cell is a tiny quad.

	for(int i = 0; i < NUMDIV; i++)	//Scan every cell of the image plane
	{
		xp = XMIN + i*cellX;
		for(int j = 0; j < NUMDIV; j++)
		{ 
			//extra for loop for anti aliasing
			/*glm::vec3 cols[4];
			for (int k = 0; k < 4; k++) {
				yp = YMIN + j*cellY + cellY*cells[k+1];
				float temp_xp = xp + cellX * cells[k];
				glm::vec3 dir(temp_xp + 0.5*cellX, yp + 0.5*cellY, -EDIST);	//direction of the primary ray
				Ray ray = Ray(eye, dir);
				cols[k] = trace(ray, 1);
			}
			glm::vec3 col = (cols[0] + cols[1] + cols[2] + cols[3]) * 0.25f;*/
			
			// Uncomment below to not use supersampling
			yp = YMIN + j*cellY;
		    glm::vec3 dir(xp+0.5*cellX, yp+0.5*cellY, -EDIST);	//direction of the primary ray
		    Ray ray = Ray(eye, dir);
		    glm::vec3 col = trace (ray, 1); //Trace the primary ray and get the colour value*/

			glColor3f(col.r, col.g, col.b);
			glVertex2f(xp, yp);
			glVertex2f(xp+cellX, yp);
			glVertex2f(xp+cellX, yp+cellY);
			glVertex2f(xp, yp+cellY);
        }
    }

    glEnd();
    glFlush();
}


void octahedron(glm::vec3 c, float width, float height)
{
	float hh = height / 2;
	float hw = width / 2;

	Plane* plane1 = new Plane(glm::vec3(c.x, c.y, c.z),
		glm::vec3(c.x, c.y+hh, c.z+hw),
		glm::vec3(c.x-hw, c.y+hh, c.z));
	plane1->setColor(glm::vec3(0, 1, 0));
	plane1->setShininess(0.9);
	sceneObjects.push_back(plane1);
	Plane* plane2 = new Plane(glm::vec3(c.x, c.y, c.z),
		glm::vec3(c.x+hw, c.y+hh, c.z),
		glm::vec3(c.x, c.y+hh, c.z+hw));
	plane2->setColor(glm::vec3(0, 1, 0));
	plane2->setShininess(0.9);
	sceneObjects.push_back(plane2);
	Plane* plane3 = new Plane(glm::vec3(c.x, c.y, c.z),
		glm::vec3(c.x, c.y+hh, c.z-hw),
		glm::vec3(c.x+hw, c.y+hh, c.z));
	plane3->setColor(glm::vec3(0, 1, 0));
	plane3->setShininess(0.9);
	sceneObjects.push_back(plane3);
	Plane* plane4 = new Plane(glm::vec3(c.x, c.y, c.z),
		glm::vec3(c.x-hw, c.y+hh, c.z),
		glm::vec3(c.x, c.y+hh, c.z-hw));
	plane4->setColor(glm::vec3(0, 1, 0));
	plane4->setShininess(0.9);
	sceneObjects.push_back(plane4);

	Plane* plane5 = new Plane(glm::vec3(c.x, c.y+height, c.z),
		glm::vec3(c.x-hw, c.y+hh, c.z),
		glm::vec3(c.x, c.y + hh, c.z+hw));
	plane5->setColor(glm::vec3(0, 1, 0));
	plane5->setShininess(0.9);
	sceneObjects.push_back(plane5);
	Plane* plane6 = new Plane(glm::vec3(c.x, c.y+height, c.z),
		glm::vec3(c.x, c.y + hh, c.z+hw),
		glm::vec3(c.x+hw, c.y + hh, c.z));
	plane6->setColor(glm::vec3(0, 1, 0));
	plane6->setShininess(0.9);
	sceneObjects.push_back(plane6);
	Plane* plane7 = new Plane(glm::vec3(c.x, c.y+height, c.z),
		glm::vec3(c.x+hw, c.y + hh, c.z),
		glm::vec3(c.x, c.y + hh, c.z-hw));
	plane7->setColor(glm::vec3(0, 1, 0));
	plane7->setShininess(0.9);
	sceneObjects.push_back(plane7);
	Plane* plane8 = new Plane(glm::vec3(c.x, c.y + height, c.z),
		glm::vec3(c.x, c.y + hh, c.z-hw),
		glm::vec3(c.x-hw, c.y + hh, c.z));
	plane8->setColor(glm::vec3(0, 1, 0));
	plane8->setShininess(0.9);
	sceneObjects.push_back(plane8);
}

//---This function initializes the scene ------------------------------------------- 
//   Specifically, it creates scene objects (spheres, planes, cones, cylinders etc)
//     and add them to the list of scene objects.
//   It also initializes the OpenGL orthographc projection matrix for drawing the
//     the ray traced image.
//----------------------------------------------------------------------------------
void initialize()
{
	glMatrixMode(GL_PROJECTION);
	gluOrtho2D(XMIN, XMAX, YMIN, YMAX);

	glClearColor(0, 0, 0, 1);

	texture1 = TextureBMP("wall.bmp");
	texture2 = TextureBMP("earth.bmp");

	Plane* floorPlane = new Plane(glm::vec3(-50., -15, -40),                           
		glm::vec3(50., -15, -40),							        
		glm::vec3(50., -15, -150),								          
		glm::vec3(-50., -15, -150));							
	floorPlane->setSpecularity(false);
	sceneObjects.push_back(floorPlane);

	Plane* backPlane = new Plane(glm::vec3(-40., -16, -130),
		glm::vec3(40., -16, -130),
		glm::vec3(40., 30, -130),
		glm::vec3(-40., 30, -130));
	backPlane->setSpecularity(false);
	backPlane->setColor(glm::vec3(0, 1, 0));
	sceneObjects.push_back(backPlane);

	//textured sphere
	Sphere* earth = new Sphere(glm::vec3(-5.0, -1.0, -80.0), 3.5);
	earth->setSpecularity(false);
	sceneObjects.push_back(earth);

	Cylinder* bottom = new Cylinder(glm::vec3(5, -8, -90), 4, 3.5);
	bottom->setColor(glm::vec3(0, 0, 1));
	bottom->setReflectivity(true, 0.4);
	sceneObjects.push_back(bottom);

	Cylinder* top = new Cylinder(glm::vec3(5, -4.5, -90), 2.5, 2.5);
	top->setColor(glm::vec3(0, 0, 1));
	top->setReflectivity(true, 0.4);
	sceneObjects.push_back(top);

	octahedron(glm::vec3(5, -2, -90), 3, 6);

	Plane* table = new Plane(glm::vec3(-10, -8, -65),
		glm::vec3(10, -8, -65),
		glm::vec3(10, -8, -100),
		glm::vec3(-10, -8, -100));
	table->setColor(glm::vec3(0.55, 0.27, 0.08));
	sceneObjects.push_back(table);

	Cylinder* cylinder1 = new Cylinder(glm::vec3(-8, -15, -67), 0.5, 7);
	cylinder1->setColor(glm::vec3(0.18, 0.3, 0.3));
	sceneObjects.push_back(cylinder1);

	Cylinder* cylinder2 = new Cylinder(glm::vec3(8, -15, -67), 0.5, 7);
	cylinder2->setColor(glm::vec3(0.18, 0.3, 0.3));
	sceneObjects.push_back(cylinder2);

	Cylinder* cylinder3 = new Cylinder(glm::vec3(8, -15, -98), 0.5, 7);
	cylinder3->setColor(glm::vec3(0.18, 0.3, 0.3));
	sceneObjects.push_back(cylinder3);

	Cylinder* cylinder4 = new Cylinder(glm::vec3(-8, -15, -98), 0.5, 7);
	cylinder4->setColor(glm::vec3(0.18, 0.3, 0.3));
	sceneObjects.push_back(cylinder4);

	Cone* cone = new Cone(glm::vec3(-5, -8, -80), 3, 4);
	cone->setColor(glm::vec3(0.8, 0, 0));
	sceneObjects.push_back(cone);

	//refractive sphere
	Sphere* glassSphere = new Sphere(glm::vec3(7, -12, -60.0), 3.0);
	glassSphere->setColor(glm::vec3(0, 0, 0));
	glassSphere->setTransparency(true, 0.9);
	glassSphere->setReflectivity(true, 0.05);
	glassSphere->setRefractivity(true, 1, 1.05);
	sceneObjects.push_back(glassSphere);

	//transparent sphere
	Sphere* sphere2 = new Sphere(glm::vec3(7, -5.5, -72.0), 2.5);
	sphere2->setColor(glm::vec3(0.2, 0, 0));
	sphere2->setTransparency(true, 0.8);
	sphere2->setReflectivity(true, 0.05);
	sceneObjects.push_back(sphere2);

	//reflective sphere
	Sphere *sphere1 = new Sphere(glm::vec3(-9, -13.5, -62.0), 1.5);
	sphere1->setColor(glm::vec3(0, 1, 0));  
	sphere1->setReflectivity(true, 0.8);
	sceneObjects.push_back(sphere1);
}


int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracing");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
