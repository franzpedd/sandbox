#pragma once

#include <Engine.h>

namespace Cosmos
{
	// forward declarations
	class Grid;

	class Menubar : public Widget
	{
	public:

		enum Action
		{
			None = 0,
			New,
			Open,
			Save,
			SaveAs
		};

	public:

		// constructor
		Menubar(Application* application, Shared<Window> window, Shared<Renderer> renderer, Grid* grid);

		// destructor
		virtual ~Menubar() = default;

	public:

		// updates the ui element
		virtual void OnUpdate() override;

	private:

		// drawing and logic of main menu
		void DisplayMainMenu();

		// handles selected menu option
		void HandleMenuAction();

		// draws teh scene settings
		void SceneSettingsWindow();

	private:

		Application* mApplication;
		Shared<Window> mWindow;
		Shared<Renderer> mRenderer;
		Grid* mGrid;

		bool mCheckboxGrid = true;
		Action mMenuAction = Action::None;
		bool mCancelAction = false;

		bool mDisplaySceneSettings = false;
	};
}