#pragma once

#include <string_view>
#include <span>

namespace arch
{
	class update_group;
	
	class world;
	
	class system_base
	{
	public:
		virtual ~system_base() = default;
		
		virtual void execute(world &) = 0;
		
		virtual std::span<const std::string_view> execute_before() const
		{
			return {};
		}
		
		virtual std::span<const std::string_view> execute_after() const
		{
			return {};
		}
		
		std::string_view get_system_name() const
		{
			return system_name;
		}
	
	protected:
		update_group *get_group()
		{
			return owning_group;
		}
		
		const update_group *get_group() const
		{
			return owning_group;
		}
	
	protected:
		std::string_view system_name{};
	
	private:
		update_group *owning_group{};
		
		friend class update_group;
	};
}