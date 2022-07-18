#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <memory_resource>

#include "helper_macros.hpp"
#include "constructor_vtable.hpp"
#include "byte_vector.hpp"

namespace arch::det
{
	/// A runtime typed vector implementation
	class rtt_vector : public byte_vector
	{
	public:
		using memory = std::pmr::memory_resource;
	public:
		template<typename T>
		static rtt_vector of(memory &memory)
		{
			return rtt_vector(memory, sizeof(T), multi_destructor_of<T>());
		}
		
		static rtt_vector copy_settings_from(const rtt_vector &other, memory &memory)
		{
			return rtt_vector(memory, other.element_size, other.destruct_n);
		}
		
		constexpr explicit rtt_vector(memory &memory, std::size_t element_size, multi_destructor destruct_n) noexcept
				: byte_vector(memory),
				  element_size(element_size),
				  destruct_n(destruct_n)
		{
		}
		
		rtt_vector(rtt_vector &&other) noexcept
				: byte_vector(arch_fwd(other)),
				  element_size(other.element_size),
				  destruct_n(other.destruct_n)
		{
		}
		
		rtt_vector(const rtt_vector &other)
				: byte_vector(other),
				  element_size(other.element_size),
				  destruct_n(other.destruct_n)
		{
		}
		
		rtt_vector &operator=(rtt_vector &&other) noexcept
		{
			if (this != &other)
			{
				std::construct_at(this, arch_fwd(other));
			}
			return *this;
		}
		
		rtt_vector &operator=(const rtt_vector &other) noexcept
		{
			if (this != &other)
			{
				std::construct_at(this, other);
			}
			return *this;
		}
		
		~rtt_vector()
		{
			if (_data_begin)
			{
				destruct_n.value(_data_begin, size());
			}
		}
		
		void *operator[](std::size_t index)
		{
			return _data_begin + (index * element_size);
		}
		
		const std::byte *operator[](std::size_t index) const
		{
			return _data_begin + (index * element_size);
		}
		
		void push_back()
		{
			reserve_one_element();
			_data_end += element_size;
		}
		
		void push_back(void *data)
		{
			reserve_one_element();
			std::memcpy(_data_end, data, element_size);
			_data_end += element_size;
		}
		
		void pop_back()
		{
			arch_assert_internal(byte_size() != 0);
			
			_data_end -= element_size;
			destruct_n.value(_data_end, 1);
		}
		
		void swap_back_remove(std::size_t index)
		{
			std::memcpy((*this)[index], _data_end - element_size, element_size);
			pop_back();
		}
		
		/// Preallocates a given number of elements
		/// \param number of elements that shall be containable without a new allocations
		void reserve(std::size_t number)
		{
			if (number <= capacity())
			{
				// already can contain this many elements
				return;
			}
			auto next_capacity = next_size(byte_capacity(), number * element_size);
			set_capacity(next_capacity);
		}
		
		void resize(std::size_t target_size)
		{
			const auto previousSize = this->size();
			if (previousSize == target_size) [[unlikely]]
			{
				// noop
				return;
			}
			else if (previousSize < target_size)
			{
				// add elements at newEnd
				reserve(target_size);
				
				// init objects
				const auto *newEnd = _data_begin + target_size;
				for (std::byte *current = _data_begin + previousSize; current < newEnd; current += element_size)
				{
					std::construct_at(current);
				}
				_data_end = _data_begin + target_size * element_size;
			}
			else // previousSize > size
			{
				auto nRemovedElements = target_size - previousSize;
				// remove last elements
				for (std::byte *current = _data_end - nRemovedElements; current < _data_end; ++current += element_size)
				{
					std::destroy_at(current);
				}
				_data_end = _data_begin + target_size * element_size;
			}
		}
		
		[[nodiscard]]
		std::size_t size() const
		{
			return byte_size() / element_size;
		}
		
		[[nodiscard]]
		std::size_t capacity() const
		{
			return byte_capacity() / element_size;
		}
		
		[[nodiscard]]
		std::size_t sizeof_elements() const
		{
			return element_size;
		}
		
		[[nodiscard]]
		multi_destructor destructor() const
		{
			return destruct_n;
		}
	
	private:
		void reserve_one_element()
		{
			auto target_size = byte_size() + element_size;
			if (target_size <= byte_capacity())
			{
				// already can contain this many elements
				return;
			}
			auto next_capacity = next_size(byte_capacity(), target_size);
			set_capacity(next_capacity);
		}
	
	private:
		const std::size_t element_size = 0;
		
		multi_destructor destruct_n;
	};
}