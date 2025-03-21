#pragma once

#include "Util/Math.h"
#include "Util/UUID.h"
#include <string>

namespace Cosmos
{
	struct IDComponent
	{
		UUID id;

		// constructor
		IDComponent() = default;
	};

	struct NameComponent
	{
		std::string name;

		// constructor
		NameComponent() = default;
	};

	struct TransformComponent
	{
		glm::vec3 translation = glm::vec3(0.0f);
		glm::vec3 rotation = glm::vec3(0.0f);
		glm::vec3 scale = glm::vec3(1.0f);

		// constructor
		TransformComponent() = default;

		// returns the transformed matrix
		glm::mat4 GetTransform() const
		{
			glm::mat4 rot = glm::toMat4(glm::quat(rotation));
			return glm::translate(glm::mat4(1.0f), translation) * rot * glm::scale(glm::mat4(1.0f), scale);
		}

		// returns the normal matrix
		glm::mat4 GetNormal() const
		{
			return glm::transpose(glm::inverse(glm::mat3(GetTransform())));
		}

		// returns the center of the matrix
		glm::vec3 GetCenter() const
		{
			glm::mat4 matrix = GetTransform();
			return glm::vec3(matrix[3][0], matrix[3][1], matrix[3][2]);
		}
	};
}