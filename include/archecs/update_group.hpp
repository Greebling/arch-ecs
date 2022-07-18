#pragma once


#include <span>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <type_traits>

#include "system.hpp"
#include "internal/scheduler.hpp"

namespace arch
{
	
	class world;
	
	class update_group : system_base
	{
	public:
		virtual ~update_group() noexcept
		{
			for (system_base *system: _contained_systems)
			{
				delete system;
			}
		}
		
		std::string_view get_group_name() const
		{
			return system_name;
		}
		
		std::span<const system_base *const> get_contained_systems() const
		{
			return {_contained_systems};
		}
		
		void update_before(const system_base &system)
		{
			_following_system_names.push_back(system.get_system_name());
		}
		
		void update_after(const system_base &system)
		{
			_previous_system_names.push_back(system.get_system_name());
		}
	
	public:
		virtual void execute(world &execution_world) final
		{
			on_before_execute();
			
			for (system_base *system: _contained_systems)
			{
				system->execute(execution_world);
			}
			
			on_after_execute();
		}
		
		virtual void on_before_execute()
		{}
		
		virtual void on_after_execute()
		{}
		
		virtual std::span<const std::string_view> execute_before() const
		{
			return _previous_system_names;
		}
		
		virtual std::span<const std::string_view> execute_after() const
		{
			return _following_system_names;
		}
	
	public:
		
		struct group_modifier
		{
		public:
			
			[[nodiscard]]
			static group_modifier create(update_group &group_to_modify)
			{
				return group_modifier(group_to_modify);
			}
			
			~group_modifier()
			{
				_edited_group.on_systems_changed();
			}
			
			template<typename t_system, typename ...t_args>
			const group_modifier &&add_system(t_args &&... args) const &&
			{
				static_assert(std::is_base_of_v<system_base, t_system>, "Type needs to inherit from arch::system_base");
				static_assert(std::is_constructible_v<t_system, t_args...>, "Type is not constructible with the given arguments");
				
				t_system *created = new t_system(arch_fwd(args)...);
				arch_assert_external(not created->get_system_name().empty());
				created->owning_group = &_edited_group;
				_edited_group._contained_systems.push_back(static_cast<system_base *>(created));
				
				//_system_scheduler.add_job(*created, )
				return std::move(*this);
			}
			
			void finish() const &&
			{
				// empty function as the destructor does the calculation of dependencies
			}
		
		private:
			group_modifier(update_group &group_to_modify) : _edited_group(group_to_modify)
			{
			}
			
			group_modifier() = delete;
			
			group_modifier(const group_modifier &) = delete;
			
			group_modifier(group_modifier &&) = delete;
			
			group_modifier(const group_modifier &&) = delete;
		
		private:
			update_group &_edited_group;
		};
		
		group_modifier modify()
		{
			return group_modifier::create(*this);
		}
	
	protected:
		std::span<system_base *> get_contained_systems_mutable()
		{
			return {_contained_systems};
		}
		
	private:
		void on_systems_changed()
		{
			reorder_systems();
		}
		
		void reorder_systems()
		{
			_system_scheduler.clear();
			
			std::unordered_map<std::size_t, system_base *> name_hash_to_system{};
			auto hash = std::hash<std::string_view>();
			for (system_base *system: _contained_systems)
			{
				std::size_t name_hash = hash(system->get_system_name());
				name_hash_to_system[name_hash] = system;
			}
			
			std::vector<system_base *> previous_systems{};
			previous_systems.reserve(16);
			std::vector<system_base *> following_systems{};
			following_systems.reserve(16);
			
			for (system_base *system: _contained_systems)
			{
				previous_systems.clear();
				for (std::string_view dependency_name: system->execute_after())
				{
					std::size_t hashed_dependency_name = hash(dependency_name);
					if (name_hash_to_system.contains(hashed_dependency_name))
					{
						system_base *dependency = name_hash_to_system.at(hashed_dependency_name);
						previous_systems.push_back(dependency);
					}
				}
				
				following_systems.clear();
				for (std::string_view dependency_name: system->execute_before())
				{
					std::size_t hashed_dependency_name = hash(dependency_name);
					if (name_hash_to_system.contains(hashed_dependency_name))
					{
						system_base *dependency = name_hash_to_system.at(hashed_dependency_name);
						following_systems.push_back(dependency);
					}
				}
				
				_system_scheduler.add_job(*system, {previous_systems}, {following_systems});
			}
			
			_contained_systems = _system_scheduler.schedule_jobs();
		}
	
	private:
		std::vector<system_base *> _contained_systems{};
		std::vector<std::string_view> _previous_system_names{};
		std::vector<std::string_view> _following_system_names{};
		
		det::scheduler<system_base> _system_scheduler{};
		
		friend class world;
	};
}