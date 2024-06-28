#include "Viewport.h"

#include "SceneHierarchy.h"
#include <array>
#include <fstream>

namespace Cosmos
{
	Viewport::Viewport(Shared<Window> window, Shared<Renderer> renderer, Shared<UI> ui, Shared<Scene> scene, SceneHierarchy* sceneHierarchy)
		: Widget("Viewport"), mWindow(window), mRenderer(renderer), mUI(ui), mScene(scene), mSceneHierarchy(sceneHierarchy)
	{
		mSceneGizmos = CreateShared<SceneGizmos>(mRenderer->GetCamera());
		CreateRendererResources();
		CreatePickingResources();
	}

	Viewport::~Viewport()
	{
		auto& swapchain = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetSwapchain();
		auto& device = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetDevice();
		vkDeviceWaitIdle(device->GetLogicalDevice());
		
		vkDestroySampler(device->GetLogicalDevice(), mSampler, nullptr);
		
		vkDestroyImageView(device->GetLogicalDevice(), mDepthView, nullptr);
		vmaDestroyImage(device->GetAllocator(), mDepthImage, mDepthMemory);
		
		for (size_t i = 0; i < swapchain->GetImagesRef().size(); i++)
		{
			vkDestroyImageView(device->GetLogicalDevice(), mImageViews[i], nullptr);
			vmaDestroyImage(device->GetAllocator(), mImages[i], mImageMemories[i]);
		}

		// picking
		vkDestroyImageView(device->GetLogicalDevice(), mPickingColorView, nullptr);
		vkDestroyImageView(device->GetLogicalDevice(), mPickingDepthView, nullptr);
		vmaDestroyImage(device->GetAllocator(), mPickingColorImage, mPickingColorMemory);
		vmaDestroyImage(device->GetAllocator(), mPickingDepthImage, mPickingDepthMemory);
	}

	void Viewport::OnUpdate()
	{
		
		if (ImGui::Begin("Viewport"))
		{
			ImGui::Image(mDescriptorSets[mRenderer->GetCurrentFrame()], ImGui::GetContentRegionAvail());
			
			// updating aspect ratio for the docking
			mCurrentSize = ImGui::GetWindowSize();
			mRenderer->GetCamera()->SetAspectRatio((float)(mCurrentSize.x / mCurrentSize.y));

			// viewport boundaries
			mContentRegionMin = ImGui::GetWindowContentRegionMin();
			mContentRegionMax = ImGui::GetWindowContentRegionMax();
			mContentRegionMin.x += ImGui::GetWindowPos().x;
			mContentRegionMin.y += ImGui::GetWindowPos().y;
			mContentRegionMax.x += ImGui::GetWindowPos().x;
			mContentRegionMax.y += ImGui::GetWindowPos().y;

			// draw gizmos
			mSceneGizmos->DrawGizmos(mSceneHierarchy->GetSelectedEntityRef(), mCurrentSize);
			DrawOverlayedMenu();
		}

		ImGui::End();
	}

	void Viewport::OnRender()
	{
	}

