#pragma once

#include <array>
#include <vector>
#include <span>
#include <unordered_map>
#include <memory_resource>
#include <thread>
#include <barrier>

#include "type_id.hpp"
#include "entity.hpp"
#include "archetype.hpp"
#include "archecs/internal/helpers.hpp"

namespace arch
{
	class world
	{
	public:
		world()
		{
			// create base archetype
			create_archetype_with_types<>();
		}
	
	public:
		[[nodiscard]]
		entity create_entity()
		{
			entity new_entity;
			if (_dead_entities.empty())
			{
				new_entity = {static_cast<entity_id_t>(_entities.size()), 0};
			}
			else
			{
				new_entity = _dead_entities.back();
				_dead_entities.pop_back();
			}
			
			archetype &owning_archetype = get_base_archetype();
			std::size_t in_archetype_index = owning_archetype.internal().add_entity(new_entity);
			_entities.emplace_back(new_entity, BASE_ARCHETYPE_INDEX, in_archetype_index);
			
			return new_entity;
		}
		
		void destroy_entity(entity entity_to_destroy)
		{
			if (not is_alive(entity_to_destroy))
			{
				return;
			}
			
			auto &entity_info = get_info(entity_to_destroy);
			entity_info.identifier.version += 1;
			_dead_entities.push_back(entity_info.identifier);
			
			archetype &owning_archetype = _archetypes[entity_info.owning_archetype_index];
			
			entity swapped_entity = owning_archetype.internal().remove_entity(entity_info.in_archetype_index);
			get_info(swapped_entity).in_archetype_index = entity_info.in_archetype_index;
		}
		
		[[nodiscard]]
		bool is_alive(entity entity_to_check) const
		{
			if (_entities.size() <= entity_to_check.id)
			{
				return false;
			}
			
			return get_info(entity_to_check).identifier.version == entity_to_check.version;
		}
		
		template<typename ...t_added_components>
		void add_components(entity target_entity, t_added_components &&...components)
		{
			static_assert(sizeof...(t_added_components) != 0);
			
			constexpr std::array added_ids = {info_of<t_added_components>()...};
			constexpr std::array added_destructors = {det::multi_destructor_of<t_added_components>()...};
			
			modify_component_set(target_entity, {added_ids}, {added_destructors}, {});
			
			entity_info info = get_info(target_entity);
			archetype &target_archetype = _archetypes[info.owning_archetype_index];
			
			target_archetype.internal().set_components(info.in_archetype_index, arch_fwd(components)...);
		}
		
		template<typename ...t_removed_components>
		void remove_components(entity target_entity)
		{
			static_assert(sizeof...(t_removed_components) != 0);
			constexpr std::array removed_ids = ids_of<t_removed_components...>();
			modify_component_set(target_entity, {}, {}, {removed_ids});
		}
		
		void add_component(entity target_entity, type_info type_to_add, void *component_data, det::multi_destructor component_destructor)
		{
			if (has_component(target_entity, type_to_add.id))
			{
				return;
			}
			
			modify_component_set(target_entity, {&type_to_add, &type_to_add + 1}, {&component_destructor, &component_destructor + 1}, {});
			
			entity_info info = get_info(target_entity);
			archetype &target_archetype = _archetypes[info.owning_archetype_index];
			
			void *target_data = target_archetype.get_component_data(info.in_archetype_index, type_to_add.id);
			std::memcpy(target_data, component_data, type_to_add.size);
		}
		
		void remove_component(entity target_entity, type_id to_remove)
		{
			arch_assert_external(is_alive(target_entity));
			
			if (not has_component(target_entity, to_remove))
			{
				return;
			}
			
			entity_info &info = _entities[target_entity.id];
			const std::size_t previous_archetype_index = info.owning_archetype_index;
			
			// get or create archetype
			const std::uint32_t previous_archetype_hash = _archetypes[info.owning_archetype_index].internal().get_combined_types_hash();
			const std::uint32_t target_archetype_hash = det::hashing::combine_hashes(to_remove.value, previous_archetype_hash);
			
			auto archetype_search = _types_to_archetype.find(target_archetype_hash);
			archetype *target_archetype;
			if (archetype_search != _types_to_archetype.end())
			{
				target_archetype = &_archetypes[archetype_search->second];
				info.owning_archetype_index = archetype_search->second;
			}
			else
			{
				target_archetype = &create_archetype_from_base_with(previous_archetype_index, {}, {}, {&to_remove, &to_remove + 1});
				info.owning_archetype_index = _archetypes.size() - 1;
			}
			
			move_entity_to(target_entity, previous_archetype_index, *target_archetype);
		}
		
