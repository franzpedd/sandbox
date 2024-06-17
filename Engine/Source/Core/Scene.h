#pragma once

#include "Util/Memory.h"
#include "Util/UUID.h"
#include "Wrapper/entt.h"

namespace Cosmos
{
	// forward declarations
	class Entity;
	class Event;
	class Renderer;

	class Scene : public std::enable_shared_from_this<Scene>
	{
	public:

		// constructor
		Scene(Shared<Renderer> renderer);

		// destructor
		~Scene();

		// returns the shared pointer from this class
		inline Shared<Scene> Get() { return shared_from_this(); }

		// returns a reference to the registry
		inline entt::registry& GetRegistryRef() { return mRegistry; }

		// returns a reference to the entity map
		inline std::unordered_map<std::string, Shared<Entity>>& GetEntityMapRef() { return mEntityMap; }

	public:

		// updates the scene logic
		void OnUpdate(float timestep);

		// draw scene objects
		void OnRender();

		// handle events
		void OnEvent(Shared<Event> event);

	public:

		// creates and returns an empty entity
		Shared<Entity> CreateEntity(std::string name);

		// destroy and erases an entity
		void DestroyEntity(Shared<Entity> entity);

		// finds an entity by it's identifier
		Shared<Entity> Scene::FindEntityById(UUID id);

	private:

		Shared<Renderer> mRenderer;
		entt::registry mRegistry;
		std::unordered_map<std::string, Shared<Entity>> mEntityMap;
	};
}