	void Viewport::OnEvent(Shared<Event> event)
	{
		if (event->GetType() == Event::Type::KeyboardPress)
		{
			auto camera = mRenderer->GetCamera();
			auto castedEvent = std::dynamic_pointer_cast<KeyboardPressEvent>(event);
			Keycode key = castedEvent->GetKeycode();

			// toggle editor viewport camera, move to viewport
			if (key == KEY_Z)
			{
				if (camera->CanMove() && camera->GetType() == Camera::Type::FREE_LOOK)
				{
					camera->SetMove(false);
					mWindow->ToggleCursor(false);
					mUI->ToggleCursor(false);
				}

				else if (!camera->CanMove() && camera->GetType() == Camera::Type::FREE_LOOK)
				{
					camera->SetMove(true);
					mWindow->ToggleCursor(true);
					mUI->ToggleCursor(true);
				}
			}
		}

		if (event->GetType() == Event::Type::MousePress)
		{
			ImVec2 cursorClickPosition = ImGui::GetMousePos();
			bool insideViewport = cursorClickPosition.x <= mContentRegionMax.x
				&& cursorClickPosition.y <= mContentRegionMax.y
				&& cursorClickPosition.x >= mContentRegionMin.x
				&& cursorClickPosition.y >= mContentRegionMin.y;

			if (insideViewport)
			{

				//// enables picking
				mRenderer->SetPicking(true);



				// this works, but i cant' read the ubo's
				//TakeScreenshot(cursorClickPosition);
				
				//// this was a bust
				//auto extent = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetSwapchain()->GetExtent();
				//auto camera = mRenderer->GetCamera();
				//glm::vec3 origin = camera->GetFrontRef();
				//glm::vec3 direction = glm::vec3(0.0f);
				//
				//ScreenPosToWorldRay((int)cursorClickPosition.x, (int)cursorClickPosition.y, (int)mCurrentSize.x, (int)mCurrentSize.y,
				//	camera->GetViewRef(), camera->GetProjectionRef(), origin, direction);
				//
				//auto meshView = mScene->GetRegistryRef().view<TransformComponent, MeshComponent>();
				//
				//for (auto ent : meshView)
				//{
				//	auto [transformComponent, meshComponent] = meshView.get<TransformComponent, MeshComponent>(ent);
				//
				//	if (meshComponent.mesh == nullptr || !meshComponent.mesh->IsLoaded())
				//		continue;
				//	
				//	float intersection_distance;
				//	glm::vec3 aabb_min(-1.0f, -1.0f, -1.0f);
				//	glm::vec3 aabb_max(1.0f, 1.0f, 1.0f);
				//
				//	if (TestRayOBBIntersection(origin, direction, aabb_min, aabb_max, transformComponent.GetTransform(), intersection_distance))
				//	{
				//		COSMOS_LOG(Logger::Trace, "Hit");
				//	}
				//}
			}
		}

		if (event->GetType() == Event::Type::WindowResize)
		{
			auto& device = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetDevice();
			auto& swapchain = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetSwapchain();
			auto& renderpass = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetRenderpassManager()->GetRenderpassesRef()["Viewport"]->GetSpecificationRef();

			vkDestroyImageView(device->GetLogicalDevice(), mDepthView, nullptr);
			vmaDestroyImage(device->GetAllocator(), mDepthImage, mDepthMemory);

			for (size_t i = 0; i < swapchain->GetImagesRef().size(); i++)
			{
				vkDestroyImageView(device->GetLogicalDevice(), mImageViews[i], nullptr);
				vmaDestroyImage(device->GetAllocator(), mImages[i], mImageMemories[i]);
			}

			for (auto& framebuffer : renderpass.frameBuffers)
			{
				vkDestroyFramebuffer(device->GetLogicalDevice(), framebuffer, nullptr);
			}

			CreateFramebufferResources();
		}
	}

	void Viewport::DrawOverlayedMenu()
	{
		// Set position to the top of the viewport
		ImGui::SetNextWindowPos(ImVec2(mContentRegionMin.x + 2.0f, mContentRegionMin.y + 2.0f));
		
		ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoDecoration 
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoScrollWithMouse
			| ImGuiWindowFlags_NoSavedSettings;

		if (ImGui::Begin("##GizmosMenu", nullptr, toolbarFlags))
		{
			ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

			static uint32_t selectedIndex = 0;
			SceneGizmos::Mode mode = SceneGizmos::Mode::UNDEFINED;
			std::string text = { ICON_LC_MOUSE_POINTER };

			for (uint32_t i = 0; i < 4; i++)
			{
				switch (i)
				{
					case 0:
					{
						mode = SceneGizmos::Mode::UNDEFINED;
						text = ICON_LC_MOUSE_POINTER;
						break;
					}

					case 1:
					{
						mode = SceneGizmos::Mode::TRANSLATE;
						text = ICON_LC_MOVE_3D;
						break;
					}

					case 2:
					{
						mode = SceneGizmos::Mode::ROTATE;
						text = ICON_LC_ROTATE_3D;
						break;
					}

					case 3:
					{
						mode = SceneGizmos::Mode::SCALE;
						text = ICON_LC_SCALE_3D;
						break;
					}

					default: break;
				}

				if (ImGui::Selectable(text.c_str(), selectedIndex == i))
				{
					mSceneGizmos->SetMode(mode);
					selectedIndex = i;
				}
			}
		}
		
		ImGui::End();
	}

