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
		alignas(4) float picking = 0.0f;
		alignas(8) glm::vec2 mousePos = glm::vec2(0.0f);
		alignas(8) glm::vec2 windowSize = glm::vec2(0.0f);
	};

	// picking
	struct Picking_Buffer
	{
		alignas(4) uint32_t id;
	};
}