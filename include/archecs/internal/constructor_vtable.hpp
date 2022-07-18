#pragma once

#include <type_traits>
#include <memory>

namespace arch::det
{
	template<typename T>
	static inline constexpr void destruct_n(void *source, std::size_t n)
	{
		if constexpr(not std::is_trivially_destructible_v<T>)
		{
			T *typed_source = reinterpret_cast<T *>(source);
			
			std::destroy_n(typed_source, n);
		}
	}
	
	struct multi_destructor
	{
		void (*const value)(void *, std::size_t);
	};
	
	template<typename T>
	static constexpr multi_destructor multi_destructor_of()
	{
		return {&destruct_n<T>};
	}
}