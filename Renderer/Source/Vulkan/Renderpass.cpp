#include "Renderpass.h"

namespace Cosmos::Vulkan
{
	Renderpass::Renderpass(std::shared_ptr<Device> device, const char* name, VkSampleCountFlagBits msaa)
		: mDevice(device), mName(name), mMSAA(msaa)
	{
	}
}