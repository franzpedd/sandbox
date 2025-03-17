#include "IRenderer.h"

#include "Vulkan/Renderer.h"
#include "Util/Assert.h"

namespace Cosmos
{
	std::shared_ptr<IRenderer> IRenderer::Create()
	{
		return std::make_shared<Vulkan::Renderer>();
	}

#if defined(_WIN32)
	void IRenderer::ConnectWindow(HINSTANCE instance, HWND window)
	{
		LOG_ASSERT(mInstance == nullptr, "Windows instance is null");
		LOG_ASSERT(mWindow == nullptr, "Windows handle is null");

		mInstance = instance;
		mWindow = window;
	}
#endif
}