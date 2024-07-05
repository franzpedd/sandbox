#pragma once

#include "Util/Math.h"

namespace Cosmos::Physics
{
	class BoundingBox
	{
	public:

		// constructor
		BoundingBox() = default;

		// constructor
		BoundingBox(glm::vec3 min, glm::vec3 max);

		// destructor
		~BoundingBox() = default;

		// returns the axis-align bounding box
		BoundingBox GetAABB(glm::mat4 mat);

		// returns the max value
		inline glm::vec3 GetMax() const { return mMax; }

		// sets a new max value
		inline void SetMax(glm::vec3 max) { mMax = max; }

		// returns the min value
		inline glm::vec3 GetMin() const { return mMin; }

		// sets a new min value
		inline void SetMin(glm::vec3 min) { mMin = min; }

		// returns if bounding box was validated
		inline bool IsValid() const { return mValidated; }

		// sets the validation status of the bounding box
		inline void SetValid(bool value) { mValidated = value; }

	private:

		glm::vec3 mMin = {};
		glm::vec3 mMax = {};
		bool mValidated = false;
	};
}