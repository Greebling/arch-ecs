#pragma once

#include <vector>
#include <span>
#include <limits>

#include "archetype.hpp"
#include "world.hpp"

namespace arch
{
	class entity_command_buffer
	{
	public:
		explicit entity_command_buffer(world &execution_world, std::size_t buffer_size = 128)
				: _execution_world(execution_world),
				  _memory_buffer(buffer_size)
		{
			// initialize commands after _memory_buffer
			_commands = std::pmr::vector<entity_command>(&_memory_buffer);
		}
	
	private:
		enum class entity_command_type
		{
			// WARNING: ordering of the values here is important, as everything before create_entity should be batchable in execution
			destroy_entity, // order first to optimize any other commands away
			create_entity,
			add_component,
			remove_component,
			set_component
		};
		
		struct add_command_data
		{
			template<typename t_component>
			static add_command_data of(void *component_data)
			{
				return {component_data, det::multi_destructor_of<t_component>()};
			}
			
			void *component_data;
			// we need to keep destructors in case we will have to create a new archetype
			det::multi_destructor destructor;
		};
		
		struct entity_command
		{
			entity_command(entity_command_type type, const entity &target, type_info component_type, void *command_args = nullptr)
					: type(type),
					  target(target),
					  component_type(component_type),
					  command_args(command_args)
			{}
			
			entity_command_type type;
			entity target;
			type_info component_type;
			void *command_args;
		};
		
		void execute_command(entity_command command)
		{
			switch (command.type)
			{
				case entity_command_type::destroy_entity:
					if (not is_target_virtual_entity(command))
					{
						_execution_world.destroy_entity(command.target);
					}
					break;
				case entity_command_type::add_component:
				{
					add_command_data *command_data = reinterpret_cast<add_command_data *>(command.command_args);
					_execution_world.add_component(command.target, command.component_type, command_data->component_data, command_data->destructor);
					break;
				}
				case entity_command_type::remove_component:
				{
					_execution_world.remove_component(command.target, command.component_type.id);
				}
					break;
				case entity_command_type::set_component:
				{
					void *component = _execution_world.get_component(command.target, command.component_type.id);
					std::memcpy(component, command.command_args, command.component_type.size);
					break;
				}
				default:
					arch_assert_external(false); // unknown or invalid command that should have been executed in other ways
					break;
			}
		}
		
		[[nodiscard]]
		static constexpr bool is_target_virtual_entity(const entity_command &command)
		{
			return command.target.version == std::numeric_limits<version_t>::max();
		}
	
	public:
		template<typename t_component>
		void add_component(entity target, t_component &&component)
		{
			void *component_memory = _memory_buffer.allocate(sizeof(t_component), alignof(t_component));
			new(component_memory) t_component(arch_fwd(component));
			
			void *add_data = _memory_buffer.allocate(sizeof(add_command_data), alignof(add_command_data));
			new(add_data) add_command_data(arch_fwd(add_command_data::of<t_component>(component_memory)));
			
			_commands.emplace_back(entity_command_type::add_component, target, info_of<t_component>(), add_data);
		}
		template<typename t_component>
		void add_component(virtual_entity target, t_component &&component)
		{
			add_component<t_component>(from_virtual(target), arch_fwd(component));
		}
		
		template<typename t_component>
		void remove_component(entity target)
		{
			_commands.emplace_back(entity_command_type::remove_component, target, info_of<t_component>());
		}
		
		template<typename t_component>
		void remove_component(virtual_entity target)
		{
			remove_component<t_component>(from_virtual(target));
		}
		
		template<typename t_component>
		void set_component(entity target, t_component &&component)
		{
			void *component_memory = _memory_buffer.allocate(sizeof(t_component), alignof(t_component));
			new(component_memory) t_component(arch_fwd(component));
			
			_commands.emplace_back(entity_command_type::set_component, target, info_of<t_component>(), component_memory);
		}
		
		template<typename t_component>
		void set_component(virtual_entity target, t_component &&component)
		{
			set_component<t_component>(from_virtual(target), arch_fwd(component));
		}
		
		/// creates a temporary entity that will be truly created when the command buffer is ran
		/// \return a temporary entity that will not be found in the world
		virtual_entity create_entity()
		{
			entity created_entity = {static_cast<entity_id_t>(n_created_entities), std::numeric_limits<version_t>::max()};
			++n_created_entities;
			
			_commands.emplace_back(entity_command_type::create_entity, created_entity, type_info::none(), nullptr);
			return virtual_entity(created_entity.id);
		}
		