		/// changes the archetype of a given entity. WARNING: does not initialize added components
		void modify_component_set(entity target_entity, std::span<const type_info> added_types, std::span<const det::multi_destructor> added_types_destructors,
		                          std::span<const type_id> removed_types)
		{
			arch_assert_external(is_alive(target_entity));
			arch_assert_external(added_types.size() == added_types_destructors.size());
			
			entity_info &info = _entities[target_entity.id];
			const std::size_t previous_archetype_index = info.owning_archetype_index;
			std::uint32_t previous_archetype_hash = _archetypes[info.owning_archetype_index].internal().get_combined_types_hash();
			
			using det::hashing::combine_hashes;
			std::uint32_t target_archetype_hash = combine_hashes(combine_hashes(added_types), combine_hashes(removed_types));
			target_archetype_hash = combine_hashes(target_archetype_hash, previous_archetype_hash);
			
			auto archetype_search = _types_to_archetype.find(target_archetype_hash);
			archetype *target_archetype;
			if (archetype_search != _types_to_archetype.end())
			{
				target_archetype = &_archetypes[archetype_search->second];
				info.owning_archetype_index = archetype_search->second;
			}
			else
			{
				target_archetype = &create_archetype_from_base_with(previous_archetype_index, added_types, added_types_destructors, removed_types);
				info.owning_archetype_index = _archetypes.size() - 1;
			}
			
			move_entity_to(target_entity, previous_archetype_index, *target_archetype);
		}
		
		template<typename t_component>
		[[nodiscard]]
		t_component &get_component(entity of_entity)
		{
			arch_assert_external(is_alive(of_entity));
			
			entity_info info = get_info(of_entity);
			
			auto &entities_archetype = _archetypes[info.owning_archetype_index];
			void *component_data = entities_archetype.get_component_data(info.in_archetype_index, id_of<t_component>());
			return *reinterpret_cast<t_component *>(component_data);
		}
		
		template<typename t_component>
		[[nodiscard]]
		const t_component &get_component(entity of_entity) const
		{
			arch_assert_external(is_alive(of_entity));
			
			entity_info info = get_info(of_entity);
			
			const auto &entities_archetype = _archetypes[info.owning_archetype_index];
			const void *component_data = entities_archetype.get_component_data(info.in_archetype_index, id_of<t_component>());
			return *reinterpret_cast<const t_component *>(component_data);
		}
		
		[[nodiscard]]
		void *get_component(entity of_entity, type_id component_type)
		{
			arch_assert_external(is_alive(of_entity));
			
			entity_info info = get_info(of_entity);
			
			auto &entities_archetype = _archetypes[info.owning_archetype_index];
			return entities_archetype.get_component_data(info.in_archetype_index, component_type);
		}
		
		[[nodiscard]]
		const void *get_component(entity of_entity, type_id component_type) const
		{
			arch_assert_external(is_alive(of_entity));
			
			entity_info info = get_info(of_entity);
			auto &entities_archetype = _archetypes[info.owning_archetype_index];
			
			return entities_archetype.get_component_data(info.in_archetype_index, component_type);
		}
		
		[[nodiscard]]
		bool has_component(entity of_entity, type_id component_type) const
		{
			arch_assert_external(is_alive(of_entity));
			
			auto &entities_archetype = get_archetype_of(of_entity);
			
			return entities_archetype.contains_type(component_type);
		}
		
		template<typename t_component>
		[[nodiscard]]
		bool has_component(entity of_entity) const
		{
			return has_component(of_entity, id_of<t_component>());
		}
		
		[[nodiscard]]
		archetype &get_archetype_of(entity of_entity)
		{
			arch_assert_external(is_alive(of_entity));
			
			entity_info info = get_info(of_entity);
			
			return _archetypes[info.owning_archetype_index];
		}
		
		[[nodiscard]]
		const archetype &get_archetype_of(entity of_entity) const
		{
			arch_assert_external(is_alive(of_entity));
			
			entity_info info = get_info(of_entity);
			
			return _archetypes[info.owning_archetype_index];
		}
		
