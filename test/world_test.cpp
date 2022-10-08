#include "doctest.h"

#include <archecs/world.hpp>
#include <archecs/queries.hpp>

namespace world_test
{
	struct t1
	{
		int data = 2;
	};
	struct t2
	{
		int data = 128;
	};
	struct t3
	{
		int data = 16;
	};
	struct t4
	{
		float data = 2.0f;
	};
	
	using arch::world;
	using arch::entity;
	using arch::with;
	
	TEST_CASE("world create entity")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		CHECK(test_world.is_alive(created1));
	}
	
	TEST_CASE("world destroy entity")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		test_world.destroy_entity(created1);
		
		CHECK(not test_world.is_alive(created1));
	}
	
	TEST_CASE("world add components")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		
		{
			test_world.add_components(created1, t1{}, t2{256});
			CHECK_EQ(test_world.get_component<t1>(created1).data, t1().data);
			t2 &component2 = test_world.get_component<t2>(created1);
			CHECK_EQ(component2.data, 256);
			component2.data = 512;
			CHECK_EQ(component2.data, 512);
		}
		
		{
			test_world.add_components(created1, t3{});
			t1 &component1 = test_world.get_component<t1>(created1);
			t2 &component2 = test_world.get_component<t2>(created1);
			t3 &component3 = test_world.get_component<t3>(created1);
			CHECK_EQ(component1.data, t1().data);
			CHECK_EQ(component2.data, 512);
			CHECK_EQ(component3.data, t3().data);
		}
		
		{
			test_world.add_components(created1, t4{});
			t1 &component1 = test_world.get_component<t1>(created1);
			t2 &component2 = test_world.get_component<t2>(created1);
			t3 &component3 = test_world.get_component<t3>(created1);
			CHECK_EQ(component1.data, t1().data);
			CHECK_EQ(component2.data, 512);
			CHECK_EQ(component3.data, t3().data);
		}
	}
	
	TEST_CASE("world destroy entity with components")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		test_world.add_components(created1, t1{}, t2{256});
		arch::archetype &entities_archetype = test_world.get_archetype_of(created1);
		
		test_world.destroy_entity(created1);
		
		CHECK(not test_world.is_alive(created1));
		CHECK_EQ(entities_archetype.entities().size(), 0);
		auto archetypes_component_vectors = entities_archetype.internal().component_vectors();
		for (const auto &archetypes_component_vector: archetypes_component_vectors)
		{
			CHECK_EQ(archetypes_component_vector.size(), 0);
		}
	}
	
	TEST_CASE("world remove components")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		test_world.add_components(created1, t1{}, t2{256});
		
		test_world.remove_components<t2>(created1);
		auto &entity_archetype = test_world.get_archetype_of(created1);
		CHECK(entity_archetype.contains_type(arch::id_of<t1>()));
		CHECK(not entity_archetype.contains_type(arch::id_of<t2>()));
	}
	
	TEST_CASE("world modify component set")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		
		{
			std::array added_types{arch::info_of<t1>(), arch::info_of<t4>()};
			std::array added_types_destructors{arch::det::multi_destructor_of<t1>(), arch::det::multi_destructor_of<t4>()};
			test_world.modify_component_set(created1, added_types, added_types_destructors, {});
			auto &entity_archetype = test_world.get_archetype_of(created1);
			CHECK(entity_archetype.contains_type(arch::id_of<t1>()));
			CHECK(entity_archetype.contains_type(arch::id_of<t4>()));
			test_world.get_component<t4>(created1).data = 8.f;
		}
		
		{
			std::array added_types{arch::info_of<t2>(), arch::info_of<t3>()};
			std::array added_types_destructors{arch::det::multi_destructor_of<t2>(), arch::det::multi_destructor_of<t3>()};
			std::array removed_types{arch::id_of<t1>()};
			test_world.modify_component_set(created1, added_types, added_types_destructors, removed_types);
			auto &entity_archetype = test_world.get_archetype_of(created1);
			CHECK(not entity_archetype.contains_type(arch::id_of<t1>()));
			CHECK(entity_archetype.contains_type(arch::id_of<t2>()));
			CHECK(entity_archetype.contains_type(arch::id_of<t3>()));
			CHECK_EQ(test_world.get_component<t4>(created1).data, 8.f);
		}
	}
	
	TEST_CASE("world add components templated")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		test_world.add_components(created1, t1{3}, t2{4});
		entity created2 = test_world.create_entity();
		test_world.add_components(created2, t2{4}, t1{3});
		
		CHECK_EQ(test_world.get_component<t1>(created1).data, 3);
		CHECK_EQ(test_world.get_component<t1>(created2).data, 3);
		CHECK_EQ(test_world.get_component<t2>(created1).data, 4);
		CHECK_EQ(test_world.get_component<t2>(created2).data, 4);
	}
	
	TEST_CASE("world add components already added")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		test_world.add_components(created1, t1{3}, t2{4});
		test_world.add_components(created1, t1{999}, t2{888});
		
		entity created2 = test_world.create_entity();
		test_world.add_components(created2, t2{4}, t1{3});
		test_world.add_components(created2, t1{999}, t2{888});
		
		CHECK_EQ(test_world.get_component<t1>(created1).data, 999);
		CHECK_EQ(test_world.get_component<t1>(created2).data, 999);
		CHECK_EQ(test_world.get_component<t2>(created1).data, 888);
		CHECK_EQ(test_world.get_component<t2>(created2).data, 888);
	}
	
	TEST_CASE("world remove components templated")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		test_world.add_components(created1, t1{3}, t2{4}, t3{});
		entity created2 = test_world.create_entity();
		test_world.add_components(created2, t2{4}, t1{3}, t3{});
		
		test_world.remove_components<t3, t1>(created1);
		test_world.remove_components<t1, t2>(created2);
		
		CHECK(test_world.has_component<t2>(created1));
		CHECK(test_world.has_component<t3>(created2));
	}
	
	struct delete_detector
	{
		inline static std::uint64_t delete_count = 0;
		inline static std::uint64_t construct_count = 0;
		
		delete_detector()
		{
			construct_count++;
		}
		
		delete_detector(delete_detector&&)
		{
			construct_count++;
		}
		
		delete_detector(const delete_detector&)
		{
			construct_count++;
		}
		
		~delete_detector()
		{
			delete_count++;
		}
	};
	
	TEST_CASE("world component remove destructor call")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		
		delete_detector::delete_count = 0;
		delete_detector::construct_count = 0;
		test_world.add_components(created1, delete_detector{});
		CHECK_EQ(delete_detector::delete_count, delete_detector::construct_count - 1);
		
		test_world.remove_components<t3>(created1);
		CHECK_EQ(delete_detector::delete_count, delete_detector::construct_count);
	}
	
	TEST_CASE("world destroy entity component destructor call")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		
		delete_detector::delete_count = 0;
		delete_detector::construct_count = 0;
		test_world.add_components(created1, delete_detector{});
		CHECK_EQ(delete_detector::delete_count, delete_detector::construct_count - 1);
		
		test_world.destroy_entity(created1);
		CHECK_EQ(delete_detector::delete_count, delete_detector::construct_count);
	}
	
	TEST_CASE("world foreach data access")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		test_world.add_components(created1, t1{}, t2{256}, t3{512});
		entity created2 = test_world.create_entity();
		test_world.add_components(created2, t1{}, t2{256}, t3{512});
		
		test_world.for_all(with<const t1 &> and with<const t2 &> and with<const t3 &>, [](entity, const t1 &my_t1, const t2 &my_t2, const t3 &my_t3)
		{
			CHECK_EQ(my_t1.data, t1().data);
			CHECK_EQ(my_t2.data, 256);
			CHECK_EQ(my_t3.data, 512);
		});
	}
	
	TEST_CASE("world foreach data change")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		test_world.add_components(created1, t1{}, t2{});
		entity created2 = test_world.create_entity();
		test_world.add_components(created2, t1{}, t2{}, t3{});
		
		constexpr auto q1 = with<t1 &> and with<t2 &> and not with<t3>;
		test_world.for_all(q1, [](entity, t1 &my_t1, t2 &my_t2)
		{
			my_t1.data = 512;
			my_t2.data = 256;
		});
		test_world.for_all(q1, [](entity, t1 &my_t1, t2 &my_t2)
		{
			CHECK_EQ(my_t1.data, 512);
			CHECK_EQ(my_t2.data, 256);
		});
		test_world.for_all(with<t1 &> and with<t2 &> and with<t3 &>,
		                   [](entity, t1 &my_t1, t2 &my_t2, t3 &my_t3)
		                   {
			                   CHECK_EQ(my_t1.data, t1().data);
			                   CHECK_EQ(my_t2.data, t2().data);
			                   CHECK_EQ(my_t3.data, t3().data);
		                   });
	}
	
	TEST_CASE("world foreach with data access")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		test_world.add_components(created1, t1{}, t2{256}, t3{512});
		entity created2 = test_world.create_entity();
		test_world.add_components(created2, t1{}, t2{256}, t3{512});
		
		test_world.for_all_with([](entity, const t1 &my_t1, const t2 &my_t2, const t3 &my_t3)
		{
			CHECK_EQ(my_t1.data, t1().data);
			CHECK_EQ(my_t2.data, 256);
			CHECK_EQ(my_t3.data, 512);
		});
	}
}