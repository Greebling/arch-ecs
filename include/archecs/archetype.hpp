#pragma once

#include <array>
#include <span>
#include <vector>
#include <memory_resource>

#if _MSC_VER && !__INTEL_COMPILER // msvc getting a special treatment... (https://en.cppreference.com/w/cpp/language/operator_alternative)
#include <ciso646>
#endif

#include "internal/helper_macros.hpp"
#include "internal/constructor_vtable.hpp"
#include "internal/rtt_vector.hpp"
#include "type_id.hpp"
#include "entity.hpp"

namespace arch
{
	namespace det
	{
		class archetype_internal
		{
		public:
			[[nodiscard]]
			std::uint32_t get_combined_types_hash() const
			{
				std::uint32_t result = 0;
				for (type_id info: _type_data)
				{
					result = det::hashing::combine_hashes(result, info.value);
				}
				return result;
			}
			
			/// adds a given entity to the archetype
			/// \return the index inside the archetype
			std::size_t add_entity(entity to_add)
			{
				std::size_t entity_index = _entities.size();
				
				_entities.push_back(to_add);
				for (auto &component_vector: _component_data)
				{
					component_vector.push_back();
				}
				
				return entity_index;
			}
			
			/// Removes entity by moving the last entity to its place
			/// \param index of the entity to be destroyed
			/// \return the swapped (non destroyed) entity
			[[nodiscard]]
			entity remove_entity(std::size_t index)
			{
				entity swapped_entity = _entities.back();
				_entities[index] = swapped_entity;
				_entities.pop_back();
				
				for (auto &component_vector: _component_data)
				{
					component_vector.swap_back_remove(index);
				}
				
				return swapped_entity;
			}
			
			std::pair<std::size_t, entity> move_entity_over_from(entity to_copy, archetype_internal &from_archetype, std::size_t in_archetype_index)
			{
				std::size_t own_archetype_index = add_entity(to_copy);
				
				std::size_t own_component_index = 0;
				std::size_t other_component_index = 0;
				while (other_component_index < from_archetype._type_data.size() && own_component_index < _type_data.size())
				{
					auto own_current_type = _type_data[own_component_index].value;
					auto other_current_type = from_archetype._type_data[other_component_index].value;
					if (own_current_type == other_current_type)
					{
						// copy component data over
						rtt_vector &source_component_vector = from_archetype._component_data[other_component_index];
						rtt_vector &target_component_vector = _component_data[own_component_index];
						
						void *source_data = source_component_vector[in_archetype_index];
						void *target_data = target_component_vector[own_archetype_index];
						std::memcpy(target_data, source_data, target_component_vector.sizeof_elements());
						
						++other_component_index;
						++own_component_index;
					}
					else
					{
						if (own_current_type > other_current_type)
						{
							// increase other_component_index
							++other_component_index;
							continue;
						}
						else
						{
							++own_component_index;;
							continue;
						}
					}
				}
				
				entity swapped_entity = from_archetype.remove_entity(in_archetype_index);
				return {own_archetype_index, swapped_entity};
			}
			
			template<typename ...t_added_components>
			void set_components(std::size_t entity_index, t_added_components &&...added_components)
			{
				((set_component<t_added_components>(entity_index, arch_fwd(added_components))), ...);
			}
			
			[[nodiscard]]
			std::size_t size() const
			{
				return _entities.size();
			}
			
			[[nodiscard]]
			std::span<rtt_vector> component_vectors()
			{
				return _component_data;
			}
			
			[[nodiscard]]
			std::span<const rtt_vector> component_vectors() const
			{
				return _component_data;
			}
			
			[[nodiscard]]
			std::span<const entity> entities() const
			{
				return {_entities};
			}
			
			[[nodiscard]]
			std::span<const type_id> get_contained_types() const
			{
				return {_type_data};
			}
			
