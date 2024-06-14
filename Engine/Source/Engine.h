
// core functionality
#include "Core/Application.h"
#include "Core/Event.h"
#include "Core/Scene.h"

// entity system
#include "Entity/Unique/Camera.h"

// platform code
#include "Platform/Detection.h"
#include "Platform/Input.h"
#include "Platform/Window.h"

// renderer
#include "Renderer/Buffer.h"
#include "Renderer/Renderer.h"
#include "Renderer/Texture.h"
#include "Renderer/Vertex.h"

// vulkan backend (must be defined before included)
#if defined COSMOS_RENDERER_VULKAN
#include "Renderer/Vulkan/Device.h"
#include "Renderer/Vulkan/Instance.h"
#include "Renderer/Vulkan/Pipeline.h"
#include "Renderer/Vulkan/Renderpass.h"
#include "Renderer/Vulkan/Shader.h"
#include "Renderer/Vulkan/Swapchain.h"
#include "Renderer/Vulkan/VKRenderer.h"
#include "Renderer/Vulkan/VKTexture.h"
#endif

// user interface
#include "UI/Icons.h"
#include "UI/Theme.h"
#include "UI/UI.h"
#include "UI/Widget.h"

// utility
#include "Util/Algorithm.h"
#include "Util/Datafile.h"
#include "Util/Files.h"
#include "Util/Logger.h"
#include "Util/Math.h"
#include "Util/Memory.h"
#include "Util/Queue.h"
#include "Util/Stack.h"

// wrappers are not to be included into the apps