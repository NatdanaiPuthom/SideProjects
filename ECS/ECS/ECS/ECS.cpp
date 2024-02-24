#include "ECS.h"

namespace ecs
{
	std::unique_ptr<EntityManager> globalECS = std::make_unique<EntityManager>(); //Remeber to call globalECS.reset() before end of memory tracker to avoid false memory leaks!
}

