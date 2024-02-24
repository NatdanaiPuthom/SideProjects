#pragma once
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <stack>
#include <mutex>
#include <shared_mutex>

#include "Entity.h"

namespace ecs
{
	class ArchetypeManager
	{
	public:
		void Update(const Entity aEntityID, const Archetype& aArchetype)
		{
			const auto& componentTypes = aArchetype;

			if (componentTypes.empty())
				return;

			const ArchetypeID archetypeID = GenerateArchetypeID(aArchetype);

			std::lock_guard<std::mutex> lock(myArchetypeMutex);
			myEntitiesInArchetype[archetypeID].push_back(aEntityID);
		}

		template <typename... ComponentTypes>
		inline const std::vector<Entity> GetEntitiesWithComponents()
		{
			const ArchetypeID signature = GetArchetypeID<ComponentTypes...>();

			if (myEntitiesInArchetype.find(signature) != myEntitiesInArchetype.end())
				return myEntitiesInArchetype.at(signature);
			else
				return std::vector<Entity>();
		}

		const std::unordered_map<ArchetypeID, std::vector<Entity>>& GetEntitiesInAllArchetype() const
		{
			return myEntitiesInArchetype;
		}

		void RemoveEntityFromArchetype(const Entity aEntityID, const Archetype& aArchetype)
		{
			const ArchetypeID archetypeID = GenerateArchetypeID(aArchetype);

			std::lock_guard<std::mutex> lock(myArchetypeMutex);
			const auto& archetype = myEntitiesInArchetype.find(archetypeID);

			if (archetype != myEntitiesInArchetype.end())
			{
				auto& entityList = archetype->second;
				auto entity = std::find(entityList.begin(), entityList.end(), aEntityID);

				if (entity != entityList.end())
				{
					entityList.erase(entity);
				}
			}
		}
	private:
		struct ChatGPTHasher
		{
			Signature operator()(const Archetype& vec) const
			{
				Signature seed = vec.size();

				for (const auto& item : vec)
				{
					seed ^= item.hash_code() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				}

				return seed;
			}
		};

		bool operator()(const Archetype& lhs, const Archetype& rhs) const
		{
			return lhs == rhs;
		}

		template <typename ...ComponentTypes>
		inline const ArchetypeID GetArchetypeID()
		{
			const Archetype types = { typeid(ComponentTypes)... };
			return GenerateArchetypeID(types);
		}

		const ArchetypeID GenerateArchetypeID(const Archetype& aArchetype)
		{
			Archetype archetype = aArchetype;

			std::sort(archetype.begin(), archetype.end(), [](const std::type_index& a, const std::type_index& b) //To make order not matter e.g {Transform, Mesh} is {Mesh, Transform}
				{
					return a.hash_code() < b.hash_code();
				});

			ChatGPTHasher hasher;
			const Signature hash = hasher(archetype);

			std::lock_guard<std::mutex> lock(myGenerateIDMutex);

			const auto& it = mySignatureToArchetype.find(hash);
			if (it == mySignatureToArchetype.end())
			{
				mySignatureToArchetype[hash] = archetype;
			}

			if (myArchetypeToArchetypeID.find(archetype) == myArchetypeToArchetypeID.end())
			{
				myArchetypeToArchetypeID[archetype] = myCurrentID++;
			}

			return myArchetypeToArchetypeID[archetype];
		}
	private:
		mutable std::shared_mutex myArchetypeSharedMutex;
		mutable std::mutex myArchetypeMutex;
		mutable std::mutex myGenerateIDMutex;
		ArchetypeID myCurrentID = 0;
		std::unordered_map<ArchetypeID, std::vector<Entity>> myEntitiesInArchetype;
		std::unordered_map<Archetype, ArchetypeID, ChatGPTHasher> myArchetypeToArchetypeID;
		std::unordered_map<Signature, Archetype> mySignatureToArchetype;
	};

	class ComponentManager
	{
	private:
		class BaseComponentWrapper
		{
		public:
			virtual ~BaseComponentWrapper() = 0 {}
		};