		template<typename t_filter, typename t_function>
		void for_all(t_filter, t_function &&function)
		{
			using searched_types = typename t_filter::resulting_components;
			for (archetype &curr_archetype: _archetypes)
			{
				std::span<const type_id> contained_types = curr_archetype.get_contained_types();
				if (t_filter::filter(contained_types))
				{
					apply_foreach_function_to_archetype(curr_archetype, function, searched_types{});
				}
			}
		}
		
		template<typename t_function>
		void for_all_with(t_function &&function)
		{
			for_all_with_impl(arch_fwd(function), det::arguments_of<t_function>());
		}
		
		template<typename t_filter, typename t_function>
		void for_all_parallel(std::size_t n_threads, t_filter, t_function &&function)
		{
			using searched_types = typename t_filter::resulting_components;
			
			std::vector<std::thread> threads{};
			threads.reserve(n_threads);
			std::span<archetype> archetypes = _archetypes;
			std::atomic<std::size_t> current_archetype_index = 0;
			while (current_archetype_index < archetypes.size())
			{
				auto contained_types = archetypes[current_archetype_index].get_contained_types();
				if (t_filter::filter(contained_types))
				{
					break;
				}
				current_archetype_index.fetch_add(1);
			}
			
			auto on_archetype_done = [&]()
			{
				current_archetype_index.fetch_add(1);
				while (current_archetype_index < archetypes.size())
				{
					auto contained_types = archetypes[current_archetype_index].get_contained_types();
					if (t_filter::filter(contained_types))
					{
						return;
					}
					current_archetype_index.fetch_add(1);
				}
			};
			
			std::barrier sync(n_threads, on_archetype_done);
			
			
			for (std::size_t thread_id = 0; thread_id < n_threads; ++thread_id)
			{
				threads.emplace_back(
						[&current_archetype_index, &function, archetypes, n_threads, thread_id, &sync]()
						{
							while (current_archetype_index < archetypes.size())
							{
								apply_foreach_function_parallel(function, t_filter(), searched_types(), archetypes[current_archetype_index], thread_id);
								
								sync.arrive_and_wait();
							}
						});
			}
			
			for (auto &thread: threads)
			{
				thread.join();
			}
		}
	
	private:
		template<typename t_function, typename ...t_args>
		void for_all_with_impl(t_function &&function, det::type_list<entity, t_args...>)
		{
			using searched_types = typename det::type_list<t_args...>;
			// currently we can simply deduct all needed types from non pointer arguments
			constexpr std::array required_type_ids = ids_of_non_pointers<t_args...>();
			for (archetype &curr_archetype: _archetypes)
			{
				std::span<const type_id> contained_types = curr_archetype.get_contained_types();
				if (contains_ids(contained_types, required_type_ids))
				{
					apply_foreach_function_to_archetype(curr_archetype, function, searched_types{});
				}
			}
		}
		
		template<typename t_function, typename ...t_components>
		static void apply_foreach_function_to_archetype(archetype &arch_restrict current_archetype, t_function &function,
		                                                det::type_list<t_components...> type_list)
		{
			static_assert(std::is_invocable_v<t_function, entity, t_components...>, "Types of function does not match with the ones of the query");
			
			constexpr std::array wanted_types = ids_of<t_components...>();
			std::array<det::rtt_vector *, sizeof...(t_components)> component_vectors;
			{
				std::span<det::rtt_vector> archetype_component_vectors = current_archetype.internal().component_vectors();
				auto archetype_types = current_archetype.get_contained_types();
				std::size_t vectors_index = 0;
				for (std::size_t wanted_types_index = 0; wanted_types_index < wanted_types.size(); ++wanted_types_index)
				{
					// search for next type
					while (archetype_types[vectors_index] < wanted_types[wanted_types_index])
					{
						++vectors_index;
					}
					
					if (archetype_types[vectors_index] != wanted_types[wanted_types_index])
					{
						// optional type was not found
						component_vectors[wanted_types_index] = nullptr;
					}
					else
					{
						component_vectors[wanted_types_index] = &archetype_component_vectors[vectors_index];
						++vectors_index;
					}
				}
			}
			
			std::span<const entity> entities = current_archetype.entities();
			std::size_t archetype_size = current_archetype.size();
			for (std::size_t i = 0; i < archetype_size; ++i)
			{
				apply_foreach_function_to_entity(function, i, entities, component_vectors, type_list, std::make_index_sequence<sizeof...(t_components)>());
			}
		}
		
