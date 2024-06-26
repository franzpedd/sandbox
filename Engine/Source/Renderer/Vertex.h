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
            UV,
            JOINT,
            WEIGHT,
            COLOR,
            UID,
        };

        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::uvec4 joint;
        glm::vec4 weight;
        glm::vec4 color;
        uint32_t uid;

        // checks if current vertex is the same as another
        inline bool operator==(const Vertex& other) const
        {
            return position == other.position
                && normal == other.normal
                && uv == other.uv
                && joint == other.joint
                && weight == other.weight
                && color == other.color
                && uid == other.uid;
        }
    };
}