			[[nodiscard]]
			bool contains_type(type_id type) const
			{
				for (type_id curr_type: _type_data)
				{
					if (curr_type == type)
					{
						return true;
					}
					else if (curr_type > type)
					{
						return false;
					}
				}
				
				return false;
			}
			
			[[nodiscard]]
			void *get_component_data(std::size_t component_index, type_id component_type)
			{
				auto *comp_vector = get_component_data_of(component_type);
				arch_assert_external(comp_vector != nullptr);
				arch_assert_external(component_index < comp_vector->size());
				
				return (*comp_vector)[component_index];
			}
			
			[[nodiscard]]
			const void *get_component_data(std::size_t component_index, type_id component_type) const
			{
				auto *comp_vector = get_component_data_of(component_type);
				arch_assert_external(comp_vector != nullptr);
				arch_assert_external(component_index < comp_vector->size());
				
				return (*comp_vector)[component_index];
			}
		
		private:
			template<typename t_add_component>
			void set_component(std::size_t entity_index, t_add_component &&to_add)
			{
				constexpr type_id curr_type = id_of<t_add_component>();
				
				// TODO: theoretically it should be possible to reuse component_vectors_index for different invocations
				std::size_t component_vectors_index = 0;
				while (curr_type.value != _type_data[component_vectors_index].value)
				{
					++component_vectors_index;
				}
				
				void *component_data = _component_data[component_vectors_index][entity_index];
				std::construct_at(reinterpret_cast<t_add_component *>(component_data), arch_fwd(to_add));
			}
		
		public:
			/// helper struct to modify what the archetype can contain. IMPORTANT: while a modifer is alive the modified archetype may not be used since
			/// its component array could be in invalid positions. if you still need to get component data while a modifer is alive make sure you call
			/// sort_types() first.
			struct archetype_modifier
			{
			public:
				constexpr explicit archetype_modifier(archetype_internal &to_modify)
						: _archetype(to_modify)
				{
				}
				
				~archetype_modifier()
				{
					// maybe not always necessary, but wont hurt since in sorted case we will only iterate over all component vectors once
					sort_types();
				}
				
				template<typename ...Ts>
				void init(std::pmr::memory_resource &resource)
				{
					_archetype._type_data = {info_of<Ts>()...};
					_archetype._component_data = {det::rtt_vector::of<Ts>(resource)...};
					init_entities(resource);
				}
				
				void copy_settings_from(const archetype_internal &other_archetype, std::pmr::memory_resource &resource)
				{
					_archetype._type_data = other_archetype._type_data;
					
					_archetype._component_data.reserve(other_archetype._component_data.size());
					for (const rtt_vector &other_vec: other_archetype._component_data)
					{
						_archetype._component_data.emplace_back(arch_fwd(det::rtt_vector::copy_settings_from(other_vec, resource)));
					}
					init_entities(resource);
				}
				
				void init_components(std::pmr::memory_resource &resource, std::span<const type_id> type_datas,
				                     std::span<const det::rtt_vector> component_datas)
				{
					arch_assert_internal(type_datas.size() == component_datas.size());
					
					_archetype._type_data = std::pmr::vector<type_id>(type_datas.begin(), type_datas.end(), &resource);
					
					_archetype._component_data = std::pmr::vector<det::rtt_vector>(&resource);
					_archetype._component_data.reserve(component_datas.size());
					for (const auto &curr_components: component_datas)
					{
						_archetype._component_data.emplace_back(det::rtt_vector(resource, curr_components.sizeof_elements(), curr_components.destructor()));
					}
				}
				
				/// initializes the archetypes entity array. usually only needs to be called once in the lifetime of an archetype
				void init_entities(std::pmr::memory_resource &resource)
				{
					_archetype._entities = std::pmr::vector<entity>(&resource);
				}
				
				/// adds a new type to the archetype
				template<typename T>
				void add_type(std::pmr::memory_resource &resource)
				{
					if (_archetype.contains_type(id_of<T>()))
					{
						return;
					}
					
					_archetype._type_data.emplace_back(id_of<T>());
					_archetype._component_data.emplace_back(det::rtt_vector::of<T>(resource));
				}
				
