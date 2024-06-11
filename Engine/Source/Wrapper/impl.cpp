#include "epch.h"

#include "Platform/Detection.h"

// volk
#define VOLK_IMPLEMENTATION
#include <volk.h>

// vulkan memory allocator

#if defined(PLATFORM_WINDOWS)
#pragma warning(push)
#pragma warning(disable : 26495 6386 6387 26110 26813)
#endif

#define VMA_VULKAN_VERSION 1002000 // vulkan memory allocator version is 1.2.0, same as in the instance
#define VMA_STATIC_VULKAN_FUNCTIONS 0 // not statically linking to vulkan 
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0 // not dynamically linking to vulkan 
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#if defined(PLATFORM_WINDOWS)
#pragma warning(pop)
#endif

// imguizmo
#if defined(PLATFORM_WINDOWS)
#pragma warning(push)
#pragma warning(disable : 26495 6263 6001 6255)
#endif

#include <ImGuizmo.cpp>

#if defined(PLATFORM_WINDOWS)
#pragma warning(pop)
#endif