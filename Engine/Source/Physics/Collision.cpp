#include "epch.h"
#include "Collision.h"

namespace Cosmos
{
	void ScreenPosToWorldRay(int mouseX, int mouseY, int screenWidth, int screenHeight, glm::mat4 ViewMatrix, glm::mat4 ProjectionMatrix, glm::vec3& out_origin, glm::vec3& out_direction)
	{
		// The ray Start and End positions, in Normalized Device Coordinates (Have you read Tutorial 4 ?)
		glm::vec4 lRayStart_NDC(
			((float)mouseX / (float)screenWidth - 0.5f),
			-((float)mouseY / (float)screenHeight - 0.5f) * 2.0f,
			0.0f,
			1.0f
		);

		glm::vec4 lRayEnd_NDC(
			((float)mouseX / (float)screenWidth - 0.5f) * 2.0f,
			-((float)mouseY / (float)screenHeight - 0.5f) * 2.0f,
			1.0f,
			1.0f
		);

		// The Projection matrix goes from Camera Space to NDC.
		// So inverse(ProjectionMatrix) goes from NDC to Camera Space.
		glm::mat4 InverseProjectionMatrix = glm::inverse(ProjectionMatrix);

		// The View Matrix goes from World Space to Camera Space.
		// So inverse(ViewMatrix) goes from Camera Space to World Space.
		glm::mat4 InverseViewMatrix = glm::inverse(ViewMatrix);

		// just one inverse
		glm::mat4 M = glm::inverse(ProjectionMatrix * ViewMatrix);
		glm::vec4 lRayStart_world = M * lRayStart_NDC; lRayStart_world /= lRayStart_world.w;
		glm::vec4 lRayEnd_world = M * lRayEnd_NDC; lRayEnd_world /= lRayEnd_world.w;

		glm::vec3 lRayDir_world(lRayEnd_world - lRayStart_world);
		lRayDir_world = glm::normalize(lRayDir_world);

		out_origin = glm::vec3(lRayStart_world);
		out_direction = lRayDir_world;
	}

	bool TestRayOBBIntersection(glm::vec3 ray_origin, glm::vec3 ray_direction, glm::vec3 aabb_min, glm::vec3 aabb_max, glm::mat4 ModelMatrix, float& intersection_distance)
	{
		// intersection method from real-time rendering and essential mathematics for games
		float tMin = 0.0f;
		float tMax = 100000.0f;

		glm::vec3 OBBposition_worldspace(ModelMatrix[3].x, ModelMatrix[3].y, ModelMatrix[3].z);
		glm::vec3 delta = OBBposition_worldspace - ray_origin;

		// test intersection with the 2 planes perpendicular to the OBB's X axis
		{
			glm::vec3 xaxis(ModelMatrix[0].x, ModelMatrix[0].y, ModelMatrix[0].z);
			float e = glm::dot(xaxis, delta);
			float f = glm::dot(ray_direction, xaxis);

			// standard case
			if (fabs(f) > 0.001f)
			{ 
				float t1 = (e + aabb_min.x) / f; // intersection with the "left" plane
				float t2 = (e + aabb_max.x) / f; // intersection with the "right" plane
				// t1 and t2 now contain distances betwen ray origin and ray-plane intersections

				// we want t1 to represent the nearest intersection, so if it's not the case, invert t1 and t2
				if (t1 > t2)
				{
					// swap t1 and t2
					float w = t1;
					t1 = t2;
					t2 = w;
				}

				// tMax is the nearest "far" intersection (amongst the X,Y and Z planes pairs)
				if (t2 < tMax)
				{
					tMax = t2;
				}

				// tMin is the farthest "near" intersection (amongst the X,Y and Z planes pairs)
				if (t1 > tMin)
				{
					tMin = t1;
				}

				// And here's the trick, if "far" is closer than "near", then there is NO intersection.
				if (tMax < tMin)
				{
					return false;
				}
			}

			// rare case : the ray is almost parallel to the planes, so they don't have any "intersection"
			else
			{ 
				if (-e + aabb_min.x > 0.0f || -e + aabb_max.x < 0.0f)
				{
					return false;
				}
			}
		}


		// test intersection with the 2 planes perpendicular to the OBB's Y axis, exactly the same thing than above.
		{
			glm::vec3 yaxis(ModelMatrix[1].x, ModelMatrix[1].y, ModelMatrix[1].z);
			float e = glm::dot(yaxis, delta);
			float f = glm::dot(ray_direction, yaxis);

			if (fabs(f) > 0.001f)
			{
				float t1 = (e + aabb_min.y) / f;
				float t2 = (e + aabb_max.y) / f;

				if (t1 > t2) 
				{ 
					float w = t1; 
					t1 = t2; 
					t2 = w; 
				}

				if (t2 < tMax)
				{
					tMax = t2;
				}
					
				if (t1 > tMin)
				{
					tMin = t1;
				}
					
				if (tMin > tMax)
				{
					return false;
				}
			}

			else
			{
				if (-e + aabb_min.y > 0.0f || -e + aabb_max.y < 0.0f)
				{
					return false;
				}
			}
		}

		// Test intersection with the 2 planes perpendicular to the OBB's Z axis, exactly the same thing than above.
		{
			glm::vec3 zaxis(ModelMatrix[2].x, ModelMatrix[2].y, ModelMatrix[2].z);
			float e = glm::dot(zaxis, delta);
			float f = glm::dot(ray_direction, zaxis);

			if (fabs(f) > 0.001f)
			{

				float t1 = (e + aabb_min.z) / f;
				float t2 = (e + aabb_max.z) / f;

				if (t1 > t2) 
				{ 
					float w = t1; 
					t1 = t2; 
					t2 = w; 
				}

				if (t2 < tMax)
				{
					tMax = t2;
				}
					
				if (t1 > tMin)
				{
					tMin = t1;
				}
					
				if (tMin > tMax)
				{
					return false;
				}
			}

			else
			{
				if (-e + aabb_min.z > 0.0f || -e + aabb_max.z < 0.0f)
				{
					return false;
				}
			}
		}

		intersection_distance = tMin;
		return true;
	}
}