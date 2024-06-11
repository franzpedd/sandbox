#pragma once

#include <Engine.h>

namespace Cosmos
{
	// forward declarations
	class ImDemo;

	class Editor : public Application
	{
	public:

		// constructor
		Editor();

		// destructor
		virtual ~Editor();

	private:

		ImDemo* mImDemo;
	};
}