#include "epch.h"
#if defined COSMOS_RENDERER_VULKAN

#include "VKUI.h"

#include "VKRenderer.h"
#include "VKTexture.h"
#include "Device.h"
#include "Instance.h"
#include "Renderpass.h"
#include "Swapchain.h"

#include "Core/Application.h"
#include "Core/Event.h"
#include "UI/Widget.h"
#include "Platform/Window.h"
#include "Util/Logger.h"

// SDL
#if defined(PLATFORM_WINDOWS)
#pragma warning(push)
#pragma warning(disable : 26819)
#endif

#if defined(PLATFORM_LINUX)
#include <SDL2/SDL.h>
#elif defined(PLATFORM_WINDOWS)
#include <SDL.h>
#endif

#if defined(PLATFORM_WINDOWS)
#pragma warning(pop)
#endif

// ImGui
#include <imgui_impl_sdl2.cpp>
#include <imgui_impl_vulkan.cpp>

namespace Cosmos::Vulkan
{
	VKUI::VKUI(Cosmos::Application* application)
		: mApplication(application)
	{
		COSMOS_LOG(Logger::Todo, "Make an UI Library/Unorderedmap of currently loaded textures");
		std::dynamic_pointer_cast<Vulkan::VKRenderer>(mApplication->GetRenderer())->GetRenderpassManager()->Insert("ImGui", VK_SAMPLE_COUNT_1_BIT);

		CreateResources();
		SetupBackend();
	}

	VKUI::~VKUI()
	{
		vkDeviceWaitIdle(std::dynamic_pointer_cast<Vulkan::VKRenderer>(mApplication->GetRenderer())->GetDevice()->GetLogicalDevice());
		ImGui_ImplVulkan_Shutdown();

		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	void VKUI::OnUpdate()
	{
		ImGui_ImplVulkan_NewFrame();

		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		for (auto& widget : mWidgets.GetElementsRef())
		{
			widget->OnUpdate();
		}

		// end frame
		ImGui::Render();

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void VKUI::OnEvent(Shared<Event> event)
	{
		// resize the ui
		if (event->GetType() == Event::Type::WindowResize)
		{
			auto device = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mApplication->GetRenderer())->GetDevice();
			auto swapchain = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mApplication->GetRenderer())->GetSwapchain();
			auto& renderpass = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mApplication->GetRenderer())->GetRenderpassManager()->GetRenderpassesRef()["ImGui"]->GetSpecificationRef();

			vkDeviceWaitIdle(device->GetLogicalDevice());

			// recreate frame buffers
			for (auto framebuffer : renderpass.frameBuffers)
			{
				vkDestroyFramebuffer(device->GetLogicalDevice(), framebuffer, nullptr);
			}

			renderpass.frameBuffers.resize(swapchain->GetImageViews().size());

			for (size_t i = 0; i < swapchain->GetImageViews().size(); i++)
			{
				VkImageView attachments[] = { swapchain->GetImageViews()[i] };

				VkFramebufferCreateInfo framebufferCI = {};
				framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferCI.renderPass = renderpass.renderPass;
				framebufferCI.attachmentCount = 1;
				framebufferCI.pAttachments = attachments;
				framebufferCI.width = swapchain->GetExtent().width;
				framebufferCI.height = swapchain->GetExtent().height;
				framebufferCI.layers = 1;
				COSMOS_ASSERT(vkCreateFramebuffer(device->GetLogicalDevice(), &framebufferCI, nullptr, &renderpass.frameBuffers[i]) == VK_SUCCESS, "Failed to create framebuffer");
			}
		}

