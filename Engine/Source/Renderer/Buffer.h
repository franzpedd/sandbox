#pragma once

#include "Util/Math.h"

namespace Cosmos
{
	// object push constant, contains object id and model matrix
	struct ObjectPushConstant
	{
		alignas(4) uint32_t id = 0;
		alignas(16)glm::mat4 model = glm::mat4(1.0f);
	};

	// contains the camera's view, projection, view * projection and front
	struct CameraBuffer
	{
		alignas(16) glm::mat4 view = glm::mat4(1.0f);
		alignas(16) glm::mat4 projection = glm::mat4(1.0f);
		alignas(16) glm::mat4 viewProjection = glm::mat4(1.0f);
		alignas(16) glm::vec3 cameraFront = glm::vec3(1.0f);
	};

	// contains the window attributes
	struct WindowBuffer
	{
		alignas(4) uint32_t selectedID = 0;
		alignas(8) glm::vec2 mousePos = glm::vec2(1.0f);
	};

	// contains usefull data, like mouse coords and picking max depth
	#define Z_DEPTH 64 
	struct PickingDepthBuffer
	{
		alignas(4) uint32_t pickingDepth[Z_DEPTH] = {0}; // this is the depth distance of the scene when performing mouse picking, changing this requires changing on shader
	};
}