#include "doctest.h"

#include <archecs/internal/scheduler.hpp>
#include <archecs/system.hpp>
#include <archecs/update_group.hpp>

namespace
{
	struct t1
	{
		int id;
	};
	
	using arch::det::scheduler;
	
	TEST_CASE("scheduler schedule 1")
	{
		scheduler<t1> my_scheduler{};
		t1 first{1};
		t1 second{2};
		std::array second_dep{&first};
		t1 third{3};
		std::array third_dep{&first, &second};
		t1 fourth{4};
		std::array fourth_dep{&first, &second, &third};
		
		my_scheduler.add_job(first, std::span<t1 *>(), {});
		my_scheduler.add_job(second, second_dep, {});
		my_scheduler.add_job(third, third_dep, {});
		my_scheduler.add_job(fourth, fourth_dep, {});
		
		auto result = my_scheduler.schedule_jobs();
		CHECK_EQ(result.size(), 4);
		CHECK_EQ(result[0]->id, 1);
		CHECK_EQ(result[1]->id, 2);
		CHECK_EQ(result[2]->id, 3);
		CHECK_EQ(result[3]->id, 4);
	}
	
	TEST_CASE("scheduler schedule 2")
	{
		scheduler<t1> my_scheduler{};
		t1 first{1};
		t1 second{2};
		std::array second_dep{&first};
		t1 third{3};
		std::array third_dep{&first};
		t1 fourth{4};
		std::array fourth_dep{&second, &third};
		
		my_scheduler.add_job(first, std::span<t1 *>(), {});
		my_scheduler.add_job(second, second_dep, {});
		my_scheduler.add_job(third, third_dep, {});
		my_scheduler.add_job(fourth, fourth_dep, {});
		
		auto result = my_scheduler.schedule_jobs();
		CHECK_EQ(result.size(), 4);
		CHECK_EQ(result[0]->id, 1);
		CHECK_EQ(result[1]->id, 2);
		CHECK_EQ(result[2]->id, 3);
		CHECK_EQ(result[3]->id, 4);
	}
}