//#include <iostream>
//#include <unordered_map>
//#include <vector>
//#include <typeindex>
//#include <string>
//#include <thread>
//#include <algorithm>
//
//#define _CRTDBG_MAP_ALLOC
//
//
//#include <stdlib.h>
//#include <crtdbg.h>
//
//using Entity = size_t;
//using Signature = size_t;
//using ArchetypeID = size_t;
//using Archetype = std::vector<std::type_index>;
//
//struct Position
//{
//	float x;
//	float y;
//	float z;
//};
//
//struct Mesh
//{
//	char name[260];
//	//std::string name = ""; //this std::string shit gives memory leaks, when change to example int, it doesn't give memory leaks
//};
//
//struct Collider
//{
//	float size;
//};
//
//class ArchetypeManager
//{
//public:
//	void Update(const Entity aEntityID, const Archetype& aArchetype)
//	{
//		const auto& componentTypes = aArchetype;
//
//		if (componentTypes.empty())
//			return;
//
//		const ArchetypeID archetypeID = GenerateArchetypeID(aArchetype);
//		myEntitiesInArchetype[archetypeID].push_back(aEntityID);
//	}
//
//	template <typename... ComponentTypes>
//	const std::vector<Entity> GetEntitiesWithComponents()
//	{
//		const ArchetypeID signature = GetArchetypeID<ComponentTypes...>();
//
//		if (myEntitiesInArchetype.find(signature) != myEntitiesInArchetype.end())
//			return myEntitiesInArchetype.at(signature);
//		else
//			return std::vector<Entity>();
//	}
//
//	void RemoveEntityFromArchetype(const Entity aEntityID, const Archetype& aArchetype)
//	{
//		const ArchetypeID archetypeID = GenerateArchetypeID(aArchetype);
//		const auto& archetype = myEntitiesInArchetype.find(archetypeID);
//
//		if (archetype != myEntitiesInArchetype.end())
//		{
//			auto& entityList = archetype->second;
//			auto entity = std::find(entityList.begin(), entityList.end(), aEntityID);
//
//			if (entity != entityList.end())
//			{
//				entityList.erase(entity);
//			}
//		}
//	}
//private:
//	struct ChatGPTHasher
//	{
//		Signature operator()(const Archetype& vec) const
//		{
//			Signature seed = vec.size();
//
//			for (const auto& item : vec)
//			{
//				seed ^= item.hash_code() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//			}
//
//			return seed;
//		}
//	};
//
//	bool operator()(const Archetype& lhs, const Archetype& rhs) const
//	{
//		return lhs == rhs;
//	}
//
//	template <typename ...ComponentTypes>
//	const ArchetypeID GetArchetypeID()
//	{
//		const Archetype types = { typeid(ComponentTypes)... };
//		return GenerateArchetypeID(types);
//	}
//
//	const ArchetypeID GenerateArchetypeID(const Archetype& aArchetype)
//	{
//		Archetype archetype = aArchetype;
//
//		std::sort(archetype.begin(), archetype.end(), [](const std::type_index& a, const std::type_index& b) //To make order not matter e.g {Transform, Mesh} is {Mesh, Transform}
//			{
//				return a.hash_code() < b.hash_code();
//			});
//
//		ChatGPTHasher hasher;
//		const Signature hash = hasher(archetype);
//
//		const auto& it = mySignatureToArchetype.find(hash);
//		if (it == mySignatureToArchetype.end())
//		{
//			mySignatureToArchetype[hash] = archetype;
//		}
//
//		if (myArchetypeToArchetypeID.find(archetype) == myArchetypeToArchetypeID.end())
//		{
//			myArchetypeToArchetypeID[archetype] = myCurrentID++;
//		}
//
//		return myArchetypeToArchetypeID[archetype];
//	}
//private:
//	ArchetypeID myCurrentID = 0;
//	std::unordered_map<ArchetypeID, std::vector<Entity>> myEntitiesInArchetype;
//	std::unordered_map<Archetype, ArchetypeID, ChatGPTHasher> myArchetypeToArchetypeID;
//	std::unordered_map<Signature, Archetype> mySignatureToArchetype;
//};
//
//class ComponentManager
//{
//public:
//	void OnDestroy()
//	{
//		for (const auto& entity : myComponentsInEntity)
//		{
//			for (auto& component : entity.second)
//			{
//				if (component.second)
//					delete component.second;
//			}
//		}
//		myComponentsInEntity.clear();
//	}
//
//	template<typename ComponentType>
//	void AddComponent(const Entity aEntityID, const ComponentType& aComponent)
//	{
//		const std::type_index typeIndex = typeid(ComponentType);
//		const auto& currentArchetype = GetAllComponents(aEntityID);
//
//		myArchetypeManager.RemoveEntityFromArchetype(aEntityID, currentArchetype);
//
//		if (myComponentsInEntity[aEntityID].count(typeIndex) == 0)
//		{
//			myComponentsInEntity[aEntityID][typeIndex] = new ComponentType(aComponent);
//		}
//
//		const auto& newArchetype = GetAllComponents(aEntityID);
//		myArchetypeManager.Update(aEntityID, newArchetype);
//	}
//
//	template<typename ComponentType>
//	void RemoveComponent(const Entity aEntityID, const ComponentType& aComponent)
//	{
//		const std::type_index typeIndex = typeid(aComponent);
//
//		if (myComponentsInEntity[aEntityID].count(typeIndex) == 0)
//			return;
//
//		const auto& currentArchetype = GetAllComponents(aEntityID);
//		myArchetypeManager.RemoveEntityFromArchetype(aEntityID, currentArchetype);
//
//		void* componentToRemove = myComponentsInEntity[aEntityID][typeIndex];
//
//		delete static_cast<ComponentType*>(componentToRemove);
//		myComponentsInEntity[aEntityID].erase(typeIndex);
//
//		const auto& newArchetype = GetAllComponents(aEntityID);
//		myArchetypeManager.Update(aEntityID, newArchetype);
//	}
//
//	template<typename ComponentType>
//	ComponentType& GetComponent(const Entity aEntityID)
//	{
//		const std::type_index typeIndex = typeid(ComponentType);
//		return *static_cast<ComponentType*>(myComponentsInEntity[aEntityID][typeIndex]);
//	}
//
//	template <typename... ComponentTypes>
//	const std::vector<Entity> GetEntitiesWithComponents()
//	{
//		return myArchetypeManager.GetEntitiesWithComponents<ComponentTypes...>();
//	}
//
//	const std::vector<std::type_index> GetAllComponents(const Entity aEntityID) const
//	{
//		Archetype components;
//
//		const auto& component = myComponentsInEntity.find(aEntityID);
//
//		if (component != myComponentsInEntity.end())
//		{
//			const auto& innerMap = component->second;
//
//			for (const auto& componentPair : innerMap)
//			{
//				components.push_back(componentPair.first);
//			}
//		}
//
//		return components;
//	}
//private:
//	ArchetypeManager myArchetypeManager;
//	std::unordered_map<Entity, std::unordered_map<std::type_index, void*>> myComponentsInEntity;
//};
//
//class EntityManager
//{
//public:
//	EntityManager()
//	{
//		myEntities.reserve(10000);
//	}
//
//	const Entity CreateEntity()
//	{
//		static Entity id = 1;
//		myEntities.push_back(id);
//		return id++;
//	}
//
//	const std::vector<Entity>& GetEntities() const
//	{
//		return myEntities;
//	}
//private:
//	std::vector<Entity> myEntities;
//};
//
//class MovementSystem
//{
//public:
//	void Update(const float aDeltaTime, ComponentManager& aComponentManager)
//	{
//		const auto& entities = aComponentManager.GetEntitiesWithComponents<Position, Collider>();
//
//		const Entity chunkSize = static_cast<Entity>(entities.size() / numThreads);
//
//		for (size_t i = 0; i < numThreads; ++i)
//		{
//			const size_t index = i;
//
//			threads.emplace_back([&]
//				{
//					Entity startIdx = static_cast<Entity>(index * chunkSize);
//					Entity endIdx = static_cast<size_t>((index + 1)) == numThreads ? static_cast<Entity>(entities.size()) : static_cast<Entity>((index + 1) * chunkSize);
//
//					ProcessEntities(aDeltaTime, aComponentManager, entities.begin() + startIdx, entities.begin() + endIdx);
//				});
//		}
//
//		for (auto& thread : threads)
//		{
//			if (thread.joinable())
//				thread.join();
//		}
//	}
//private:
//	void ProcessEntities(const float aDeltaTime, ComponentManager& aComponentManager,
//		std::vector<Entity>::const_iterator start, std::vector<Entity>::const_iterator end)
//	{
//		for (auto& it = start; it != end; ++it)
//		{
//			Position& position = aComponentManager.GetComponent<Position>(*it);
//			position.x += 1.0f * aDeltaTime;
//		}
//	}
//private:
//	const unsigned int numThreads = std::thread::hardware_concurrency();
//	std::vector<std::thread> threads;
//};
//
//int main()
//{
//	{
//		EntityManager entityManager;
//		ComponentManager componentManager;
//
//		Entity entity;
//
//		for (int i = 0; i < 100; ++i)
//		{
//			entity = entityManager.CreateEntity();
//			componentManager.AddComponent(entity, Position{ 1.0f, 1.0f });
//		}
//
//		for (int i = 0; i < 50; ++i)
//		{
//			entity = entityManager.CreateEntity();
//			componentManager.AddComponent(entity, Position{ 1.0f, 1.0f });
//			componentManager.AddComponent(entity, Collider{ 10 });
//		}
//
//		MovementSystem moveSystem;
//		moveSystem.Update(1.0f, componentManager);
//
//		componentManager.OnDestroy();
//	}
//
//	_CrtDumpMemoryLeaks();
//
//	return 0;
//}
