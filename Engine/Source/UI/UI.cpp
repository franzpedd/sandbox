#include "epch.h"
#include "UI.h"

#include "Icons.h"
#include "Theme.h"
#include "Core/Application.h"
#include "Renderer/Vulkan/VKRenderer.h"
#include "Util/Files.h"
#include "Util/Logger.h"

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

#include <imgui_impl_sdl2.cpp>
#include <imgui_impl_vulkan.cpp>

namespace Cosmos
{
	UI::UI(Application* application)
		: mApplication(application)
	{
		std::dynamic_pointer_cast<VKRenderer>(mApplication->GetRenderer())->GetRenderpassManager()->Insert("ImGui", VK_SAMPLE_COUNT_1_BIT);

		CreateResources();
		SetupConfiguration();
	}

	UI::~UI()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	void UI::OnUpdate()
	{
		// new frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		for (auto widget : mWidgets.GetElementsRef())
		{
			widget.OnUpdate();
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

	void UI::OnRender()
	{
		for (auto widget : mWidgets.GetElementsRef())
		{
			widget.OnRender();
		}
	}

	void UI::OnEvent(Shared<Event> event)
	{
		// resize the ui
		if (event->GetType() == Event::Type::WindowResize)
		{
			auto device = std::dynamic_pointer_cast<VKRenderer>(mApplication->GetRenderer())->GetDevice();
			auto swapchain = std::dynamic_pointer_cast<VKRenderer>(mApplication->GetRenderer())->GetSwapchain();
			auto& renderpass = std::dynamic_pointer_cast<VKRenderer>(mApplication->GetRenderer())->GetRenderpassManager()->GetRenderpassesRef()["ImGui"]->GetSpecificationRef();

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

				COSMOS_ASSERT
				(
					vkCreateFramebuffer
					(
						device->GetLogicalDevice(),
						&framebufferCI,
						nullptr,
						&renderpass.frameBuffers[i]
					) == VK_SUCCESS,
					"Failed to create framebuffer"
				);
			}
		}

		// resize ui objects
		for (auto widget : mWidgets.GetElementsRef())
		{
			widget.OnEvent(event);
		}
	}

	void UI::Draw(void* commandBuffer)
	{
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), (VkCommandBuffer)commandBuffer);
	}

	void UI::SetImageCount(uint32_t count)
	{
		ImGui_ImplVulkan_SetMinImageCount(count);
	}

	void UI::ToggleCursor(bool hide)
	{
		ImGuiIO& io = ImGui::GetIO();

		if (hide)
		{
			io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
			io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
			return;
		}

		io.ConfigFlags ^= ImGuiConfigFlags_NoMouse;
	}

	void UI::HandleInternalEvent(SDL_Event* e)
	{
		ImGui_ImplSDL2_ProcessEvent(e);
	}

	void UI::SetupConfiguration()
	{
		// initial config
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		if (io.BackendFlags | ImGuiBackendFlags_PlatformHasViewports && io.BackendFlags | ImGuiBackendFlags_RendererHasViewports)
		{
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		}

		static const ImWchar iconRanges1[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		static const ImWchar iconRanges2[] = { ICON_MIN_LC, ICON_MAX_LC, 0 };
		constexpr float iconSize = 13.0f;
		constexpr float fontSize = 18.0f;

		ImFontConfig iconCFG;
		iconCFG.MergeMode = true;
		iconCFG.GlyphMinAdvanceX = iconSize;
		iconCFG.PixelSnapH = true;

		io.Fonts->Clear();
		LoadFont(fontSize);

		mIconFA = io.Fonts->AddFontFromFileTTF(GetAssetSubDir("Font/icon-awesome.ttf").c_str(), iconSize, &iconCFG, iconRanges1);
		mIconLC = io.Fonts->AddFontFromFileTTF(GetAssetSubDir("Font/icon-lucide.ttf").c_str(), iconSize, &iconCFG, iconRanges2);
		io.Fonts->Build();

		io.IniFilename = "UI.ini";
		io.WantCaptureMouse = true;

		ImGui::StyleColorsDark();
		StyleColorsSpectrum();

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

		auto& vkRenderer = std::dynamic_pointer_cast<VKRenderer>(mApplication->GetRenderer());
		auto& renderpass = vkRenderer->GetRenderpassManager()->GetRenderpassesRef()["ImGui"]->GetSpecificationRef();

		VkDescriptorPoolCreateInfo poolCI = {};
		poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCI.pNext = nullptr;
		poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolCI.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
		poolCI.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
		poolCI.pPoolSizes = poolSizes;
		COSMOS_ASSERT(vkCreateDescriptorPool(vkRenderer->GetDevice()->GetLogicalDevice(), &poolCI, nullptr, &renderpass.descriptorPool) == VK_SUCCESS, "Failed to create descriptor pool for the User Interface");

		// glfw and vulkan initialization
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
	
	void UI::CreateResources()
	{
		auto& vkRenderer = std::dynamic_pointer_cast<VKRenderer>(mApplication->GetRenderer());
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
}