	void Viewport::CreateRendererResources()
	{
		// sets the main renderpass to the viewport
		std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetRenderpassManager()->Insert("Viewport", VK_SAMPLE_COUNT_1_BIT);
		std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetRenderpassManager()->SetMain("Viewport");
		auto& renderpass = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetRenderpassManager()->GetRenderpassesRef()["Viewport"]->GetSpecificationRef();
		auto& device = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetDevice();

		mSurfaceFormat = VK_FORMAT_R8G8B8A8_SRGB;
		mDepthFormat = device->FindSuitableDepthFormat();

		// create render pass
		{
			std::array<VkAttachmentDescription, 2> attachments = {};

			// color attachment
			attachments[0].format = mSurfaceFormat;
			attachments[0].samples = renderpass.msaa;
			attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			// depth attachment
			attachments[1].format = mDepthFormat;
			attachments[1].samples = renderpass.msaa;
			attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colorReference = {};
			colorReference.attachment = 0;
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthReference = {};
			depthReference.attachment = 1;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpassDescription = {};
			subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDescription.colorAttachmentCount = 1;
			subpassDescription.pColorAttachments = &colorReference;
			subpassDescription.pDepthStencilAttachment = &depthReference;
			subpassDescription.inputAttachmentCount = 0;
			subpassDescription.pInputAttachments = nullptr;
			subpassDescription.preserveAttachmentCount = 0;
			subpassDescription.pPreserveAttachments = nullptr;
			subpassDescription.pResolveAttachments = nullptr;

			// subpass dependencies for layout transitions
			std::array<VkSubpassDependency, 2> dependencies = {};

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			dependencies[0].dependencyFlags = 0;

			dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].dstSubpass = 0;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].srcAccessMask = 0;
			dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			dependencies[1].dependencyFlags = 0;

