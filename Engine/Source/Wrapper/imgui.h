#pragma once

#include "Platform/Detection.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#if defined(PLATFORM_WINDOWS)
#pragma warning(push)
#pragma warning(disable : 26495 6263 6001 6255)
#endif

#include <imguizmo.h>

#if defined(PLATFORM_WINDOWS)
#pragma warning(pop)
#endif