#pragma once

#include <Engine.h>

namespace Cosmos
{
	class SceneHierarchy : public Widget
	{
	public:

		// constructor
		SceneHierarchy(Shared<Renderer> renderer, Shared<Scene> scene);

		// destructor
		~SceneHierarchy();

		// returns a pointer to the currently selected entity
		inline Shared<Entity> GetSelectedEntity() const { return mSelectedEntity; }

		// unslects the selected entity
		inline void UnselectEntity() { mSelectedEntity = nullptr; }

	public:

		// updates the widget
		virtual void OnUpdate() override;

	private:

		// sub-menu with scene hierarchy
		void DisplaySceneHierarchy();

		// draws a existing entity node on hierarchy menu, redraw is used to check if entity map was altered and needs to be redrawn
		void DrawEntityNode(Shared<Entity> entity, bool* redraw);

		// sub-menu with selected entity components on display
		void DisplaySelectedEntityComponents();

		// draws all components of a given entity
		void DrawComponents(Shared<Entity> entity);

		// adds a menu option with the component name
		template<typename T>
		void DisplayAddComponentEntry(const char* name);

		// draws a single component and forwards the function
		template<typename T, typename F>
		static void DrawComponent(const char* name, Shared<Entity> entity, F func);

	private:

		Shared<Renderer> mRenderer;
		Shared<Scene> mScene;
		Shared<Entity> mSelectedEntity = nullptr; // working with only one selected entity at a time
	};
}