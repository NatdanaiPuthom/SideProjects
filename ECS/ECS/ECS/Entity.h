#pragma once
#include <vector>
#include <typeindex>

namespace ecs
{
	constexpr size_t ENTITIES_RESERVE = 10000;

	using Entity = size_t;
	using Signature = size_t;
	using ArchetypeID = size_t;
	using Archetype = std::vector<std::type_index>;
}