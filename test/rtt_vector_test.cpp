#include "doctest.h"

#include <archecs/internal/rtt_vector.hpp>

namespace
{
	using arch::det::rtt_vector;
	
	struct t1
	{
		std::uint64_t value = 2;
	};
	
	TEST_CASE("rtt_vector push back")
	{
		std::pmr::monotonic_buffer_resource resource{128};
		
		rtt_vector vector = rtt_vector::of<t1>(resource);
		CHECK_EQ(vector.size(), 0);
		
		vector.push_back();
		CHECK_EQ(vector.size(), 1);
		CHECK_GE(vector.capacity(), 1);
		{
			auto &first_element = *reinterpret_cast<t1 *>(vector[0]);
			first_element.value = 512;
			CHECK_EQ(first_element.value, 512);
		}
		
		vector.push_back();
		CHECK_EQ(vector.size(), 2);
		CHECK_GE(vector.capacity(), 2);
		CHECK_EQ(reinterpret_cast<t1*>(vector[0])->value, 512);
		
		vector.push_back();
		CHECK_EQ(vector.size(), 3);
		CHECK_GE(vector.capacity(), 3);
		CHECK_EQ(reinterpret_cast<t1*>(vector[0])->value, 512);
	}
	
	TEST_CASE("rtt_vector empty copy")
	{
		std::pmr::monotonic_buffer_resource resource{128};
		
		rtt_vector vector = rtt_vector::of<t1>(resource);
		rtt_vector other = vector;
	}
	
	TEST_CASE("rtt_vector copy")
	{
		std::pmr::monotonic_buffer_resource resource{128};
		
		rtt_vector vector = rtt_vector::of<t1>(resource);
		vector.push_back();
		
		rtt_vector other = vector;
		CHECK_EQ(vector.size(), 1);
		CHECK_EQ(other.size(), 1);
	}
	
	TEST_CASE("rtt_vector empty move")
	{
		std::pmr::monotonic_buffer_resource resource{128};
		
		rtt_vector vector = rtt_vector::of<t1>(resource);
		rtt_vector other = std::move(vector);
	}
	
	TEST_CASE("rtt_vector move")
	{
		std::pmr::monotonic_buffer_resource resource{128};
		
		rtt_vector vector = rtt_vector::of<t1>(resource);
		vector.push_back();
		rtt_vector other = std::move(vector);
		CHECK_EQ(other.size(), 1);
	}
}