			VkRenderPassCreateInfo renderPassCI = {};
			renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCI.attachmentCount = (uint32_t)attachments.size();
			renderPassCI.pAttachments = attachments.data();
			renderPassCI.subpassCount = 1;
			renderPassCI.pSubpasses = &subpassDescription;
			renderPassCI.dependencyCount = (uint32_t)dependencies.size();
			renderPassCI.pDependencies = dependencies.data();
			COSMOS_ASSERT(vkCreateRenderPass(device->GetLogicalDevice(), &renderPassCI, nullptr, &renderpass.renderPass) == VK_SUCCESS, "Failed to create renderpass");
		}
		
		// command pool
		{
			Vulkan::Device::QueueFamilyIndices indices = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetDevice()->FindQueueFamilies
			(
				device->GetPhysicalDevice(),
				device->GetSurface()
			);

			VkCommandPoolCreateInfo cmdPoolInfo = {};
			cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolInfo.queueFamilyIndex = indices.graphics.value();
			cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			COSMOS_ASSERT(vkCreateCommandPool(device->GetLogicalDevice(), &cmdPoolInfo, nullptr, &renderpass.commandPool) == VK_SUCCESS, "Failed to create command pool");
		}

		// command buffers
		{
			renderpass.commandBuffers.resize(mRenderer->GetConcurrentlyRenderedFramesCount());

			VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
			cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdBufferAllocInfo.commandPool = renderpass.commandPool;
			cmdBufferAllocInfo.commandBufferCount = (uint32_t)renderpass.commandBuffers.size();
			COSMOS_ASSERT(vkAllocateCommandBuffers(device->GetLogicalDevice(), &cmdBufferAllocInfo, renderpass.commandBuffers.data()) == VK_SUCCESS, "Failed to allocate command buffers");
		}

		// sampler
		{
			VkSamplerCreateInfo samplerCI = {};
			samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCI.magFilter = VK_FILTER_LINEAR;
			samplerCI.minFilter = VK_FILTER_LINEAR;
			samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCI.anisotropyEnable = VK_TRUE;
			samplerCI.maxAnisotropy = device->GetPropertiesRef().limits.maxSamplerAnisotropy;
			samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerCI.unnormalizedCoordinates = VK_FALSE;
			samplerCI.compareEnable = VK_FALSE;
			samplerCI.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			COSMOS_ASSERT(vkCreateSampler(device->GetLogicalDevice(), &samplerCI, nullptr, &mSampler) == VK_SUCCESS, "Failed to create sampler");
		}

		// framebuffers are directly tied to the renderpass, therefore we must create all it's resources
		CreateFramebufferResources();

		// recreate all global pipelines to be handled with the viewport renderpass
		std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetPipelineLibrary()->RecreatePipelines();
	}

	void Viewport::CreateFramebufferResources()
	{
		size_t imagesSize = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetSwapchain()->GetImagesRef().size();
		Shared<Vulkan::VKRenderer> vkRenderer = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer);
		auto& renderpass = vkRenderer->GetRenderpassManager()->GetRenderpassesRef()["Viewport"]->GetSpecificationRef();

		// depth buffer
		{
			vkRenderer->GetDevice()->CreateImage
			(
				vkRenderer->GetSwapchain()->GetExtent().width,
				vkRenderer->GetSwapchain()->GetExtent().height,
				1,
				1,
				renderpass.msaa,
				mDepthFormat,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				mDepthImage,
				mDepthMemory
			);

			mDepthView = vkRenderer->GetDevice()->CreateImageView
			(
				mDepthImage,
				mDepthFormat, 
				VK_IMAGE_ASPECT_DEPTH_BIT
			);
		}

		// images
		{
			mImages.resize(imagesSize);
			mImageMemories.resize(imagesSize);
			mImageViews.resize(imagesSize);
			mDescriptorSets.resize(imagesSize);
			renderpass.frameBuffers.resize(imagesSize);

			for (size_t i = 0; i < imagesSize; i++)
			{
				vkRenderer->GetDevice()->CreateImage
				(
					vkRenderer->GetSwapchain()->GetExtent().width,
					vkRenderer->GetSwapchain()->GetExtent().height,
					1,
					1,
					renderpass.msaa,
					mSurfaceFormat,
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // for picking
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					mImages[i],
					mImageMemories[i]
				);

				VkCommandBuffer command = vkRenderer->GetDevice()->BeginSingleTimeCommand(renderpass.commandPool);

				vkRenderer->GetDevice()->InsertImageMemoryBarrier
				(
					command,
					mImages[i],
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_ACCESS_MEMORY_READ_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, // must get from last render pass (undefined also works)
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // must set for next render pass
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
				);

				vkRenderer->GetDevice()->EndSingleTimeCommand(renderpass.commandPool, command);

				mImageViews[i] = vkRenderer->GetDevice()->CreateImageView(mImages[i], mSurfaceFormat, VK_IMAGE_ASPECT_COLOR_BIT);
				mDescriptorSets[i] = (VkDescriptorSet)Vulkan::VKUI::AddTextureBackend(mSampler, mImageViews[i]);

				std::array<VkImageView, 2> attachments = { mImageViews[i], mDepthView };

				VkFramebufferCreateInfo framebufferCI = {};
				framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferCI.renderPass = renderpass.renderPass;
				framebufferCI.attachmentCount = (uint32_t)attachments.size();
				framebufferCI.pAttachments = attachments.data();
				framebufferCI.width = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetSwapchain()->GetExtent().width;
				framebufferCI.height = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetSwapchain()->GetExtent().height;
				framebufferCI.layers = 1;
				COSMOS_ASSERT(vkCreateFramebuffer(vkRenderer->GetDevice()->GetLogicalDevice(), &framebufferCI, nullptr, &renderpass.frameBuffers[i]) == VK_SUCCESS, "Failed to create framebuffer");
			}
		}
	}

	void Viewport::CreatePickingResources()
	{
		auto vkRenderer = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer);
		vkRenderer->GetRenderpassManager()->Insert("Picking", VK_SAMPLE_COUNT_1_BIT);
		auto& renderpassCfg = vkRenderer->GetRenderpassManager()->GetRenderpassesRef()["Picking"]->GetSpecificationRef();

		// images
		{
			vkRenderer->GetDevice()->CreateImage
			(
				vkRenderer->GetSwapchain()->GetExtent().width,
				vkRenderer->GetSwapchain()->GetExtent().width,
				1, 1, renderpassCfg.msaa, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				mPickingColorImage, mPickingColorMemory
			);

			vkRenderer->GetDevice()->CreateImage
			(
				vkRenderer->GetSwapchain()->GetExtent().width,
				vkRenderer->GetSwapchain()->GetExtent().width,
				1, 1, renderpassCfg.msaa, mDepthFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				mPickingDepthImage, mPickingDepthMemory
			);

			mPickingColorView = vkRenderer->GetDevice()->CreateImageView(mPickingColorImage, VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
			mPickingDepthView = vkRenderer->GetDevice()->CreateImageView(mPickingDepthImage, mDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		}
		
		// render pass
		{
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = VK_FORMAT_R32_SFLOAT;
			colorAttachment.samples = renderpassCfg.msaa;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = mDepthFormat;
			depthAttachment.samples = renderpassCfg.msaa;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colorRef = {};
			colorRef.attachment = 0;
			colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthRef = {};
			depthRef.attachment = 1;
			depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpassDesc = {};
			subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDesc.colorAttachmentCount = 1;
			subpassDesc.pColorAttachments = &colorRef;
			subpassDesc.pDepthStencilAttachment = &depthRef;

			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			std::array<VkAttachmentDescription, 2 > attachmentsDesc = { colorAttachment, depthAttachment };
			VkRenderPassCreateInfo renderpassCI = {};
			renderpassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderpassCI.attachmentCount = static_cast<uint32_t>(attachmentsDesc.size());
			renderpassCI.pAttachments = attachmentsDesc.data();
			renderpassCI.subpassCount = 1;
			renderpassCI.pSubpasses = &subpassDesc;
			renderpassCI.dependencyCount = 1;
			renderpassCI.pDependencies = &dependency;
			COSMOS_ASSERT(vkCreateRenderPass(vkRenderer->GetDevice()->GetLogicalDevice(), &renderpassCI, nullptr, &renderpassCfg.renderPass) == VK_SUCCESS, "Failed to create renderpass for mouse picking");
		}

		// command pool
		{
			Vulkan::Device::QueueFamilyIndices indices = vkRenderer->GetDevice()->FindQueueFamilies(vkRenderer->GetDevice()->GetPhysicalDevice(), vkRenderer->GetDevice()->GetSurface());

			VkCommandPoolCreateInfo cmdPoolInfo = {};
			cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolInfo.queueFamilyIndex = indices.graphics.value();
			cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			COSMOS_ASSERT(vkCreateCommandPool(vkRenderer->GetDevice()->GetLogicalDevice(), &cmdPoolInfo, nullptr, &renderpassCfg.commandPool) == VK_SUCCESS, "Failed to create command pool");
		}

		// command buffers
		{
			renderpassCfg.commandBuffers.resize(mRenderer->GetConcurrentlyRenderedFramesCount());

			VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
			cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdBufferAllocInfo.commandPool = renderpassCfg.commandPool;
			cmdBufferAllocInfo.commandBufferCount = (uint32_t)renderpassCfg.commandBuffers.size();
			COSMOS_ASSERT(vkAllocateCommandBuffers(vkRenderer->GetDevice()->GetLogicalDevice(), &cmdBufferAllocInfo, renderpassCfg.commandBuffers.data()) == VK_SUCCESS, "Failed to allocate command buffers");
		}

		// frame buffer
		{
			renderpassCfg.frameBuffers.resize(1);
			std::array<VkImageView, 2> attachments = { mPickingColorView, mPickingDepthView };
			VkFramebufferCreateInfo framebufferCI = {};
			framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCI.renderPass = renderpassCfg.renderPass;
			framebufferCI.pAttachments = attachments.data();
			framebufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferCI.width = vkRenderer->GetSwapchain()->GetExtent().width;
			framebufferCI.height = vkRenderer->GetSwapchain()->GetExtent().width;
			framebufferCI.layers = 1;
			COSMOS_ASSERT(vkCreateFramebuffer(vkRenderer->GetDevice()->GetLogicalDevice(), &framebufferCI, nullptr, &renderpassCfg.frameBuffers[0]) == VK_SUCCESS, "Failed to create framebuffer for mouse picking");
		}

		// pipeline
		{
			Vulkan::Pipeline::Specification specs = {};
			specs.renderPass = vkRenderer->GetRenderpassManager()->GetRenderpassesRef()["Picking"];
			specs.vertexShader = CreateShared<Vulkan::Shader>(vkRenderer->GetDevice(), Vulkan::Shader::Type::Vertex, "Picking.vert", GetAssetSubDir("Shader/picking.vert"));
			specs.fragmentShader = CreateShared<Vulkan::Shader>(vkRenderer->GetDevice(), Vulkan::Shader::Type::Fragment, "Picking.frag", GetAssetSubDir("Shader/picking.frag"));
			specs.vertexComponents = { Vertex::Component::POSITION, Vertex::Component::UID };
		
			specs.bindings.resize(2);
		
			// model view projection
			specs.bindings[0].binding = 0;
			specs.bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			specs.bindings[0].descriptorCount = 1;
			specs.bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			specs.bindings[0].pImmutableSamplers = nullptr;
		
			// utils buffer
			specs.bindings[1].binding = 1;
			specs.bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			specs.bindings[1].descriptorCount = 1;
			specs.bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			specs.bindings[1].pImmutableSamplers = nullptr;
		
			// create
			auto pipeline = CreateShared<Vulkan::Pipeline>(vkRenderer->GetDevice(), specs);
			vkRenderer->GetPipelineLibrary()->Insert("Picking", pipeline);
			
			// modify parameters after initial creation
			pipeline->GetSpecificationRef().RSCI.cullMode = VK_CULL_MODE_BACK_BIT;
		
			// build the pipeline
			pipeline->Build(vkRenderer->GetPipelineLibrary()->GetPipelineCache());
		}
	}

	void Viewport::TakeScreenshot(ImVec2 clickedPosition)
	{
		auto vkRenderer = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer);
		auto renderpass = vkRenderer->GetRenderpassManager()->GetRenderpassesRef()["Viewport"]->GetSpecificationRef();

		// source for the copy is the last rendered viewport image
		VkImage srcImage = mImages[mRenderer->GetCurrentFrame()];
		VkImage dstImage;
		VmaAllocation dstMemory;

		// create the linear tiled destination image to copy to and to read the memory from
		vkRenderer->GetDevice()->CreateImage
		(
			vkRenderer->GetSwapchain()->GetExtent().width,
			vkRenderer->GetSwapchain()->GetExtent().height,
			1, 1, renderpass.msaa, mSurfaceFormat, VK_IMAGE_TILING_LINEAR,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			dstImage,
			dstMemory,
			0,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
		);

		// do the actual blit from the swapchain image to our host visible destination image
		VkCommandBuffer copyCmd = vkRenderer->GetDevice()->CreateCommandBuffer(renderpass.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		// transition destination image to transfer destination
		vkRenderer->GetDevice()->InsertImageMemoryBarrier
		(
			copyCmd,
			dstImage,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);

		// transition viewport image to transfer source
		vkRenderer->GetDevice()->InsertImageMemoryBarrier
		(
			copyCmd,
			srcImage,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);

		VkImageCopy imageCopyRegion = {};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = vkRenderer->GetSwapchain()->GetExtent().width;
		imageCopyRegion.extent.height = vkRenderer->GetSwapchain()->GetExtent().height;
		imageCopyRegion.extent.depth = 1;

		// issue the copy command
		vkCmdCopyImage(copyCmd, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

		// transition destination image to general layout, which is the required layout for mapping the image memory later on
		vkRenderer->GetDevice()->InsertImageMemoryBarrier
		(
			copyCmd,
			dstImage,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);

		// transition back the viewport image after the blit is done
		vkRenderer->GetDevice()->InsertImageMemoryBarrier
		(
			copyCmd,
			srcImage,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_MEMORY_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);

		vkRenderer->GetDevice()->EndCommandBuffer(renderpass.commandPool, copyCmd, vkRenderer->GetDevice()->GetGraphicsQueue(), true);

		// get layout of the image (including row pitch)
		VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
		VkSubresourceLayout subResourceLayout;
		vkGetImageSubresourceLayout(vkRenderer->GetDevice()->GetLogicalDevice(), dstImage, &subResource, &subResourceLayout);

		// map image memory so we can start copying from it
		const char* data;
		vmaMapMemory(vkRenderer->GetDevice()->GetAllocator(), dstMemory, (void**)&data);
		data += subResourceLayout.offset;

		auto pitch = subResourceLayout.rowPitch;
		
		//// copy
		//Util_Buffer buffer = {};
		//memcpy(&buffer, data, sizeof(buffer));

		std::ofstream file("ss.ppm", std::ios::out | std::ios::binary);

		// ppm header
		file << "P6\n" << vkRenderer->GetSwapchain()->GetExtent().width << "\n" << vkRenderer->GetSwapchain()->GetExtent().height << "\n" << 255 << "\n";

		for (uint32_t y = 0; y < vkRenderer->GetSwapchain()->GetExtent().height; y++)
		{
			unsigned int* row = (unsigned int*)data;
			for (uint32_t x = 0; x < vkRenderer->GetSwapchain()->GetExtent().width; x++)
			{
				file.write((char*)row, 3);
				row++;
			}
			data += subResourceLayout.rowPitch;
		}
		file.close();
		
		// destroy resources
		vmaUnmapMemory(vkRenderer->GetDevice()->GetAllocator(), dstMemory);
		vmaDestroyImage(vkRenderer->GetDevice()->GetAllocator(), dstImage, dstMemory);
	}

	SceneGizmos::SceneGizmos(Shared<Camera> camera)
		: mCamera(camera)
	{
	}

	void SceneGizmos::DrawGizmos(Shared<Entity>& entity, ImVec2 viewportSize)
	{
		// gizmos on entity
		if (!entity) return;
		if (mMode == Mode::UNDEFINED) return;
		if (!entity->HasComponent<TransformComponent>()) return;
			
		ImGuizmo::SetOrthographic(false); // 3D engine dont have orthographic but perspective
		ImGuizmo::SetDrawlist();

		// viewport rect
		float windowWidth = (float)ImGui::GetWindowWidth();
		float windowHeight = (float)ImGui::GetWindowHeight();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

		// camera
		glm::mat4 view = mCamera->GetViewRef();
		glm::mat4 proj = glm::perspectiveRH(glm::radians(mCamera->GetFov()), viewportSize.x / viewportSize.y, mCamera->GetNear(), mCamera->GetFar());

		// entity
		auto& tc = entity->GetComponent<TransformComponent>();
		glm::mat4 transform = tc.GetTransform();

		// snapping
		bool snap = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);
		float snapValue = mMode == Mode::ROTATE ? 5.0f : 0.1f;
		float snapValues[3] = { snapValue, snapValue, snapValue };

		// gizmos drawing
		ImGuizmo::Manipulate
		(
			glm::value_ptr(view),
			glm::value_ptr(proj),
			(ImGuizmo::OPERATION)mMode,
			ImGuizmo::MODE::LOCAL,
			glm::value_ptr(transform),
			nullptr,
			!snap ? snapValues : nullptr
		);

		if (ImGuizmo::IsUsing())
		{
			glm::vec3 translation, rotation, scale;
			Decompose(transform, translation, rotation, scale);

			glm::vec3 deltaRotation = rotation - tc.rotation;
			tc.translation = translation;
			tc.rotation += deltaRotation;
			tc.scale = scale;
		}
	}
}