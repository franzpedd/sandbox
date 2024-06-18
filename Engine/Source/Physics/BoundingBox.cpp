#include "epch.h"
#include "BoundingBox.h"

namespace Cosmos
{
	BoundingBox::BoundingBox(glm::vec3 min, glm::vec3 max)
		: mMin(min), mMax(max)
	{
	}

	BoundingBox BoundingBox::GetAABB(glm::mat4 m)
	{
		glm::vec3 min = glm::vec3(m[3]);
		glm::vec3 max = min;
		glm::vec3 v0, v1;

		glm::vec3 right = glm::vec3(m[0]);
		v0 = right * mMin.x;
		v1 = right * mMax.x;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		glm::vec3 up = glm::vec3(m[1]);
		v0 = up * mMin.y;
		v1 = up * mMax.y;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		glm::vec3 back = glm::vec3(m[2]);
		v0 = back * mMin.z;
		v1 = back * mMax.z;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		return BoundingBox(min, max);
	}
}