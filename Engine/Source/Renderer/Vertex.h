#pragma once

#include "Util/Math.h"

namespace Cosmos
{
    struct Vertex
    {
        enum Component
        {
            POSITION = 0,
            NORMAL,
            UV0,
            UV1,
            JOINT,
            WEIGHT,
            COLOR
        };

        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv0;
        glm::vec2 uv1;
        glm::uvec4 joint;
        glm::vec4 weight;
        glm::vec4 color;

        // checks if current vertex is the same as another
        inline bool operator==(const Vertex& other) const
        {
            return position == other.position
                && normal == other.normal
                && uv0 == other.uv0
                && uv1 == other.uv1
                && joint == other.joint
                && weight == other.weight
                && color == other.color;
        }
    };
}