#include "epch.h"
#include "Scene.h"

#include "Entity/Entity.h"
#include "Entity/Components/Base.h"
#include "Entity/Components/Renderable.h"
#include "Renderer/Renderer.h"

#include "Util/Logger.h"

namespace Cosmos
{
	Scene::Scene(Shared<Renderer> renderer)
		: mRenderer(renderer)
	{
	}

	Scene::~Scene()
	{
	}

	void Scene::OnUpdate(float timestep)
	{
		// update meshes
		auto meshView = mRegistry.view<IDComponent, TransformComponent, MeshComponent>();
		for (auto ent : meshView)
		{
			auto [transformComponent, meshComponent] = meshView.get<TransformComponent, MeshComponent>(ent);

			if (meshComponent.mesh == nullptr || !meshComponent.mesh->IsLoaded())
				continue;

			meshComponent.mesh->OnUpdate(timestep, transformComponent.GetTransform());
		}

		//mRenderer->SetPicking(false); // resets mouse picking
	}

	void Scene::OnRender(void* commandBuffer)
	{
		// draw models
		auto meshView = mRegistry.view<MeshComponent>();
		for (auto ent : meshView)
		{
			auto [meshComponent] = meshView.get<MeshComponent>(ent);

			if (meshComponent == nullptr || !meshComponent->IsLoaded())
				continue;

			meshComponent->OnRender(commandBuffer);
		}
	}

	void Scene::OnEvent(Shared<Event> event)
	{
	}

	Shared<Entity> Scene::CreateEntity(std::string name)
	{
		entt::entity handle = mRegistry.create();
		Shared<Entity> entity = CreateShared<Entity>(Get(), handle);

		// add id component into the entity
		entity->AddComponent<IDComponent>();

		// add name component into the entity
		entity->AddComponent<NameComponent>();
		entity->GetComponent<NameComponent>().name = name;

		mEntityMap[entity->GetComponent<IDComponent>().id] = entity;
		return mEntityMap[entity->GetComponent<IDComponent>().id];
	}

	void Scene::DestroyEntity(Shared<Entity> entity)
	{
		UUID id = entity->GetComponent<IDComponent>().id;

		if (entity->HasComponent<IDComponent>()) {
			entity->RemoveComponent<IDComponent>();
		}

		if(entity->HasComponent<NameComponent>()) {
			entity->RemoveComponent<NameComponent>();
		}

		if (entity->HasComponent<TransformComponent>()) {
			entity->RemoveComponent<TransformComponent>();
		}

		if (entity->HasComponent<MeshComponent>()) {
			entity->RemoveComponent<MeshComponent>();
		}

		mEntityMap.erase(id);
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