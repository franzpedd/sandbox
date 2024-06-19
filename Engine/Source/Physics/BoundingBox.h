#pragma once

#include "Util/Math.h"

namespace Cosmos
{
	class BoundingBox
	{
	public:

		// constructor
		BoundingBox() = default;

		// constructor 
		BoundingBox(glm::vec3 min, glm::vec3 max);

		// returns a reference to the min vector
		inline glm::vec3& GetMinRef() { return mMin; }

		// returns a reference to the max vector
		inline glm::vec3& GetMaxRef() { return mMax; }

		// returns if the bounding box was validated
		inline bool IsValid() const { return mValid; }

		// sets the bounding box to valid or not
		inline void SetValid(bool value) { mValid = value; }

		// returns the axis-aligned bounding boxes
		BoundingBox GetAABB(glm::mat4 m);

	private:

		glm::vec3 mMin;
		glm::vec3 mMax;
		bool mValid = false;
	};
}