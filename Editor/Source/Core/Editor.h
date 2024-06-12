#pragma once

#include <Engine.h>

namespace Cosmos
{
	// forward declarations
	class Dockspace;
	class ImDemo;
	class Viewport;

	class Editor : public Application
	{
	public:

		// constructor
		Editor();

		// destructor
		virtual ~Editor();

	private:

		Dockspace* mDockspace;
		ImDemo* mImDemo;
		Viewport* mViewport;
	};
}