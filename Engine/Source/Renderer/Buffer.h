#pragma once

#include "Util/Math.h"

namespace Cosmos
{
	// this is the camera buffer, it contains the model, view and projection
	struct MVP_Buffer
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 projection;
	};
}