#pragma once

#include "Core/Scene.h"

#include "Util/UUID.h"
#include "Wrapper/entt.h"

namespace Cosmos
{
	class Entity
	{
	public:

		// constructor
		Entity(Shared<Scene> scene, entt::entity handle) : mScene(scene), mHandle(handle) {}

		// destructor
		~Entity() = default;

		// returns the entity handle
		inline entt::entity GetHandle() const { return mHandle; }

	public:

		// returns if the entity is valid
		operator bool() const { return mHandle != entt::null; }

	public:

		// checks if entity has a certain component
		template<typename T>
		bool HasComponent()
		{
			return mScene->GetRegistryRef().all_of<T>(mHandle);
		}

		// returns the component
		template<typename T>
		T& GetComponent()
		{
			return mScene->GetRegistryRef().get<T>(mHandle);
		}

		// adds a component for the entity
		template<typename T, typename...Args>
		T& AddComponent(Args&&... args)
		{
			return mScene->GetRegistryRef().emplace_or_replace<T>(mHandle, std::forward<Args>(args)...);
		}

		// removes the component
		template<typename T>
		void RemoveComponent()
		{
			mScene->GetRegistryRef().remove<T>(mHandle);
		}

	private:

		Shared<Scene> mScene;
		entt::entity mHandle;
	};
}