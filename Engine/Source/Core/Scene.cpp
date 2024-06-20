#include "epch.h"
#include "Scene.h"

#include "Entity/Entity.h"
#include "Entity/Components/Base.h"
#include "Entity/Components/Renderable.h"

#include "Util/Logger.h"

namespace Cosmos
{
	Scene::Scene(Shared<Renderer> renderer)
	{
	}

	Scene::~Scene()
	{
	}

	void Scene::OnUpdate(float timestep)
	{
		// update meshes
		auto meshView = mRegistry.view<TransformComponent, MeshComponent>();
		for (auto ent : meshView)
		{
			auto [transformComponent, meshComponent] = meshView.get<TransformComponent, MeshComponent>(ent);

			if (meshComponent.mesh == nullptr || !meshComponent.mesh->IsLoaded())
				continue;

			meshComponent.mesh->OnUpdate(timestep, transformComponent.GetTransform());
		}
	}

	void Scene::OnRender()
	{
		// draw models
		auto meshView = mRegistry.view<MeshComponent>();
		for (auto ent : meshView)
		{
			auto [meshComponent] = meshView.get<MeshComponent>(ent);

			if (meshComponent == nullptr || !meshComponent->IsLoaded())
				continue;

			meshComponent->OnRender();
		}
	}

	void Scene::OnEvent(Shared<Event> event)
	{
	}

	Shared<Entity> Scene::CreateEntity(std::string name)
	{
		UUID id = UUID();
		entt::entity handle = mRegistry.create();
		Shared<Entity> entity = CreateShared<Entity>(Get(), handle, id);

		// add name component into the entity
		entity->AddComponent<NameComponent>();
		entity->GetComponent<NameComponent>().name = name;

		mEntityMap[id] = entity;
		return mEntityMap[id];
	}

	void Scene::DestroyEntity(Shared<Entity> entity)
	{
		if(entity->HasComponent<NameComponent>()) {
			entity->RemoveComponent<NameComponent>();
		}

		if (entity->HasComponent<TransformComponent>()) {
			entity->RemoveComponent<TransformComponent>();
		}

		mEntityMap.erase(entity->GetID());
	}

	Shared<Entity> Scene::FindEntityById(UUID id)
	{
		auto it = mEntityMap.find(id);

		if (it != mEntityMap.end())
		{
			return it->second;
		}

		COSMOS_LOG(Logger::Error, "Could not find any entity with id %d", id.GetValue());
		return nullptr;
	}
}