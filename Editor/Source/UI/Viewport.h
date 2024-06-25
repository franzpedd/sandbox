#pragma once

#define COSMOS_RENDERER_VULKAN
#include <Engine.h>

namespace Cosmos
{
	// forward declarations
	class SceneHierarchy;
	class SceneGizmos;

	class Viewport : public Widget
	{
	public:

		// constructor
		Viewport(Shared<Window> window, Shared<Renderer> renderer, Shared<UI> ui, SceneHierarchy* sceneHierarchy);

		// destructor
		virtual ~Viewport();

	public:

		// for user interface drawing
		virtual void OnUpdate();

		// for renderer drawing
		virtual void OnRender();

		// // called when the window is resized
		virtual void OnEvent(Shared<Event> event);

	private:

		// draws an overlayed menu into the viewport, for gizmos operation and more
		void DrawOverlayedMenu();

		// creates all renderer resources
		void CreateRendererResources();

		// creates all framebuffer resources
		void CreateFramebufferResources();

	private:

		Shared<Window> mWindow;
		Shared<Renderer> mRenderer;
		Shared<UI> mUI;
		SceneHierarchy* mSceneHierarchy;

		// gizmos
		Shared<SceneGizmos> mSceneGizmos;

		// ui resources
		ImVec2 mCurrentSize;
		ImVec2 mContentRegionMin;
		ImVec2 mContentRegionMax;

		// vulkan resources
		VkFormat mSurfaceFormat = VK_FORMAT_UNDEFINED;
		VkFormat mDepthFormat = VK_FORMAT_UNDEFINED;
		VkSampler mSampler = VK_NULL_HANDLE;

		VkImage mDepthImage = VK_NULL_HANDLE;
		VmaAllocation mDepthMemory = VK_NULL_HANDLE;
		VkImageView mDepthView = VK_NULL_HANDLE;

		std::vector<VkImage> mImages;
		std::vector<VmaAllocation> mImageMemories;
		std::vector<VkImageView> mImageViews;

		std::vector<VkDescriptorSet> mDescriptorSets;
	};

	class SceneGizmos
	{
	public:
		typedef enum Mode
		{
			UNDEFINED = (0u << 0),

			TRANSLATE_X = (1u << 0),
			TRANSLATE_Y = (1u << 1),
			TRANSLATE_Z = (1u << 2),
			ROTATE_X = (1u << 3),
			ROTATE_Y = (1u << 4),
			ROTATE_Z = (1u << 5),
			ROTATE_SCREEN = (1u << 6),
			SCALE_X = (1u << 7),
			SCALE_Y = (1u << 8),
			SCALE_Z = (1u << 9),
			BOUNDS = (1u << 10),
			SCALE_XU = (1u << 11),
			SCALE_YU = (1u << 12),
			SCALE_ZU = (1u << 13),

			TRANSLATE = TRANSLATE_X | TRANSLATE_Y | TRANSLATE_Z,
			ROTATE = ROTATE_X | ROTATE_Y | ROTATE_Z | ROTATE_SCREEN,
			SCALE = SCALE_X | SCALE_Y | SCALE_Z,
			SCALEU = SCALE_XU | SCALE_YU | SCALE_ZU, // universal
			UNIVERSAL = TRANSLATE | ROTATE | SCALEU

		} Mode;

	public:

		// constructor
		SceneGizmos(Shared<Camera> camera);

		// destructor
		~SceneGizmos() = default;

		// returns current gizmo's mode
		inline Mode GetMode() const { return mMode; }

		// sets a new gizmos operation mode
		inline void SetMode(Mode mode) { mMode = mode; }

	public:

		// renders gizmos logic
		void DrawGizmos(Shared<Entity>& selectedEntity, ImVec2 viewportSize);

	private:

		Shared<Camera> mCamera;
		Mode mMode = Mode::UNDEFINED;
		bool mSelectedButton = false;
	};
}