		// resize ui objects
		for (auto& widget : mWidgets.GetElementsRef())
		{
			widget->OnEvent(event);
		}
	}

	void VKUI::OnRender()
	{
		for (auto& widget : mWidgets.GetElementsRef())
		{
			widget->OnRender();
		}
	}

	void VKUI::SetupBackend()
	{
		UI::SetupFrontend();

		// create descriptor pool
		VkDescriptorPoolSize poolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		auto& vkRenderer = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mApplication->GetRenderer());
		auto& renderpass = vkRenderer->GetRenderpassManager()->GetRenderpassesRef()["ImGui"]->GetSpecificationRef();

		VkDescriptorPoolCreateInfo poolCI = {};
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.pNext = nullptr;
		poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolCI.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
		poolCI.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
		poolCI.pPoolSizes = poolSizes;
		COSMOS_ASSERT(vkCreateDescriptorPool(vkRenderer->GetDevice()->GetLogicalDevice(), &poolCI, nullptr, &renderpass.descriptorPool) == VK_SUCCESS, "Failed to create descriptor pool for the User Interface");

		// sdl and vulkan initialization
		ImGui::CreateContext();
		ImGui_ImplSDL2_InitForVulkan(mApplication->GetWindow()->GetNativeWindow());

		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = vkRenderer->GetInstance()->GetInstance();
		initInfo.PhysicalDevice = vkRenderer->GetDevice()->GetPhysicalDevice();
		initInfo.Device = vkRenderer->GetDevice()->GetLogicalDevice();
		initInfo.Queue = vkRenderer->GetDevice()->GetGraphicsQueue();
		initInfo.DescriptorPool = renderpass.descriptorPool;
		initInfo.MinImageCount = vkRenderer->GetSwapchain()->GetImageCount();
		initInfo.ImageCount = vkRenderer->GetSwapchain()->GetImageCount();
		initInfo.MSAASamples = renderpass.msaa;
		initInfo.Allocator = nullptr;
		initInfo.RenderPass = renderpass.renderPass;
		ImGui_ImplVulkan_Init(&initInfo);

		// upload fonts
		ImGui_ImplVulkan_CreateFontsTexture();
	}

	void VKUI::Draw(void* commandBuffer)
	{
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), (VkCommandBuffer)commandBuffer);
	}

	void VKUI::SetImageCount(uint32_t count)
	{
		ImGui_ImplVulkan_SetMinImageCount(count);
	}

	void VKUI::CreateResources()
	{
		auto& vkRenderer = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mApplication->GetRenderer());
		auto& renderpass = vkRenderer->GetRenderpassManager()->GetRenderpassesRef()["ImGui"]->GetSpecificationRef();

		// render pass
		VkAttachmentDescription attachment = {};
		attachment.format = vkRenderer->GetSwapchain()->GetSurfaceFormat().format;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachment = {};
		colorAttachment.attachment = 0;
		colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachment;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = 1;
		info.pAttachments = &attachment;
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.dependencyCount = 1;
		info.pDependencies = &dependency;
		COSMOS_ASSERT(vkCreateRenderPass(vkRenderer->GetDevice()->GetLogicalDevice(), &info, nullptr, &renderpass.renderPass) == VK_SUCCESS, "Failed to create render pass");

		// command pool
		Vulkan::Device::QueueFamilyIndices indices = vkRenderer->GetDevice()->FindQueueFamilies
		(
			vkRenderer->GetDevice()->GetPhysicalDevice(),
			vkRenderer->GetDevice()->GetSurface()
		);

		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = indices.graphics.value();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		COSMOS_ASSERT(vkCreateCommandPool(vkRenderer->GetDevice()->GetLogicalDevice(), &cmdPoolInfo, nullptr, &renderpass.commandPool) == VK_SUCCESS, "Failed to create command pool");

		// command buffers
		renderpass.commandBuffers.resize(mApplication->GetRenderer()->GetConcurrentlyRenderedFramesCount());

		VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocInfo.commandPool = renderpass.commandPool;
		cmdBufferAllocInfo.commandBufferCount = (uint32_t)renderpass.commandBuffers.size();
		COSMOS_ASSERT(vkAllocateCommandBuffers(vkRenderer->GetDevice()->GetLogicalDevice(), &cmdBufferAllocInfo, renderpass.commandBuffers.data()) == VK_SUCCESS, "Failed to allocate command buffers");

		// frame buffers
		renderpass.frameBuffers.resize(vkRenderer->GetSwapchain()->GetImageViews().size());

		for (size_t i = 0; i < vkRenderer->GetSwapchain()->GetImageViews().size(); i++)
		{
			VkImageView attachments[] = { vkRenderer->GetSwapchain()->GetImageViews()[i] };

			VkFramebufferCreateInfo framebufferCI = {};
			framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCI.renderPass = renderpass.renderPass;
			framebufferCI.attachmentCount = 1;
			framebufferCI.pAttachments = attachments;
			framebufferCI.width = vkRenderer->GetSwapchain()->GetExtent().width;
			framebufferCI.height = vkRenderer->GetSwapchain()->GetExtent().height;
			framebufferCI.layers = 1;
			COSMOS_ASSERT(vkCreateFramebuffer(vkRenderer->GetDevice()->GetLogicalDevice(), &framebufferCI, nullptr, &renderpass.frameBuffers[i]) == VK_SUCCESS, "Failed to create framebuffer");
		}
	}

	void VKUI::HandleSDLEvent(SDL_Event* e)
	{
		ImGui_ImplSDL2_ProcessEvent(e);
	}

	void* VKUI::AddTextureBackend(Shared<Texture2D> texture)
	{
		ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
		ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;

		// create descriptor set
		VkDescriptorSet descriptorSet;
		{
			VkDescriptorSetAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			alloc_info.descriptorPool = v->DescriptorPool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = &bd->DescriptorSetLayout;
			VkResult err = vkAllocateDescriptorSets(v->Device, &alloc_info, &descriptorSet);
			check_vk_result(err);
		}

		// update descriptor set
		{
			VkDescriptorImageInfo desc_image[1] = {};
			desc_image[0].sampler = (VkSampler)std::dynamic_pointer_cast<Vulkan::VKTexture2D>(texture)->GetSampler();
			desc_image[0].imageView = (VkImageView)std::dynamic_pointer_cast<Vulkan::VKTexture2D>(texture)->GetView();
			desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet write_desc[1] = {};
			write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_desc[0].dstSet = descriptorSet;
			write_desc[0].descriptorCount = 1;
			write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write_desc[0].pImageInfo = desc_image;
			vkUpdateDescriptorSets(v->Device, 1, write_desc, 0, nullptr);
		}

		return descriptorSet;
	}

	void* VKUI::AddTextureBackend(void* sampler, void* view)
	{
		ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
		ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;

		// create descriptor set
		VkDescriptorSet descriptorSet;
		{
			VkDescriptorSetAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			alloc_info.descriptorPool = v->DescriptorPool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = &bd->DescriptorSetLayout;
			VkResult err = vkAllocateDescriptorSets(v->Device, &alloc_info, &descriptorSet);
			check_vk_result(err);
		}

		// update descriptor set
		{
			VkDescriptorImageInfo desc_image[1] = {};
			desc_image[0].sampler = (VkSampler)sampler;
			desc_image[0].imageView = (VkImageView)view;
			desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet write_desc[1] = {};
			write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_desc[0].dstSet = descriptorSet;
			write_desc[0].descriptorCount = 1;
			write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write_desc[0].pImageInfo = desc_image;
			vkUpdateDescriptorSets(v->Device, 1, write_desc, 0, nullptr);
		}

		return descriptorSet;
	}

	void VKUI::RemoveTexture(void* descriptor)
	{
		ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
		ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;
		vkDeviceWaitIdle(v->Device);
		vkFreeDescriptorSets(v->Device, v->DescriptorPool, 1, &(VkDescriptorSet&)descriptor);
	}
}

#endif