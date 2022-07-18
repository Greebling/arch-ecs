#pragma once

#include <cassert>
#include <string_view>
#include <array>
#include <algorithm>

#include "../type_id.hpp"

namespace arch::det
{
	template<std::size_t N>
	[[nodiscard]]
	static constexpr std::array<type_id, N> sort(std::array<type_id, N> array)
	{
		for (std::size_t i = 1; i < N; i++)
		{
			type_id current_value = array[i];
			std::size_t insertion_index = i;
			while (insertion_index > 0 && array[insertion_index - 1] > current_value)
			{
				array[insertion_index] = array[insertion_index - 1];
				insertion_index--;
			}
			array[insertion_index] = current_value;
		}
		return array;
	}
	
	template<typename T>
	using add_const_helper = std::conditional_t<std::is_reference_v<T>, const std::remove_reference_t<T> &, const T>;
	
	template<typename ...Ts>
	struct type_list
	{
		static constexpr std::size_t size()
		{
			return sizeof...(Ts);
		}
		
		using as_const = type_list<add_const_helper<Ts>...>;
		using as_reference = type_list<std::remove_pointer_t<Ts> &...>;
		using as_pointer = type_list<std::remove_reference_t<Ts> *...>;
		using as_value = type_list<std::decay_t<Ts>...>;
	};
	
	template<typename ...As, typename ...Bs>
	static constexpr type_list<As..., Bs...> combine_lists_impl(type_list<As...>, type_list<Bs...>)
	{
		return {};
	}
	
	template<typename A, typename B>
	using combined_list = decltype(combine_lists_impl(A{}, B{}));
	
	
	template<typename Filter, typename Front, typename ...As, typename ...TDone>
	static constexpr auto filter_out_impl(type_list<Front, As...>, type_list<TDone...>)
	{
		if constexpr(sizeof...(As) == 0)
		{
			return type_list<TDone...>();
		}
		else
		{
			if constexpr(std::is_same_v<Filter, Front>)
			{
				return filter_out_impl(type_list<As...>(), type_list<TDone...>());
			}
			else
			{
				return filter_out_impl(type_list<As...>(), type_list<TDone..., Front>());
			}
		}
	}
	
	template<typename Filter, typename ...As>
	static constexpr auto filter_out(type_list<As...> l)
	{
		return filter_out_impl<Filter>(l);
	}
	
	template<typename ...Ts, std::size_t N = sizeof...(Ts)>
	[[nodiscard]]
	static constexpr std::array<type_id, N> to_type_ids(type_list<Ts...>)
	{
		constexpr std::array<type_id, N> ids = {id_of<Ts>()...};
		return sort(ids);
	}
	
	template<typename ...Ts, std::size_t N = sizeof...(Ts)>
	[[nodiscard]]
	static constexpr std::array<type_id, N> to_type_ids()
	{
		constexpr std::array<type_id, N> ids = {id_of<Ts>()...};
		return sort(ids);
	}
	
	template<typename ...Ts>
	static constexpr std::array<type_id, sizeof...(Ts)> to_type_ids(type_list<Ts...>)
	{
		return {id_of<Ts>()...};
	}
	
	template<typename MemberPtrT>
	struct function_traits
	{
	};
	
	template<typename TResult, typename TClass, typename ...TArgs>
	struct function_traits<TResult (TClass::*)(TArgs...)>
	{
		using result = TResult;
		using arguments = type_list<TArgs...>;
		
		static constexpr bool isConst = false;
	};
	
	template<typename TResult, typename TClass, typename ...TArgs>
	struct function_traits<TResult (TClass::*)(TArgs...) const>
	{
		using result = TResult;
		using arguments = type_list<TArgs...>;
		
		static constexpr bool isConst = true;
	};
	
	template<typename T>
	using arguments_of = typename function_traits<decltype(&T::operator())>::arguments;
	
	/// Generates a type_list<FunctionArgTypes...> from any given callable.
	/// NOTE: For value types this removes any const qualification, meaning 'const int' becomes 'int'
	/// \tparam T type of the callable
	/// \return type_list<Ts...> of all arguments
	template<typename T>
	static constexpr auto arg_list_of(const T &)
	{
		return arguments_of<T>{};
	}
	
}