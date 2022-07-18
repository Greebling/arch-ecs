#include "doctest.h"

#include <archecs/system.hpp>
#include <archecs/update_group.hpp>

namespace
{
	class sys1 : public arch::system_base
	{
	public:
		sys1()
		{
			system_name = "sys1";
		}
		
		std::span<const std::string_view> execute_after() const override
		{
			return after_groups;
		}
		
		std::span<const std::string_view> execute_before() const override
		{
			return before_groups;
		}
		
		std::vector<std::string_view> after_groups{};
		std::vector<std::string_view> before_groups{{"sys3"}, {"sys4"}, {"sys1"}};
		
		void execute(arch::world &) override
		{}
	};
	
	class sys2 : public arch::system_base
	{
	public:
		sys2()
		{
			system_name = "sys2";
		}
		
		std::span<const std::string_view> execute_after() const override
		{
			return after_groups;
		}
		
		std::span<const std::string_view> execute_before() const override
		{
			return before_groups;
		}
		
		std::vector<std::string_view> after_groups{{"sys1"}};
		std::vector<std::string_view> before_groups{{"sys3"}, {"sys4"}};
		
		void execute(arch::world &) override
		{}
	};
	
	class sys3 : public arch::system_base
	{
	public:
		sys3()
		{
			system_name = "sys3";
		}
		
		std::span<const std::string_view> execute_after() const override
		{
			return after_groups;
		}
		
		std::span<const std::string_view> execute_before() const override
		{
			return before_groups;
		}
		
		std::vector<std::string_view> after_groups{{"sys1"}, {"sys2"}};
		std::vector<std::string_view> before_groups{{"sys4"}};
		
		void execute(arch::world &) override
		{}
	};
	
	class sys4 : public arch::system_base
	{
	public:
		sys4()
		{
			system_name = "sys4";
		}
		
		std::span<const std::string_view> execute_after() const override
		{
			return after_groups;
		}
		
		std::span<const std::string_view> execute_before() const override
		{
			return before_groups;
		}
		
		std::vector<std::string_view> after_groups{{"sys1"}, {"sys2"},  {"sys3"}};
		std::vector<std::string_view> before_groups{};
		
		void execute(arch::world &) override
		{}
	};
	
	TEST_CASE("update group add systems")
	{
		using namespace std::literals::string_view_literals;
		
		arch::update_group my_group{};
		my_group.modify().add_system<sys4>().add_system<sys2>().add_system<sys3>().add_system<sys1>();
		auto group_systems = my_group.get_contained_systems();
		CHECK_EQ(group_systems.size(), 4);
		CHECK_EQ(group_systems[0]->get_system_name(), "sys1"sv);
		CHECK_EQ(group_systems[1]->get_system_name(), "sys2"sv);
		CHECK_EQ(group_systems[2]->get_system_name(), "sys3"sv);
		CHECK_EQ(group_systems[3]->get_system_name(), "sys4"sv);
	}
}