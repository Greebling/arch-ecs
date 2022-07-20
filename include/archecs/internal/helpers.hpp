#pragma once

#include <array>


namespace arch::det
{
	template<typename ...ts>
	struct type_list
	{
		static constexpr std::size_t size()
		{
			return sizeof...(ts);
		}
	};
	
	template<typename ...t_first, typename ...t_second>
	static constexpr type_list<t_first..., t_second...> combine_lists_impl(type_list<t_first...>, type_list<t_second...>)
	{
		return {};
	}
	
	template<typename A, typename B>
	using combined_list = decltype(combine_lists_impl(A{}, B{}));
	
	template<typename MemberPtrT>
	struct function_traits
	{
	};
	
	template<typename t_result, typename t_class, typename ...t_args>
	struct function_traits<t_result (t_class::*)(t_args...)>
	{
		using result = t_result;
		using arguments = type_list<t_args...>;
		
		static constexpr bool isConst = false;
	};
	
	template<typename t_result, typename t_class, typename ...t_args>
	struct function_traits<t_result (t_class::*)(t_args...) const>
	{
		using result = t_result;
		using arguments = type_list<t_args...>;
		
		static constexpr bool isConst = true;
	};
	
	template<typename t>
	using arguments_of = typename function_traits<decltype(&t::operator())>::arguments;
}