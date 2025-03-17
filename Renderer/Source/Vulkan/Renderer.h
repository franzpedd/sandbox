#pragma once

#include "IRenderer.h"

namespace Cosmos::Vulkan
{
	class Instance;
	class Device;

	class Renderer : public Cosmos::IRenderer
	{
	public:

		// constructor
		Renderer();

		// destructor
		~Renderer();

		// returns the smart-ptr to the instance object
		inline std::shared_ptr<Instance> GetInstance() const { return mInstance; }

		// returns the smart-ptr to the device object
		inline std::shared_ptr<Device> GetDevice() const { return mDevice; }

	public:

		// initializes the renderer
		virtual void Initialize() override;

	private:

		std::shared_ptr<Instance> mInstance = nullptr;
		std::shared_ptr<Device> mDevice = nullptr;
	};
}