		template<std::size_t array_size>
		[[nodiscard]]
		static consteval std::array<std::size_t, array_size> map_type_indices(std::array<type_id, array_size> from, std::array<type_id, array_size> to)
		{
			std::array<std::size_t, array_size> mapped_indices{};
			
			for (std::size_t i = 0; i < array_size; ++i)
			{
				auto found = std::find(to.begin(), to.end(), from[i]);
				
				if (found == to.end())
				{
					throw std::exception();
				}
				else
				{
					std::size_t to_index = found - to.begin();
					mapped_indices[i] = to_index;
				}
			}
			
			return mapped_indices;
		}
		
		template<typename t_function, std::size_t ...is, typename ...t_components>
		static void apply_foreach_function_to_entity(t_function &function, std::size_t in_archetype_index,
		                                             std::span<const entity> entities, std::span<det::rtt_vector *> component_vectors,
		                                             det::type_list<t_components...>, std::integer_sequence<std::size_t, is...>)
		{
			// function parameters are unsorted but component_vectors are sorted by type_id, so we need to map the indices
			constexpr std::array parameter_indices = map_type_indices(ids_of<t_components...>(), {id_of<t_components>()...});
			function(entities[in_archetype_index], get_from_vector_or_null<t_components>(component_vectors[parameter_indices[is]], in_archetype_index)...);
		}
		
		template<typename t_component>
		static std::conditional_t<std::is_pointer_v<t_component>, t_component, t_component &>
		get_from_vector_or_null(det::rtt_vector *arch_restrict component_vector, std::size_t in_vector_index)
		{
			if constexpr (std::is_pointer_v<t_component>)
			{
				// optional component, expect component_vector to be null
				if (component_vector == nullptr)
				{
					return nullptr;
				}
				else
				{
					return reinterpret_cast<t_component>((*component_vector)[in_vector_index]);
				}
			}
			else
			{
				// guaranteed component access
				return *reinterpret_cast<std::remove_reference_t<t_component> *>((*component_vector)[in_vector_index]);
			}
		}
		
		template<typename t_filter, typename t_function, typename ...t_components>
		static void
		apply_foreach_function_parallel(t_function &function, t_filter, det::type_list<t_components...> type_list, archetype &current_archetype,
		                                std::size_t thread_id)
		{
			//NOTE: should we leave this here so that user don't have to write & in queries?
			static_assert(std::is_invocable_v<t_function, entity, t_components...> || std::is_invocable_v<t_function, entity, t_components &...>,
			              "Types of function does not match with the ones of the query");
			
			constexpr std::array wanted_types = ids_of<t_components...>();
			auto contained_types = current_archetype.get_contained_types();
			
			std::array<det::rtt_vector *, sizeof...(t_components)> component_vectors;
			{
				std::span<det::rtt_vector> archetype_component_vectors = current_archetype.internal().component_vectors();
				std::size_t vectors_index = 0;
				for (std::size_t wanted_types_index = 0; wanted_types_index < wanted_types.size(); ++wanted_types_index)
				{
					// search for next type
					while (contained_types[vectors_index] < wanted_types[wanted_types_index])
					{
						++vectors_index;
					}
					
					if (contained_types[vectors_index] != wanted_types[wanted_types_index])
					{
						// optional type was not found
						component_vectors[wanted_types_index] = nullptr;
					}
					else
					{
						component_vectors[wanted_types_index] = &archetype_component_vectors[vectors_index];
						++vectors_index;
					}
				}
			}
			
			constexpr std::size_t thread_stride = 64;
			
			std::span<const entity> entities = current_archetype.entities();
			const std::size_t archetype_size = current_archetype.size();
			for (std::size_t iteration_begin = thread_id * thread_stride; iteration_begin < archetype_size; iteration_begin += thread_stride)
			{
				const std::size_t iteration_end = iteration_begin + thread_stride;
				for (std::size_t current_index = iteration_begin; current_index < iteration_end; ++current_index)
				{
					apply_foreach_function_to_entity(function, current_index, entities, component_vectors,
					                                 type_list, std::make_index_sequence<sizeof...(t_components)>());
				}
			}
		}
		
