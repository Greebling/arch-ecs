#include <vector>

#include <nanobench.h>
#include <archecs/arch_ecs.hpp>
#include <entt/entt.hpp>


struct t1
{
	float x,y,z;
};
struct t2
{
	float x,y,z;
};

int main()
{
	auto benchmark = ankerl::nanobench::Bench().minEpochIterations(10000).warmup(5).relative(true);
	constexpr std::size_t iterations = 1280;
	constexpr std::size_t foreach_calls = 0;
	
	//{
	//	benchmark.run("vectors", [&]
	//	{
	//		auto vec1 = std::vector<t1>();
	//		auto vec2 = std::vector<t2>();
	//		for (int i = 0; i < iterations; ++i)
	//		{
	//			vec1.emplace_back(t1{i});
	//			vec2.emplace_back(t2{i});
	//		}
	//
	//		for (std::size_t i = 0; i < vec1.size(); ++i)
	//		{
	//			vec1[i].data = 1;
	//			vec2[i].data = 1;
	//		}
	//	});
	//}
	
	{
		using arch::with;
		benchmark.run("archecs", [&]
		{

            arch::world my_world{};
            for (int i = 0; i < iterations; ++i)
            {
                auto created = my_world.create_entity();
                float val = i;
                my_world.add_components(created, t1{val,val,val});
                my_world.add_components(created, t2{val,val,val});
            }
			for (std::size_t i = 0; i < foreach_calls; ++i)
			{
				my_world.for_all(with<t1 &> and with<t2 &>, [](arch::entity, t1 &first, t2 &second)
				{
					first.x = 1;
					first.y = 1;
					first.z = 1;
					second.x = 1;
					second.y = 1;
					second.z = 1;
				});
			}
		});
	}
	
	//{
	//	using arch::with;
	//	benchmark.run("archecs buffered", [&]
	//	{
	//		arch::world my_world{};
	//		arch::entity_command_buffer buffer{my_world};
	//		for (int i = 0; i < iterations; ++i)
	//		{
	//			float val = i;
	//			auto created = buffer.create_entity();
	//			buffer.add_component(created, t1{val, val, val});
	//			buffer.add_component(created, t2{val, val, val});
	//		}
	//		buffer.run();
	//
	//		for (std::size_t i = 0; i < foreach_calls; ++i)
	//		{
	//			my_world.for_all(with<t1 &> and with<t2 &>, [](arch::entity, t1 &first, t2 &second)
	//			{
	//				first.x = 1;
	//				first.y = 1;
	//				first.z = 1;
	//				second.x = 1;
	//				second.y = 1;
	//				second.z = 1;
	//			});
	//		}
	//	});
	//}
	//
	{
		benchmark.run("entt", [&]
		{
			entt::registry registry;
	
			for (int i = 0; i < iterations; ++i)
			{
				float val = i;
				const auto entity = registry.create();
				registry.emplace<t1>(entity, val, val, val);
				registry.emplace<t2>(entity, val, val, val);
			}
	
			auto view = registry.view<t1, t2>();
	
			for (std::size_t i = 0; i < foreach_calls; ++i)
			{
				view.each([](t1 &first, t2 &second)
				          {
					          first.x = 1;
					          first.y = 1;
					          first.z = 1;
					          second.x = 1;
					          second.y = 1;
					          second.z = 1;
				          });
			}
		});
	}
}