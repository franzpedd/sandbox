#include "epch.h"
#if defined COSMOS_RENDERER_VULKAN

#include "VKRenderer.h"

#include "Instance.h"
#include "Device.h"
#include "Renderpass.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "VKUI.h"

#include "Core/Application.h"
#include "Core/Event.h"
#include "Core/Scene.h"
#include "Entity/Unique/Camera.h"
#include "Platform/Window.h"
#include "Util/Logger.h"

#include <array>

namespace Cosmos::Vulkan
{
	VKRenderer::VKRenderer(Cosmos::Application* application, Shared<Cosmos::Window> window)
		: Renderer(application, window)
	{
		mInstance = CreateShared<Vulkan::Instance>(mWindow, "Cosmos", "Application", true);
		mDevice = CreateShared<Vulkan::Device>(mWindow, mInstance);
		mRenderpassManager = CreateShared<Vulkan::RenderpassManager>(mDevice);
		mSwapchain = CreateShared<Vulkan::Swapchain>(mWindow, mDevice, mRenderpassManager, mConcurrentlyRenderedFrames);
		mPipelineLibrary = CreateShared<Vulkan::PipelineLibrary>(mDevice, mRenderpassManager);
	}

	VKRenderer::~VKRenderer()
	{
		
	}

	void VKRenderer::OnUpdate()
	{
		// first we update general renderer
		Renderer::OnUpdate(); 

		// aquire image from swapchain
		vkWaitForFences(mDevice->GetLogicalDevice(), 1, &mSwapchain->GetInFlightFencesRef()[mCurrentFrame], VK_TRUE, UINT64_MAX);
		VkResult res = vkAcquireNextImageKHR(mDevice->GetLogicalDevice(), mSwapchain->GetSwapchain(), UINT64_MAX, mSwapchain->GetAvailableSemaphoresRef()[mCurrentFrame], VK_NULL_HANDLE, &mImageIndex);

		if (res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			mSwapchain->Recreate();
			return;
		}

		else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
		{
			COSMOS_ASSERT(false, "Failed to acquired next swapchain image");
		}

		vkResetFences(mDevice->GetLogicalDevice(), 1, &mSwapchain->GetInFlightFencesRef()[mCurrentFrame]);

		// register draw calls based on the configuration of previously configured renderpasses
		ManageRenderpasses();

		// submits the draw calls into the graphics queue, we're not using any compute queue at the momment
		VkSwapchainKHR swapChains[] = { mSwapchain->GetSwapchain() };
		VkSemaphore waitSemaphores[] = { mSwapchain->GetAvailableSemaphoresRef()[mCurrentFrame] };
		VkSemaphore signalSemaphores[] = { mSwapchain->GetFinishedSempahoresRef()[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		std::vector<VkCommandBuffer> submitCommandBuffers = { mRenderpassManager->GetRenderpassesRef()["Swapchain"]->GetSpecificationRef().commandBuffers[mCurrentFrame] };

		if (mRenderpassManager->Exists("Viewport"))
		{
			submitCommandBuffers.push_back(mRenderpassManager->GetRenderpassesRef()["Viewport"]->GetSpecificationRef().commandBuffers[mCurrentFrame]);
		}

		if (mRenderpassManager->Exists("ImGui"))
		{
			submitCommandBuffers.push_back(mRenderpassManager->GetRenderpassesRef()["ImGui"]->GetSpecificationRef().commandBuffers[mCurrentFrame]);
		}

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = (uint32_t)submitCommandBuffers.size();
		submitInfo.pCommandBuffers = submitCommandBuffers.data();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
		COSMOS_ASSERT(vkQueueSubmit(mDevice->GetGraphicsQueue(), 1, &submitInfo, mSwapchain->GetInFlightFencesRef()[mCurrentFrame]) == VK_SUCCESS, "Failed to submit draw command");

		// presents the image
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &mImageIndex;
		
		res = vkQueuePresentKHR(mDevice->GetPresentQueue(), &presentInfo);
		
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || mWindow->ShouldResize())
		{
			mSwapchain->Recreate();
		
			mCamera->SetAspectRatio(mWindow->GetAspectRatio());
			std::dynamic_pointer_cast<Vulkan::VKUI>(mApplication->GetUI())->SetImageCount(mSwapchain->GetImageCount());
		
			int32_t width = (int32_t)mSwapchain->GetExtent().width;
			int32_t height = (int32_t)mSwapchain->GetExtent().height;
		
			Shared<WindowResizeEvent> event = CreateShared<WindowResizeEvent>(width, height);
			mApplication->OnEvent(event);
		}
		
		else if (res != VK_SUCCESS)
		{
			COSMOS_ASSERT(false, "Failed to present swapchain image");
		}
	}

	void VKRenderer::OnEvent(Shared<Event> event)
	{
		Renderer::OnEvent(event);
	}

	void VKRenderer::ManageRenderpasses()
	{
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };

		// color and depth render pass
		if (mRenderpassManager->Exists("Swapchain"))
		{
			VkCommandBuffer& cmdBuffer = mRenderpassManager->GetRenderpassesRef()["Swapchain"]->GetSpecificationRef().commandBuffers[mCurrentFrame];
			VkFramebuffer& frameBuffer = mRenderpassManager->GetRenderpassesRef()["Swapchain"]->GetSpecificationRef().frameBuffers[mImageIndex];
			VkRenderPass& renderPass = mRenderpassManager->GetRenderpassesRef()["Swapchain"]->GetSpecificationRef().renderPass;

			vkResetCommandBuffer(cmdBuffer, /*VkCommandBufferResetFlagBits*/ 0);

			VkCommandBufferBeginInfo cmdBeginInfo = {};
			cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBeginInfo.pNext = nullptr;
			cmdBeginInfo.flags = 0;
			COSMOS_ASSERT(vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo) == VK_SUCCESS, "Failed to begin command buffer recording");

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = frameBuffer;
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = mSwapchain->GetExtent();
			renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
			renderPassBeginInfo.pClearValues = clearValues.data();
			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// set frame commandbuffer viewport
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)mSwapchain->GetExtent().width;
			viewport.height = (float)mSwapchain->GetExtent().height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			// set frame commandbuffer scissor
			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = mSwapchain->GetExtent();
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			// render scene, only if viewport doesnt exists
			if (!mRenderpassManager->Exists("Viewport"))
			{
				mApplication->GetScene()->OnRender();
			}

