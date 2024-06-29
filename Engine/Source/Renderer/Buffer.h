#pragma once

#include "Util/Math.h"

namespace Cosmos
{
	// object push constant, contains object id and model matrix
	struct ObjectPushConstant
	{
		alignas(4) uint32_t id;
		alignas(16)glm::mat4 model;
	};

	// Contains the camera's view, projection, view * projection and front
	struct CameraBuffer
	{
		alignas(16) glm::mat4 view = glm::mat4(1.0f);
		alignas(16) glm::mat4 projection = glm::mat4(1.0f);
		alignas(16) glm::mat4 viewProjection = glm::mat4(1.0f);
		alignas(16) glm::vec3 cameraFront = glm::vec3(1.0f);
	};
}