		template<class ComponentType>
		class ComponentWrapper : public BaseComponentWrapper
		{
		public:

			inline ComponentWrapper(const ComponentType& aComponent)
				: myComponent(aComponent)
			{

			}

			ComponentType myComponent;
		};
	public:
		void OnDestroy()
		{
			for (const auto& entity : myComponentsInEntity)
			{
				RemoveAllComponents(entity.first);
			}

			myComponentsInEntity.clear();
		}

		template<typename ComponentType>
		inline void AddComponent(const Entity aEntityID, const ComponentType& aComponent)
		{
			const std::type_index typeIndex = typeid(ComponentType);

			const auto& currentArchetype = GetAllComponents(aEntityID);

			std::lock_guard<std::mutex> lock(myComponentMutex);
			myArchetypeManager.RemoveEntityFromArchetype(aEntityID, currentArchetype);

			if (myComponentsInEntity[aEntityID].count(typeIndex) == 0)
			{
				myComponentsInEntity[aEntityID][typeIndex] = new ComponentWrapper<ComponentType>(aComponent);;
			}

			const auto& newArchetype = GetAllComponents(aEntityID);
			myArchetypeManager.Update(aEntityID, newArchetype);
		}

		template<typename ComponentType>
		inline void RemoveComponent(const Entity aEntityID, const ComponentType& aComponent)
		{
			const std::type_index typeIndex = typeid(aComponent);

			if (myComponentsInEntity[aEntityID].count(typeIndex) == 0)
				return;

			const auto& currentArchetype = GetAllComponents(aEntityID);
			myArchetypeManager.RemoveEntityFromArchetype(aEntityID, currentArchetype);

			std::lock_guard<std::mutex> lock(myComponentMutex);
			BaseComponentWrapper* componentToRemove = myComponentsInEntity[aEntityID][typeIndex];

			delete componentToRemove;
			myComponentsInEntity[aEntityID].erase(typeIndex);

			const auto& newArchetype = GetAllComponents(aEntityID);
			myArchetypeManager.Update(aEntityID, newArchetype);
		}

		inline void RemoveComponent(const Entity aEntityID, const std::type_index& aComponent)
		{
			if (myComponentsInEntity[aEntityID].count(aComponent) == 0)
				return;

			const auto& currentArchetype = GetAllComponents(aEntityID);
			myArchetypeManager.RemoveEntityFromArchetype(aEntityID, currentArchetype);

			std::lock_guard<std::mutex> lock(myComponentMutex);
			BaseComponentWrapper* componentToRemove = myComponentsInEntity[aEntityID][aComponent];

			delete componentToRemove;
			myComponentsInEntity[aEntityID].erase(aComponent);

			const auto& newArchetype = GetAllComponents(aEntityID);
			myArchetypeManager.Update(aEntityID, newArchetype);
		}

		void RemoveAllComponents(const Entity aEntityID)
		{
			const std::vector<std::type_index> allComponentsOfEntity = GetAllComponents(aEntityID);

			for (const std::type_index& componentTypeIndex : allComponentsOfEntity)
			{
				RemoveComponent(aEntityID, componentTypeIndex);
			}
		}

		template<typename ComponentType>
		inline ComponentType& GetComponent(const Entity aEntityID)
		{
			const std::type_index typeIndex = typeid(ComponentType);

			std::shared_lock<std::shared_mutex> lock(myComponentSharedMutex);
			ComponentWrapper<ComponentType>* wrapper = static_cast<ComponentWrapper<ComponentType>*>(myComponentsInEntity[aEntityID][typeIndex]);
			return wrapper->myComponent;
		}

		template <typename... ComponentTypes>
		inline const std::vector<Entity> GetEntitiesWithComponents()
		{
			return myArchetypeManager.GetEntitiesWithComponents<ComponentTypes...>();
		}

