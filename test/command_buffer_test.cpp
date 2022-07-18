#include "doctest.h"

#include <archecs/world.hpp>
#include <archecs/command_buffer.hpp>
#include <archecs/queries.hpp>

namespace command_buffer_test
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
	};
	
	using arch::world;
	using arch::entity;
	using arch::virtual_entity;
	using arch::entity_command_buffer;
	
	TEST_CASE("command buffer add component")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		
		{
			entity_command_buffer ecb{test_world};
			ecb.add_component(created1, t1{});
			ecb.add_component(created1, t2{256});
			ecb.run();
		}
		CHECK_EQ(test_world.get_component<t1>(created1).data, t1().data);
		CHECK_EQ(test_world.get_component<t2>(created1).data, 256);
		
		{
			entity_command_buffer ecb{test_world};
			ecb.add_component(created1, t3{});
			ecb.run();
		}
		CHECK_EQ(test_world.get_component<t1>(created1).data, t1().data);
		CHECK_EQ(test_world.get_component<t2>(created1).data, 256);
		CHECK_EQ(test_world.get_component<t3>(created1).data, t3().data);
	}
	
	TEST_CASE("command buffer remove component")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		test_world.add_components(created1, t1{}, t2{256});
		
		{
			entity_command_buffer ecb{test_world};
			ecb.remove_component<t1>(created1);
			ecb.run();
		}
		CHECK(not test_world.get_archetype_of(created1).contains_type(arch::id_of<t1>()));
		CHECK_EQ(test_world.get_component<t2>(created1).data, 256);
		
		{
			entity_command_buffer ecb{test_world};
			ecb.remove_component<t2>(created1);
			ecb.run();
		}
		CHECK(not test_world.get_archetype_of(created1).contains_type(arch::id_of<t2>()));
	}
	
	TEST_CASE("command buffer set component")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		test_world.add_components(created1, t1{}, t2{256});
		
		{
			entity_command_buffer ecb{test_world};
			ecb.set_component(created1, t1{128});
			ecb.run();
		}
		CHECK_EQ(test_world.get_component<t1>(created1).data, 128);
		CHECK_EQ(test_world.get_component<t2>(created1).data, 256);
		
		{
			entity_command_buffer ecb{test_world};
			ecb.set_component(created1, t2{0});
			ecb.run();
		}
		CHECK_EQ(test_world.get_component<t1>(created1).data, 128);
		CHECK_EQ(test_world.get_component<t2>(created1).data, 0);
	}
	
	TEST_CASE("command buffer optimized add")
	{
		world test_world{};
		entity created1 = test_world.create_entity();
		entity created2 = test_world.create_entity();
		entity created3 = test_world.create_entity();
		test_world.add_components(created1, t1{});
		
		{
			entity_command_buffer ecb{test_world};
			ecb.add_component(created2, t1{128});
			ecb.add_component(created1, t2{256});
			ecb.add_component(created3, t3{512});
			ecb.remove_component<t1>(created1);
			ecb.add_component(created1, t3{1024});
			ecb.run();
		}
		
		CHECK_EQ(test_world.get_component<t2>(created1).data, 256);
		CHECK_EQ(test_world.get_component<t3>(created1).data, 1024);
		CHECK_EQ(test_world.get_component<t1>(created2).data, 128);
		CHECK_EQ(test_world.get_component<t3>(created3).data, 512);
		CHECK_FALSE(test_world.get_archetype_of(created3).contains_type(arch::id_of<t1>()));
	}
	
	TEST_CASE("command buffer create entity")
	{
		world test_world{};
		
		{
			entity_command_buffer ecb{test_world};
			
			virtual_entity created1 = ecb.create_entity();
			virtual_entity created2 = ecb.create_entity();
			virtual_entity created3 = ecb.create_entity();
			
			ecb.add_component(created2, t1{128});
			ecb.add_component(created1, t2{256});
			ecb.add_component(created3, t3{512});
			ecb.add_component(created1, t3{1024});
			ecb.run();
		}
		
		std::size_t t1_count = 0;
		std::size_t t2_count = 0;
		std::size_t t3_count = 0;
		
		test_world.for_all(arch::with<t1>, [&](entity, auto)
		{
			++t1_count;
		});
		test_world.for_all(arch::with<t2>, [&](entity, auto)
		{
			++t2_count;
		});
		test_world.for_all(arch::with<t3>, [&](entity, auto)
		{
			++t3_count;
		});
		
		CHECK_EQ(t1_count, 1);
		CHECK_EQ(t2_count, 1);
		CHECK_EQ(t3_count, 2);
	}
}