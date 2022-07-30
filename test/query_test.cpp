#include "doctest.h"

#include <archecs/queries.hpp>

using namespace arch;
using namespace arch::det;

namespace test_space
{
	struct test_struct{};
	class test_class{};
}

	struct t1{};
	struct t2{};
	struct t3{};
	struct t4{};
	TEST_CASE("Has Components")
	{
		{
			auto all_types = ids_of<t1, t2, t3>();
			auto all_type_span = std::span(all_types);
			auto q1 = with<t1>;
			CHECK(q1.filter(all_type_span));
			static_assert(std::is_same_v<typename decltype(q1)::resulting_components, det::type_list<t1>>);
		}
		{
			auto all_types = ids_of<t1, t2, t3>();
			auto all_type_span = std::span(all_types);
			auto q1 = with<t2>;
			CHECK(q1.filter(all_type_span));
			static_assert(std::is_same_v<typename decltype(q1)::resulting_components, det::type_list<t2>>);
		}
		{
			auto all_types = ids_of<t1, t2, t3>();
			auto all_type_span = std::span(all_types);
			auto q1 = with<t3>;
			CHECK(q1.filter(all_type_span));
			static_assert(std::is_same_v<typename decltype(q1)::resulting_components, det::type_list<t3>>);
		}
		{
			auto all_types = ids_of<t1, t2, t3>();
			auto all_type_span = std::span(all_types);
			auto q1 = with<t1, t2>;
			CHECK(q1.filter(all_type_span));
			static_assert(std::is_same_v<typename decltype(q1)::resulting_components, det::type_list<t1, t2>>);
		}
		{
			auto all_types = ids_of<t1>();
			auto all_type_span = std::span(all_types);
			auto q1 = with<t1, t2>;
			CHECK_FALSE(q1.filter(all_type_span));
		}
		{
			auto all_types = ids_of<t1, t3>();
			auto all_type_span = std::span(all_types);
			auto q1 = with<t1, t2>;
			CHECK_FALSE(q1.filter(all_type_span));
		}
	}
	TEST_CASE("Exact Components")
	{
		{
			auto all_types = ids_of<t1, t2, t3>();
			auto all_type_span = std::span(all_types);
			auto q1 = with_exactly<t1>;
			CHECK_FALSE(q1.filter(all_type_span));
			static_assert(std::is_same_v<typename decltype(q1)::resulting_components, det::type_list<t1>>);
		}
		{
			auto all_types = ids_of<t1, t2, t3>();
			auto all_type_span = std::span(all_types);
			auto q1 = with<t1, t2, t3>;
			CHECK(q1.filter(all_type_span));
			static_assert(std::is_same_v<typename decltype(q1)::resulting_components, det::type_list<t1, t2, t3>>);
		}
		{
			auto all_types = ids_of<t1>();
			auto all_type_span = std::span(all_types);
			auto q1 = with<t1>;
			CHECK(q1.filter(all_type_span));
		}
	}
	TEST_CASE("AND Query")
	{
		{
			auto all_types = ids_of<t1, t2, t3>();
			auto all_type_span = std::span(all_types);
			auto q1 = with<t1> and with<t2>;
			static_assert(std::is_same_v<typename decltype(q1)::resulting_components, det::type_list<t1, t2>>);
			CHECK(q1.filter(all_type_span));
			auto q2 = with<t1> and with<t4>;
			static_assert(std::is_same_v<decltype(q2)::resulting_components, det::type_list<t1, t4>>);
			CHECK_FALSE(q2.filter(all_type_span));
		}
	}
	TEST_CASE("NOT Query")
	{
		{
			auto all_types = ids_of<t1, t2, t3>();
			auto all_type_span = std::span(all_types);
			auto q1 = not with<t1>;
			CHECK_FALSE(q1.filter(all_type_span));
		}
		{
			auto all_types = ids_of<t1, t2, t3>();
			auto all_type_span = std::span(all_types);
			auto q1 = not with<t4>;
			CHECK(q1.filter(all_type_span));
		}
	}

namespace
{
	struct very_long_name_is_very_long{};
	struct s{};
	
	TEST_CASE("nameof Type")
	{
#ifdef _MSC_VER
		CHECK_EQ("`anonymous-namespace'::t1", det::nameof<t1>());
		CHECK_EQ("`anonymous-namespace'::s", det::nameof<s>());
		CHECK_EQ("`anonymous-namespace'::very_long_name_is_very_long", det::nameof<very_long_name_is_very_long>());
		CHECK_EQ("test_space::test_class", det::nameof<test_space::test_class>());
		CHECK_EQ("test_space::test_struct", det::nameof<test_space::test_struct>());
#else
		CHECK_EQ("t1", name_of<t1>());
		CHECK_EQ("{anonymous}::s", name_of<s>());
		CHECK_EQ("{anonymous}::very_long_name_is_very_long", name_of<very_long_name_is_very_long>());
		CHECK_EQ("test_space::test_class", name_of<test_space::test_class>());
		CHECK_EQ("test_space::test_struct", name_of<test_space::test_struct>());
#endif
	}
}