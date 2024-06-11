#pragma once

#include "Util/Math.h"

namespace Cosmos
{
    struct Vertex
    {
        enum Component
        {
            POSITION = 0,
            COLOR,
            NORMAL,
            UV
        };

        glm::vec3 position;
        glm::vec3 color;
        glm::vec3 normal;
        glm::vec2 uv;

        // checks if current vertex is the same as another
        inline bool operator==(const Vertex& other) const
        {
            return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
        }
    };
}