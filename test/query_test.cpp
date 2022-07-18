#include "doctest.h"

#include <archecs/queries.hpp>

using namespace arch;
using namespace arch::det;

namespace test_space
{
	struct test_struct{};
	class test_class{};
}

	struct T1{};
	struct T2{};
	struct T3{};
	struct T4{};
	//TEST_CASE("Has Components")
	//{
	//	{
	//		auto all_types = to_type_ids<T1, T2, T3>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = has_components<T1>();
	//		CHECK(q1.filter(all_type_span));
	//		static_assert(std::is_same_v<query_type_t<decltype(q1)>, det::type_list<T1&>>);
	//	}
	//	{
	//		auto all_types = to_type_ids<T1, T2, T3>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = has_components<T2>();
	//		CHECK(q1.filter(all_type_span));
	//		static_assert(std::is_same_v<query_type_t<decltype(q1)>, det::type_list<T2&>>);
	//	}
	//	{
	//		auto all_types = to_type_ids<T1, T2, T3>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = has_components<T3>();
	//		CHECK(q1.filter(all_type_span));
	//		static_assert(std::is_same_v<query_type_t<decltype(q1)>, det::type_list<T3&>>);
	//	}
	//	{
	//		auto all_types = to_type_ids<T1, T2, T3>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = has_components<T1, T2>();
	//		CHECK(q1.filter(all_type_span));
	//		static_assert(std::is_same_v<query_type_t<decltype(q1)>, det::type_list<T1&, T2&>>);
	//	}
	//	{
	//		auto all_types = to_type_ids<T1>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = has_components<T1, T2>();
	//		CHECK_FALSE(q1.filter(all_type_span));
	//	}
	//	{
	//		auto all_types = to_type_ids<T1, T3>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = has_components<T1, T2>();
	//		CHECK_FALSE(q1.filter(all_type_span));
	//	}
	//}
	//TEST_CASE("Exact Components")
	//{
	//	{
	//		auto all_types = to_type_ids<T1, T2, T3>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = exactly_components<T1>();
	//		CHECK_FALSE(q1.filter(all_type_span));
	//		static_assert(std::is_same_v<query_type_t<decltype(q1)>, det::type_list<T1&>>);
	//	}
	//	{
	//		auto all_types = to_type_ids<T1, T2, T3>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = has_components<T1, T2, T3>();
	//		CHECK(q1.filter(all_type_span));
	//		static_assert(std::is_same_v<query_type_t<decltype(q1)>, det::type_list<T1&, T2&, T3&>>);
	//	}
	//	{
	//		auto all_types = to_type_ids<T1>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = has_components<T1>();
	//		CHECK(q1.filter(all_type_span));
	//	}
	//}
	//TEST_CASE("AND Query")
	//{
	//	{
	//		auto all_types = to_type_ids<T1, T2, T3>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = has_components<T1>() and has_components<T2>();
	//		static_assert(std::is_same_v<query_type_t<decltype(q1)>, det::type_list<T1&, T2&>>);
	//		CHECK(q1.filter(all_type_span));
	//		auto q2 = has_components<T1>() and has_components<T4>();
	//		static_assert(std::is_same_v<query_type_t<decltype(q2)>, det::type_list<T1&, T4&>>);
	//		CHECK_FALSE(q2.filter(all_type_span));
	//	}
	//}
	//TEST_CASE("OR Query")
	//{
	//	{
	//		auto all_types = to_type_ids<T1, T2, T3>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = has_components<T1>() or has_components<T4>();
	//		CHECK(q1.filter(all_type_span));
	//		auto q2 = has_components<T4>() or has_components<T3>();
	//		CHECK(q2.filter(all_type_span));
	//	}
	//}
	//TEST_CASE("NOT Query")
	//{
	//	{
	//		auto all_types = to_type_ids<T1, T2, T3>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = not has_components<T1>();
	//		CHECK_FALSE(q1.filter(all_type_span));
	//	}
	//	{
	//		auto all_types = to_type_ids<T1, T2, T3>();
	//		auto all_type_span = std::span(all_types);
	//		auto q1 = !has_components<T4>();
	//		CHECK(q1.filter(all_type_span));
	//	}
	//}

namespace
{
	struct very_long_name_is_very_long{};
	struct s{};
	
	TEST_CASE("Nameof Type")
	{
#ifdef _MSC_VER
		CHECK_EQ("`anonymous-namespace'::T1", det::nameof<T1>());
		CHECK_EQ("`anonymous-namespace'::s", det::nameof<s>());
		CHECK_EQ("`anonymous-namespace'::very_long_name_is_very_long", det::nameof<very_long_name_is_very_long>());
		CHECK_EQ("test_space::test_class", det::nameof<test_space::test_class>());
		CHECK_EQ("test_space::test_struct", det::nameof<test_space::test_struct>());
#else
		CHECK_EQ("T1", name_of<T1>());
		CHECK_EQ("{anonymous}::s", name_of<s>());
		CHECK_EQ("{anonymous}::very_long_name_is_very_long", name_of<very_long_name_is_very_long>());
		CHECK_EQ("test_space::test_class", name_of<test_space::test_class>());
		CHECK_EQ("test_space::test_struct", name_of<test_space::test_struct>());
#endif
	}
}