				/// adds a new type to the archetype
				void add_type(std::pmr::memory_resource &resource, type_info component_info, multi_destructor destruct_n)
				{
					if (_archetype.contains_type(component_info.id))
					{
						return;
					}
					
					_archetype._type_data.emplace_back(component_info.id);
					_archetype._component_data.emplace_back(det::rtt_vector(resource, component_info.size, destruct_n));
				}
				
				void remove_type(type_id to_remove)
				{
					if (not _archetype.contains_type(to_remove))
					{
						return;
					}
					
					for (std::size_t i = 0; i < _archetype._type_data.size(); ++i)
					{
						if (to_remove == _archetype._type_data[i])
						{
							_archetype._type_data[i] = _archetype._type_data.back();
							_archetype._component_data[i] = std::move(_archetype._component_data.back());
							
							_archetype._type_data.pop_back();
							_archetype._component_data.pop_back();
						}
					}
				}
				
				/// sorts all internal component vectors and type id vectors
				void sort_types()
				{
					// use basic insertion sort as we only deal with small array sizes that are sometimes nearly sorted
					const std::size_t n_components = _archetype._type_data.size();
					for (std::size_t i = 1; i < n_components; ++i)
					{
						auto curr_type = _archetype._type_data[i];
						std::size_t j = i;
						while (j > 0 and (_archetype._type_data[j - 1].value > curr_type.value))
						{
							swap_places(j, j - 1);
							j = j - 1;
						}
						_archetype._type_data[j] = curr_type;
					}
				}
			
			private:
				/// swaps the places of two component types
				void swap_places(std::size_t place1, std::size_t place2)
				{
					if (place1 == place2)
					{
						return;
					}
					
					{
						auto temp = _archetype._type_data[place1];
						_archetype._type_data[place1] = _archetype._type_data[place2];
						_archetype._type_data[place2] = temp;
					}
					{
						rtt_vector temp = _archetype._component_data[place1];
						_archetype._component_data[place1] = std::move(_archetype._component_data[place2]);
						_archetype._component_data[place2] = std::move(temp);
					}
				}
			
			private:
				archetype_internal &_archetype;
			};
			
			///IMPORTANT: while a modifer is alive the modified archetype may not be used since its component array could be in invalid positions. if you
			/// need to get component data while a modifer is alive despite this make sure you call sort_types() first.
			[[nodiscard]]
			archetype_modifier modify_archetype()
			{
				return archetype_modifier(*this);
			}
		
		protected:
			[[nodiscard]]
			det::rtt_vector *get_component_data_of(type_id component_type)
			{
				for (std::size_t i = 0; i < _type_data.size(); ++i)
				{
					if (_type_data[i] == component_type)
					{
						return &(_component_data[i]);
					}
				}
				
				return nullptr;
			}
			
			[[nodiscard]]
			const det::rtt_vector *get_component_data_of(type_id component_type) const
			{
				for (std::size_t i = 0; i < _type_data.size(); ++i)
				{
					if (_type_data[i] == component_type)
					{
						return &(_component_data[i]);
					}
				}
				
				return nullptr;
			}
		
		protected:
			std::pmr::vector<type_id> _type_data;
			std::pmr::vector<det::rtt_vector> _component_data;
			std::pmr::vector<entity> _entities;
		};
	}
	
	class archetype : protected det::archetype_internal
	{
	public:
		using det::archetype_internal::size;
		using det::archetype_internal::entities;
		using det::archetype_internal::get_component_data;
		using det::archetype_internal::get_contained_types;
		using det::archetype_internal::contains_type;
		
		[[nodiscard]]
		det::archetype_internal &internal()
		{
			return *static_cast<det::archetype_internal *>(this);
		}
		
		[[nodiscard]]
		const det::archetype_internal &internal() const
		{
			return *static_cast<const det::archetype_internal *>(this);
		}
	};
}