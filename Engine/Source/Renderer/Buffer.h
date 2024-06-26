#pragma once

#include "Util/Math.h"

namespace Cosmos
{
	// this is the camera buffer, it contains the model, view and projection
	struct MVP_Buffer
	{
		alignas(16) glm::mat4 model = glm::mat4(1.0f);
		alignas(16) glm::mat4 view = glm::mat4(1.0f);
		alignas(16) glm::mat4 projection = glm::mat4(1.0f);
		alignas(16) glm::vec3 cameraPos = glm::vec3(0.0f);
	};

	// editor utilities buffer
	struct Util_Buffer
	{
		alignas(4) float selected = 0.0f;
	};
}