			vkCmdEndRenderPass(cmdBuffer);

			// end command buffer
			COSMOS_ASSERT(vkEndCommandBuffer(cmdBuffer) == VK_SUCCESS, "Failed to end command buffer recording");
		}

		// viewport
		if (mRenderpassManager->Exists("Viewport"))
		{
			VkCommandBuffer& cmdBuffer = mRenderpassManager->GetRenderpassesRef()["Viewport"]->GetSpecificationRef().commandBuffers[mCurrentFrame];
			VkFramebuffer& frameBuffer = mRenderpassManager->GetRenderpassesRef()["Viewport"]->GetSpecificationRef().frameBuffers[mImageIndex];
			VkRenderPass& renderPass = mRenderpassManager->GetRenderpassesRef()["Viewport"]->GetSpecificationRef().renderPass;

			vkResetCommandBuffer(cmdBuffer, /*VkCommandBufferResetFlagBits*/ 0);

			VkCommandBufferBeginInfo cmdBeginInfo = {};
			cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBeginInfo.pNext = nullptr;
			cmdBeginInfo.flags = 0;
			COSMOS_ASSERT(vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo) == VK_SUCCESS, "Failed to begin command buffer recording");

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = frameBuffer;
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = mSwapchain->GetExtent();
			renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
			renderPassBeginInfo.pClearValues = clearValues.data();
			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// set frame commandbuffer viewport
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)mSwapchain->GetExtent().width;
			viewport.height = (float)mSwapchain->GetExtent().height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			// set frame commandbuffer scissor
			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = mSwapchain->GetExtent();
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			// render scene and special widgets
			mApplication->GetUI()->OnRender();
			mApplication->GetScene()->OnRender();

			vkCmdEndRenderPass(cmdBuffer);

			// end command buffer
			COSMOS_ASSERT(vkEndCommandBuffer(cmdBuffer) == VK_SUCCESS, "Failed to end command buffer recording");
		}

		// user interface
		if (mRenderpassManager->Exists("ImGui"))
		{
			VkCommandBuffer& cmdBuffer = mRenderpassManager->GetRenderpassesRef()["ImGui"]->GetSpecificationRef().commandBuffers[mCurrentFrame];
			VkFramebuffer& frameBuffer = mRenderpassManager->GetRenderpassesRef()["ImGui"]->GetSpecificationRef().frameBuffers[mImageIndex];
			VkRenderPass& renderPass = mRenderpassManager->GetRenderpassesRef()["ImGui"]->GetSpecificationRef().renderPass;

			vkResetCommandBuffer(cmdBuffer, /*VkCommandBufferResetFlagBits*/ 0);

			VkCommandBufferBeginInfo cmdBeginInfo = {};
			cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBeginInfo.pNext = nullptr;
			cmdBeginInfo.flags = 0;
			COSMOS_ASSERT(vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo) == VK_SUCCESS, "Failed to begin command buffer recording");

			VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = frameBuffer;
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = mSwapchain->GetExtent();
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearValue;
			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			std::dynamic_pointer_cast<Vulkan::VKUI>(mApplication->GetUI())->Draw(cmdBuffer);

			vkCmdEndRenderPass(cmdBuffer);

			COSMOS_ASSERT(vkEndCommandBuffer(cmdBuffer) == VK_SUCCESS, "Failed to end command buffer recording");
		}
	}
}

#endif