#include "doctest.h"

#include <archecs/archetype.hpp>

namespace
{
	struct t1{};
	struct t2{};
	struct t3{};
	
	using arch::archetype;
	using arch::id_of;
	
	TEST_CASE("archetype type sorting")
	{
		std::pmr::monotonic_buffer_resource resource{128};
		archetype test_arch{};
		{
			auto modifier = test_arch.internal().modify_archetype();
			modifier.init(resource);
			modifier.add_type<t1>(resource);
			modifier.add_type<t2>(resource);
			modifier.add_type<t3>(resource);
		}
		
		CHECK(test_arch.contains_type(id_of<t1>()));
		CHECK(test_arch.contains_type(id_of<t2>()));
		CHECK(test_arch.contains_type(id_of<t3>()));
	}
}