		template <typename ComponentType>
		inline const std::vector<Entity>& GetAllEntitiesWithThisComponent()
		{
			const std::type_index searchID = typeid(ComponentType);

			std::shared_lock<std::shared_mutex> lock(myComponentSharedMutex);
			const auto& list = myArchetypeManager.GetEntitiesInAllArchetype();

			std::vector<Entity> entities;

			for (const auto& archetype : list)
			{
				if (archetype.second.size() > 0)
				{
					const Entity entity = archetype.second[0];
					const auto& componentsIndex = GetAllComponents(entity);

					for (const auto& componentIndex : componentsIndex)
					{
						if (searchID == componentIndex)
						{
							entities.insert(entities.end(), archetype.second.begin(), archetype.second.end());
						}
					}
				}
			}

			return entities;
		}

		const std::vector<std::type_index> GetAllComponents(const Entity aEntityID) const
		{
			Archetype components;

			std::shared_lock<std::shared_mutex> lock(myComponentSharedMutex);
			const auto& component = myComponentsInEntity.find(aEntityID);

			if (component != myComponentsInEntity.end())
			{
				const auto& innerMap = component->second;

				for (const auto& componentPair : innerMap)
				{
					components.push_back(componentPair.first);
				}
			}

			return components;
		}
	private:
		mutable std::mutex myComponentMutex;
		mutable std::mutex myRemoveComponentMutex;
		mutable std::shared_mutex myComponentSharedMutex;
		ArchetypeManager myArchetypeManager;
		std::unordered_map<Entity, std::unordered_map<std::type_index, BaseComponentWrapper*>> myComponentsInEntity;
	};

	class EntityManager
	{
	public:
		EntityManager()
		{
			myEntities.reserve(ENTITIES_RESERVE);
		}

		~EntityManager()
		{
			myComponentManager.OnDestroy();
		}

		const Entity CreateEntity()
		{
			Entity id;

			std::lock_guard<std::mutex> lock(myEntityMutex);
			if (!myEntitiesInTrashcan.empty())
			{
				id = myEntitiesInTrashcan.top();
				myEntitiesInTrashcan.pop();
			}
			else
			{
				id = myNextEntityID++;
			}

			myEntities.push_back(id);
			return id;
		}

		void RemoveEntity(const Entity aEntityID)
		{
			std::lock_guard<std::mutex> lock(myEntityMutex);
			auto it = std::find(myEntities.begin(), myEntities.end(), aEntityID);
			if (it != myEntities.end())
			{
				myComponentManager.RemoveAllComponents(*it);
				myEntitiesInTrashcan.push(aEntityID);
				myEntities.erase(it);
			}
		}

		template<typename ComponentType>
		inline void AddComponent(const Entity aEntityID, const ComponentType& aComponent)
		{
			myComponentManager.AddComponent(aEntityID, aComponent);
		}

		template<typename ComponentType>
		inline void RemoveComponent(const Entity aEntityID, const ComponentType& aComponent)
		{
			myComponentManager.RemoveComponent(aEntityID, aComponent);
		}

		void RemoveComponent(const Entity aEntityID, const std::type_index& aComponentIndex)
		{
			myComponentManager.RemoveComponent(aEntityID, aComponentIndex);
		}

		const std::vector<Entity>& GetEntities() const
		{
			return myEntities;
		}

		template <typename... ComponentTypes>
		inline const std::vector<Entity> GetEntitiesWithComponents()
		{
			return myComponentManager.GetEntitiesWithComponents<ComponentTypes...>();
		}

		template <typename ComponentType>
		inline const std::vector<Entity> GetAllEntitiesWithThisComponent()
		{
			return myComponentManager.GetAllEntitiesWithThisComponent<ComponentType>();
		}

		template<typename ComponentType>
		inline ComponentType& GetComponent(const Entity aEntityID)
		{
			return myComponentManager.GetComponent<ComponentType>(aEntityID);
		}

		const std::vector<std::type_index> GetAllComponents(const Entity aEntityID) const
		{
			return myComponentManager.GetAllComponents(aEntityID);
		}
	private:
		mutable std::mutex myEntityMutex;
		Entity myNextEntityID = 1;
		ComponentManager myComponentManager;
		std::vector<Entity> myEntities;
		std::stack<Entity> myEntitiesInTrashcan;
	};
}

namespace ecs
{
	extern std::unique_ptr<EntityManager> globalECS;
}