		void destroy_entity(entity target)
		{
			_commands.emplace_back(entity_command_type::create_entity, target, type_info::none(), nullptr);
		}
		
		void destroy_entity(virtual_entity target)
		{
			_commands.emplace_back(entity_command_type::create_entity, from_virtual(target), type_info::none(), nullptr);
		}
		
		void run()
		{
			optimize_entity_modification_order();
			
			const std::size_t iteration_end = _commands.size();
			
			// buffers for batching commands together
			std::vector<type_info> added_types{};
			std::vector<det::multi_destructor> added_destructors{};
			std::vector<type_id> removed_types{};
			
			for (std::size_t command_index = 0; command_index < iteration_end; ++command_index)
			{
				entity current_entity = _commands[command_index].target;
				std::size_t same_entity_modifications_end = command_index;
				while (same_entity_modifications_end < iteration_end
				       && _commands[same_entity_modifications_end].target == current_entity)
				{
					++same_entity_modifications_end;
				}
				
				if (command_index + 1 == same_entity_modifications_end)
				{
					// only do one modification to entity, no batching needed
					execute_command(_commands[command_index]);
				}
				else
				{
					// execute commands in as few batches as possible
					
					if (_commands[command_index].type == entity_command_type::destroy_entity)
					{
						// ignore following commands, entity will be destroyed anyways
						execute_command(_commands[command_index]);
						command_index = same_entity_modifications_end - 1;
						continue;
					}
					else if (_commands[command_index].type == entity_command_type::create_entity)
					{
						// create entity and patch the used 'virtual' one
						current_entity = _execution_world.create_entity();
						
						for (std::size_t i = command_index + 1; i < same_entity_modifications_end; ++i)
						{
							_commands[i].target = current_entity;
						}
						
						++command_index; // advance so we don't run this command again
					}
					
					std::size_t batched_commands_index = command_index;
					
					for (; batched_commands_index < same_entity_modifications_end; ++batched_commands_index)
					{
						auto &current_command = _commands[batched_commands_index];
						switch (_commands[batched_commands_index].type)
						{
							case entity_command_type::add_component:
							{
								add_command_data *command_data = reinterpret_cast<add_command_data *>(current_command.command_args);
								added_types.push_back(current_command.component_type);
								added_destructors.push_back(command_data->destructor);
								break;
							}
							case entity_command_type::remove_component:
							{
								removed_types.push_back(current_command.component_type.id);
								break;
							}
							default:
							{
								// command was not batchable
								arch_assert_internal(false);
								break;
							}
						}
					}
					
					_execution_world.modify_component_set(current_entity, {added_types}, {added_destructors}, {removed_types});
					// set component data of added components
					{
						for (std::size_t i = command_index; i < batched_commands_index; ++i)
						{
							auto &current_command = _commands[i];
							if (current_command.type == entity_command_type::add_component)
							{
								add_command_data *command_data = reinterpret_cast<add_command_data *>(current_command.command_args);
								void *target_component = _execution_world.get_component(current_entity, current_command.component_type.id);
								std::memcpy(target_component, command_data->component_data, current_command.component_type.size);
							}
						}
					}
					
					// execute commands that could not be batched
					for (std::size_t i = batched_commands_index; i < same_entity_modifications_end; ++i)
					{
						execute_command(_commands[i]);
					}
					
					// jump ahead the modifications of the next entity
					command_index = same_entity_modifications_end - 1;
					
					added_types.clear();
					added_destructors.clear();
					removed_types.clear();
				}
			}
			
			_commands.clear();
		}
	
	private:
		void optimize_entity_modification_order()
		{
			std::sort(_commands.begin(), _commands.end(), [](const entity_command &lhs, const entity_command &rhs)
			{
				if (lhs.target == rhs.target)
				{
					// bunch similar command types together
					return lhs.type < rhs.type;
				}
				else
				{
					// bunch modifications on the same entity together
					return lhs.target.id < rhs.target.id;
				}
			});
			// we could also sort by an entity's archetype, but this probably does not gain us much in typical use cases as the commands usually happen inside
			// world.foreach calls which in itself operate on a per-archetype basis
		}
		
		[[nodiscard]]
		static constexpr entity from_virtual(virtual_entity source_entity)
		{
			return {source_entity.id, std::numeric_limits<version_t>::max()};
		}
	
	private:
		/// counts how many entities this command buffer has created
		entity_id_t n_created_entities = 0;
		world &_execution_world;
		std::pmr::vector<entity_command> _commands;
		std::pmr::monotonic_buffer_resource _memory_buffer;
	};
}