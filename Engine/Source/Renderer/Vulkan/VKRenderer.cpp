#include "epch.h"
#if defined COSMOS_RENDERER_VULKAN

#include "VKRenderer.h"

#include "Instance.h"
#include "Device.h"
#include "Renderpass.h"
#include "Shader.h"
#include "Swapchain.h"
#include "Pipeline.h"
#include "VKUI.h"

#include "Core/Application.h"
#include "Core/Event.h"
#include "Core/Scene.h"
#include "Entity/Unique/Camera.h"
#include "Platform/Window.h"
#include "Util/Files.h"
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

		CreateBRDFLookUpTable();
	}

	VKRenderer::~VKRenderer()
	{
		vkDestroySampler(mDevice->GetLogicalDevice(), mBRDFSampler, nullptr);
		vkDestroyImageView(mDevice->GetLogicalDevice(), mBRDFImageView, nullptr);
		vmaDestroyImage(mDevice->GetAllocator(), mBRDFImage, mBRDFMemory);
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

	void VKRenderer::CreateBRDFLookUpTable()
	{
		auto start = std::chrono::high_resolution_clock::now();
		const VkFormat format = VK_FORMAT_R16G16_SFLOAT;
		const int32_t size = 512;

		// create image resource on gpu
		mDevice->CreateImage
		(
			size, 
			size, 1, 
			1, 
			VK_SAMPLE_COUNT_1_BIT, 
			format, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			mBRDFImage,
			mBRDFMemory,
			VK_IMAGE_TYPE_2D,
			VMA_ALLOCATION_CREATE_DONT_BIND_BIT
		);

		// create an image view for the gpu-image
		mBRDFImageView = mDevice->CreateImageView
		(
			mBRDFImage,
			format,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1,
			1,
			VK_IMAGE_VIEW_TYPE_2D
		);

		// create an image sampler for the image sampling mode
		mBRDFSampler = mDevice->CreateSampler
		(
			VK_FILTER_LINEAR,
			VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			1.0f
		);

		// create framebuffer and it's color attachment
		VkAttachmentDescription attachmentDesc = {};
		attachmentDesc.format = format;
		attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference colorRef = {};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		VkSubpassDescription subpassDesc = {};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDesc.colorAttachmentCount = 1;
		subpassDesc.pColorAttachments = &colorRef;

		// use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies = {};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// create renderpass
		VkRenderPassCreateInfo renderPassCI{};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = 1;
		renderPassCI.pAttachments = &attachmentDesc;
		renderPassCI.subpassCount = 1;
		renderPassCI.pSubpasses = &subpassDesc;
		renderPassCI.dependencyCount = 2;
		renderPassCI.pDependencies = dependencies.data();

		VkRenderPass renderpass;
		COSMOS_ASSERT(vkCreateRenderPass(mDevice->GetLogicalDevice(), &renderPassCI, nullptr, &renderpass) == VK_SUCCESS, "Failed to create renderpass for BRDF");

		// finally create framebuffer
		VkFramebufferCreateInfo framebufferCI{};
		framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCI.renderPass = renderpass;
		framebufferCI.attachmentCount = 1;
		framebufferCI.pAttachments = &mBRDFImageView;
		framebufferCI.width = size;
		framebufferCI.height = size;
		framebufferCI.layers = 1;
		
		VkFramebuffer framebuffer;
		COSMOS_ASSERT(vkCreateFramebuffer(mDevice->GetLogicalDevice(), &framebufferCI, nullptr, &framebuffer) == VK_SUCCESS, "Failed to create framebuffer for BRDF");

		// create descriptor set layout and pipeline layout
		VkDescriptorSetLayout descSetLayout = {};
		VkDescriptorSetLayoutCreateInfo descSetCID = {};
		descSetCID.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descSetCID.flags = 0;
		COSMOS_ASSERT(vkCreateDescriptorSetLayout(mDevice->GetLogicalDevice(), &descSetCID, nullptr, &descSetLayout) == VK_SUCCESS, "Failed to create descriptor set layout for BRDF");

		// pipeline layout
		VkPipelineLayout pipelinelayout = {};
		VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.setLayoutCount = 1;
		pipelineLayoutCI.pSetLayouts = &descSetLayout;
		COSMOS_ASSERT(vkCreatePipelineLayout(mDevice->GetLogicalDevice(), &pipelineLayoutCI, nullptr, &pipelinelayout) == VK_SUCCESS, "Failed to create pipeline layout for BRDF");
		
		// graphics pipeline
		COSMOS_LOG(Logger::Todo, "Use Pipeline class");
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = {};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineRasterizationStateCreateInfo rasterizationStateCI = {};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCI.lineWidth = 1.0f;

		VkPipelineColorBlendAttachmentState blendAttachmentState = {};
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		blendAttachmentState.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendStateCI = {};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentState;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = {};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_FALSE;
		depthStencilStateCI.depthWriteEnable = VK_FALSE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilStateCI.front = depthStencilStateCI.back;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

		VkPipelineViewportStateCreateInfo viewportStateCI = {};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;

		VkPipelineMultisampleStateCreateInfo multisampleStateCI = {};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicStateCI = {};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

		VkPipelineVertexInputStateCreateInfo emptyInputStateCI = {};
		emptyInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		
		Shared<Shader> vert = CreateShared<Shader>(mDevice, Shader::Type::Vertex, "PBR/BRDF.vert", GetAssetSubDir("Shader/pbr/brdf.vert"));
		Shared<Shader> frag = CreateShared<Shader>(mDevice, Shader::Type::Fragment, "PBR/BRDF.frag", GetAssetSubDir("Shader/pbr/brdf.frag"));
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = { vert->GetShaderStageCreateInfoRef(), frag->GetShaderStageCreateInfoRef() };

		VkGraphicsPipelineCreateInfo pipelineCI = {};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = pipelinelayout;
		pipelineCI.renderPass = renderpass;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pVertexInputState = &emptyInputStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = 2;
		pipelineCI.pStages = shaderStages.data();

		// look-up-table (from BRDF) pipeline
		VkPipeline pipeline = {};
		COSMOS_ASSERT(vkCreateGraphicsPipelines(mDevice->GetLogicalDevice(), nullptr, 1, &pipelineCI, nullptr, &pipeline) == VK_SUCCESS, "Failed to create BRDF graphics pipeline");

		// render the image
		VkClearValue clearValues[1] = {};
		clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderpass;
		renderPassBeginInfo.renderArea.extent.width = size;
		renderPassBeginInfo.renderArea.extent.height = size;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.framebuffer = framebuffer;
		
		COSMOS_LOG(Logger::Todo, "Create render pass for BRDF ? or just use swapchain's");
		VkCommandBuffer cmdBuf = mDevice->CreateCommandBuffer(mRenderpassManager->GetMainRenderpass()->GetSpecificationRef().commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport = {};
		viewport.width = (float)size;
		viewport.height = (float)size;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.extent.width = size;
		scissor.extent.height = size;

		vkCmdSetViewport(cmdBuf, 0, 1, &viewport);
		vkCmdSetScissor(cmdBuf, 0, 1, &scissor);
		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdDraw(cmdBuf, 3, 1, 0, 0);
		vkCmdEndRenderPass(cmdBuf);

		mDevice->EndCommandBuffer(mRenderpassManager->GetMainRenderpass()->GetSpecificationRef().commandPool, cmdBuf, mDevice->GetGraphicsQueue(), true);

		// wait until it's rendered
		vkQueueWaitIdle(mDevice->GetGraphicsQueue());

		// destroy used resources
		vkDestroyPipeline(mDevice->GetLogicalDevice(), pipeline, nullptr);
		vkDestroyPipelineLayout(mDevice->GetLogicalDevice(), pipelinelayout, nullptr);
		vkDestroyRenderPass(mDevice->GetLogicalDevice(), renderpass, nullptr);
		vkDestroyFramebuffer(mDevice->GetLogicalDevice(), framebuffer, nullptr);
		vkDestroyDescriptorSetLayout(mDevice->GetLogicalDevice(), descSetLayout, nullptr);

		// shows time took
		auto end = std::chrono::high_resolution_clock::now();
		auto timeDifference = std::chrono::duration<double, std::milli>(end - start).count();
		COSMOS_LOG(Logger::Info, "BRDF Generation took %f ms", timeDifference);
	}
}

#endif