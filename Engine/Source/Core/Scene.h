#pragma once

#include "Util/Memory.h"

namespace Cosmos
{
	// forward declarations
	class Event;
	class Renderer;

	class Scene
	{
	public:

		// constructor
		Scene(Shared<Renderer> renderer);

		// destructor
		~Scene();

	public:

		// updates the scene logic
		void OnUpdate(float timestep);

		// draw scene objects
		void OnRender();

		// handle events
		void OnEvent(Shared<Event> event);

	private:

		Shared<Renderer> mRenderer;
	};
}