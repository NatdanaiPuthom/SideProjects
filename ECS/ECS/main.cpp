#pragma once
#define _CRTDBG_MAP_ALLOC

#include <iostream>
#include <thread>
#include <cstring>

#include "ECS/ECS.h"

struct Position
{
	float x;
	float y;
	float z;
};

struct Mesh
{
	char name[260];
};

struct Collider
{
	float size;
};

struct EnemyComponent
{
	int hp;
	std::vector<std::string> hello;
};

struct Test
{
	std::string hello;
};

class EnemySystem
{
public:
	void Update(const float aDeltaTime)
	{
		const auto& entities = ecs::globalECS->GetEntitiesWithComponents<Position, Mesh, Collider, EnemyComponent>();

		const ecs::Entity chunkSize = static_cast<ecs::Entity>(entities.size() / numThreads);

		for (size_t i = 0; i < numThreads; ++i)
		{
			const size_t index = i;

			threads.emplace_back([&]
				{
					ecs::Entity startIdx = static_cast<ecs::Entity>(index * chunkSize);
					ecs::Entity endIdx = static_cast<size_t>((index + 1)) == numThreads ? static_cast<ecs::Entity>(entities.size()) : static_cast<ecs::Entity>((index + 1) * chunkSize);

					ProcessEntities(aDeltaTime, entities.begin() + startIdx, entities.begin() + endIdx);
				});
		}

		for (auto& thread : threads)
		{
			if (thread.joinable())
				thread.join();
		}
	}
private:
	void ProcessEntities(const float aDeltaTime, std::vector<ecs::Entity>::const_iterator start, std::vector<ecs::Entity>::const_iterator end)
	{
		for (auto& it = start; it != end; ++it)
		{
			Position& position = ecs::globalECS->GetComponent<Position>(*it);
			position.x += 1.0f * aDeltaTime;
		}
	}
private:
	//const unsigned int numThreads = std::thread::hardware_concurrency();
	const unsigned int numThreads = 1;
	std::vector<std::thread> threads;
};


int main()
{
	{
		for (int i = 0; i < 1; ++i)
		{
			const ecs::Entity entity = ecs::globalECS->CreateEntity();
			ecs::globalECS->AddComponent(entity, Test{ "hello" });
			ecs::globalECS->AddComponent(entity, Collider{ 10 });
			ecs::globalECS->AddComponent(entity, EnemyComponent{});
		}

		EnemySystem enemySystem;

		for (int i = 0; i < 5; ++i)
		{
			enemySystem.Update(1.0f);
		}

		ecs::globalECS.reset();
	}

	_CrtDumpMemoryLeaks();

	return 0;
}