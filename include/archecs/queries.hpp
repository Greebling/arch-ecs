#pragma once

#include <type_traits>
#include <span>

#if _MSC_VER && !__INTEL_COMPILER // msvc getting a special treatment... (https://en.cppreference.com/w/cpp/language/operator_alternative)
#include <ciso646>
#endif

#include "internal/helpers.hpp"
#include "type_id.hpp"

namespace arch
{
	namespace det
	{
		template<typename T>
		struct not_q;
		
		template<typename A, typename B>
		struct and_q;
		
		//template<typename A, typename B>
		//struct or_q;
		
		struct base_component_query
		{
		};
		
		template<typename t_filter>
		struct component_query : public base_component_query
		{
			template<typename t_filter_b>
			consteval auto operator&&(t_filter_b) const
			{
				return and_q<std::decay_t<t_filter>, std::decay_t<t_filter_b>>();
			}
			
			/*
			template<typename t_filter_b>
			consteval auto operator||(t_filter_b)
			{
				return or_q<std::decay_t<t_filter>, std::decay_t<t_filter_b>>();
			}
			*/
			
			consteval not_q<t_filter> operator!() const
			{
				return {};
			}
		};
		
		template<typename t_filter_a, typename t_filter_b>
		struct and_q : public component_query<and_q<t_filter_a, t_filter_b>>
		{
			using resulting_components = combined_list<typename t_filter_a::resulting_components, typename t_filter_b::resulting_components>;
			
			constexpr static bool filter(std::span<const type_id> type)
			{
				return t_filter_a::filter(type) && t_filter_b::filter(type);
			}
		};
		
		/* TODO: decide how to handle resulting_components (either A or B)?
		template<typename t_filter_a, typename t_filter_b>
		struct or_q : public component_query<and_q<t_filter_a, t_filter_b>>
		{
			using resulting_components = combined_list<typename t_filter_a::resulting_components, typename t_filter_b::resulting_components>;
			
			constexpr static bool filter(std::span<const type_id> type)
			{
				return t_filter_a::filter(type) || t_filter_b::filter(type);
			}
		};
		*/
		
		template<typename t_filter>
		struct not_q : public det::component_query<not_q<t_filter>>
		{
			using resulting_components = det::type_list<>;
			
			constexpr static bool filter(std::span<const type_id> types)
			{
				return !t_filter::filter(types);
			}
		};
	}
	
	/// Filters for all entities that have at least [t_components...]
	/// \tparam t_components
	template<typename ...t_components>
	struct with_q : public det::component_query<with_q<t_components...>>
	{
		static_assert(((not std::is_pointer_v<t_components>) and ...));
		
		using resulting_components = det::type_list<t_components...>;
		
		// specialization for more effective entity filtering, only catches the most basic cases, though
		template<typename ...t_components_b>
		consteval auto operator&&(with_q<t_components_b...>) const
		{
			return with_q<t_components..., t_components_b...>();
		}
		
		using det::component_query<with_q<t_components...>>::operator&&;
		
		/// \param types an in ascending order sorted span
		/// \return if that span has all types t_component...
		constexpr static bool filter(std::span<const type_id> types)
		{
			if (types.size() < sizeof...(t_components))
			{
				return false;
			}
			
			auto to_search = ids_of<t_components...>();
			return contains_ids(types, to_search);
		}
	};
	
	template<typename ...t_components>
	inline constexpr with_q<t_components...> with = with_q<t_components...>();
	
	/// Like {@code with} but does not affect the functions parameters
	/// \tparam t_components an entity needs to have
	template<typename ...t_components>
	struct has_q : public with_q<t_components...>
	{
		using resulting_components = det::type_list<>;
	};
	
	template<typename ...t_components>
	inline constexpr has_q<t_components...> has = has_q<t_components...>();
	
	
	/// Filters for all entities that have at least [t_components...]
	/// \tparam t_components
	template<typename ...t_components>
	struct with_optional_q : public det::component_query<with_q<t_components...>>
	{
		static_assert(((not std::is_pointer_v<t_components>) and ...));
		
		using resulting_components = det::type_list<t_components *...>;
		
		constexpr static bool filter(std::span<const type_id> types)
		{
			return true;
		}
	};
	
	template<typename ...t_components>
	inline constexpr with_optional_q<t_components...> with_optional = with_optional_q<t_components...>();
	
	/// Filters for all entities that are exactly made up out of [t_components...]
	/// \tparam t_component
	template<typename ...t_components>
	struct with_exactly_q : public det::component_query<with_exactly_q<t_components...>>
	{
		static_assert(((not std::is_pointer_v<t_components>) and ...));
		using resulting_components = det::type_list<t_components &...>;
		
		/// \param types an in ascending order sorted span
		/// \return if that span is equal to t_component...
		constexpr static bool filter(std::span<const type_id> types)
		{
			if (types.size() != sizeof...(t_components))
			{
				return false;
			}
			
			auto to_search = ids_of<t_components...>();
			
			auto current_q = types.begin();
			auto current_from = to_search.cbegin();
			
			while (true)
			{
				if (current_from->value != current_q->value)
				{
					return false;
				}
				else
				{
					++current_from;
					++current_q;
					if (current_q == types.end())
					{
						return true;
					}
				}
			}
		}
	};
	
	template<typename ...t_components>
	inline constexpr with_exactly_q<t_components...> with_exactly = with_exactly_q<t_components...>();
}