		void move_entity_to(entity target_entity, size_t previous_archetype_index, archetype &target_archetype)
		{
			entity_info &info = get_info(target_entity);
			det::archetype_internal &previous_archetype = _archetypes[previous_archetype_index].internal();
			auto [next_in_archetype_index, swapped_entity] = target_archetype.internal().move_entity_over_from(target_entity,
			                                                                                                   previous_archetype,
			                                                                                                   info.in_archetype_index);
			get_info(swapped_entity).in_archetype_index = info.in_archetype_index;
			info.in_archetype_index = next_in_archetype_index;
		}
		
		/// Gets the archetype that contains no components
		[[nodiscard]]
		archetype &get_base_archetype()
		{
			return _archetypes[BASE_ARCHETYPE_INDEX];
		}
		
		archetype *get_archetype_with(std::span<type_id> types)
		{
			std::uint32_t combined_hash = det::hashing::combine_hashes(types);
			
			auto result = _types_to_archetype.find(combined_hash);
			if (result == _types_to_archetype.end())
			{
				return nullptr;
			}
			return &_archetypes[result->second];
		}
		
		/// Creates an archetype that contains the types of source_archetype except the ones listed in to_remove
		archetype &create_archetype_from_base_with(std::size_t previous_archetype_index, std::span<const type_info> to_add,
		                                           std::span<const det::multi_destructor> added_destructors, std::span<const type_id> to_remove)
		{
			arch_assert_internal(to_add.size() == added_destructors.size());
			
			std::size_t created_archetype_index = _archetypes.size();
			archetype &created = _archetypes.emplace_back();
			det::archetype_internal &previous = _archetypes[previous_archetype_index].internal();
			
			{
				auto modifer = created.internal().modify_archetype();
				modifer.copy_settings_from(previous, _archetype_memory);
				
				for (std::size_t i = 0; i < to_add.size(); ++i)
				{
					modifer.add_type(_archetype_memory, to_add[i], added_destructors[i]);
				}
				
				for (type_id remove_type: to_remove)
				{
					modifer.remove_type(remove_type);
				}
			}
			// TODO: This might return the same archetype, how do we deal with this?
			// We should check this earlier, before creating a new archetype
			_types_to_archetype[created.internal().get_combined_types_hash()] = created_archetype_index;
			
			return created;
		}
		
		/// Creates an archetype that contains the types of source_archetype and, additionally, Ts...
		template<typename ...t_components>
		archetype &create_archetype_with_types()
		{
			std::size_t created_archetype_index = _archetypes.size();
			archetype &created = _archetypes.emplace_back();
			
			{
				auto modifer = created.internal().modify_archetype();
				((modifer.add_type<t_components>(_archetype_memory)), ...);
			}
			
			constexpr std::array archetype_hashes = ids_of<t_components...>();
			constexpr std::uint32_t combined_types_hash = det::hashing::combine_hashes(archetype_hashes);
			_types_to_archetype[combined_types_hash] = created_archetype_index;
			
			return created;
		}
	
	public:
		struct entity_info
		{
			entity_info() = default;
			
			entity_info(entity identifier,
			            std::size_t owning_archetype_index,
			            std::size_t in_archetype_index)
					: identifier(identifier),
					  owning_archetype_index(owning_archetype_index),
					  in_archetype_index(in_archetype_index)
			{
			}
			
			entity identifier;
			std::size_t owning_archetype_index;
			std::size_t in_archetype_index;
		};
		
		[[nodiscard]]
		entity_info &get_info(entity of_entity)
		{
			return _entities[of_entity.id];
		}
		
		[[nodiscard]]
		const entity_info &get_info(entity of_entity) const
		{
			return _entities[of_entity.id];
		}
	
	private:
		static constexpr std::size_t BASE_ARCHETYPE_INDEX = 0;
		
		std::pmr::unsynchronized_pool_resource _archetype_memory{{0, 4096}};
		
		std::vector<entity_info> _entities{};
		std::vector<entity> _dead_entities{};
		std::vector<archetype> _archetypes{};
		std::unordered_map<std::uint32_t, std::size_t> _types_to_archetype{};
	};
}