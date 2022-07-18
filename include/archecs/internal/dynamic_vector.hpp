#pragma once

#include <cstddef>
#include <cassert>
#include <cstring>
#include <cmath>
#include <memory>
#include <memory_resource>

#include "byte_vector.hpp"

namespace arch::det
{
	
	class dynamic_vector : public byte_vector
	{
	public:
		
		constexpr dynamic_vector() noexcept = default;
		
		constexpr explicit dynamic_vector(std::pmr::memory_resource &resource) noexcept
				: byte_vector(resource)
		{
		}
		
		/// Calls the destructor of all elements contained in elements
		/// \tparam T
		template<typename T>
		void destruct_elements() noexcept(std::is_nothrow_destructible_v<T>)
		{
			for (T *current = reinterpret_cast<T *>(_data_begin); current < reinterpret_cast<T *>(_data_end); ++current)
			{
				std::destroy_at(current);
			}
			
			_resource->deallocate(_data_begin, byte_capacity());
			
			_data_end = _data_begin;
			_capacity_end = _data_begin;
		}
		
		/// Gets the indexth element in the vector
		/// \tparam T Type of object to get
		/// \param index of element to get
		/// \return the element at given index
		template<typename T>
		[[nodiscard]] T &get(std::size_t index) noexcept
		{
			return *(reinterpret_cast<T *>(_data_begin) + index);
		}
		
		/// Gets the indexth element in the vector
		/// \tparam T Type of object to get
		/// \param index of element to get
		/// \return the element at given index
		template<typename T>
		[[nodiscard]] const T &get(std::size_t index) const noexcept
		{
			return *(reinterpret_cast<T *>(_data_begin) + index);
		}
		
		/// Calculates how many objects of given type are contained in this vector
		/// \tparam T Type of objects
		/// \return The number of objects that are contained
		template<typename T>
		[[nodiscard]] std::size_t size() const noexcept
		{
			return byte_size() / sizeof(T);
		}
		
		/// Calculates how many objects of given type can be contained in this vector without reallocation
		/// \tparam T Type of objects
		/// \return The number of objects that can be contained without a reallocation
		template<typename T>
		[[nodiscard]] std::size_t capacity() const noexcept
		{
			return byte_capacity() / sizeof(T);
		}
		
		/// Adds an element at the back
		/// \tparam T Type of element to add
		template<typename T>
		void push_back()
		{
			reserve_one_element<T>();
			new(reinterpret_cast<T *>(_data_end)) T();
			_data_end += sizeof(T);
		}
		
		/// Adds an element at the back
		/// \tparam T Type of element to add
		template<typename T>
		void push_back(const T &element)
		{
			reserve_one_element<T>();
			std::construct_at(reinterpret_cast<T *>(_data_end), element);
			_data_end += sizeof(T);
		}
		
		/// Emplaces an element to the back of the vector
		/// \tparam T Type of element to add
		/// \param element to emplace
		template<typename T>
		void emplace_back(T &&element)
		{
			reserve_one_element<T>();
			new(reinterpret_cast<T *>(_data_end)) T(std::forward<T>(element));
			_data_end += sizeof(T);
		}
		
		/// Constructs an element at the back of the vector
		/// \tparam T Type of element to add
		/// \param args Constructor arguments to use
		template<typename T, typename ...TArgs>
		void emplace_back(TArgs &&...args)
		{
			static_assert(std::is_constructible_v<T, TArgs...>, "Invalid constructor arguments given");
			
			reserve_one_element<T>();
			new(reinterpret_cast<T *>(_data_end)) T(std::forward<TArgs>(args)...);
			_data_end += sizeof(T);
		}
		
		/// Call the copy constructor of T amount times, appending the copies to the end of the vector
		/// \tparam T Type of element to clone
		/// \param amount of clones to create
		/// \param prototype to clone from
		template<typename T>
		void clone(std::size_t amount, const T &prototype)
		{
			const auto previousSize = size<T>();
			resize_unsafe<T>(previousSize + amount);
			
			for (T *curr = reinterpret_cast<T *>(_data_begin) + previousSize; curr < reinterpret_cast<T *>(_data_end); ++curr)
			{
				new(curr) T(prototype);
			}
		}
		
		/// Removes the last element
		/// \tparam T Destructor of that type will be called on the last element
		template<typename T>
		void pop_back() noexcept(std::is_nothrow_destructible_v<T>)
		{
			if (byte_size() == 0) [[unlikely]]
			{
				return;
			}
			
			_data_end -= sizeof(T);
			std::destroy_at(reinterpret_cast<T *>(_data_end));
		}
		
		/// Preallocates a given number of elements
		/// \tparam T Type to use for number of individual elements
		/// \param number of elements that shall be containable without a new allocations
		template<typename T>
		void reserve(std::size_t number)
		{
			if (number <= capacity<T>())
			{
				// already can contain this many elements
				return;
			}
			auto next_capacity = next_size(byte_capacity(), number * sizeof(T));
			set_capacity(next_capacity);
		}
		
		/// Sets size<T> = size and does a reallocation if necessary
		/// \tparam T Type to use for resizing. Its default constructor will be called for new elements
		/// \param size Number of elements to contain in vector
		template<typename T>
		void resize(std::size_t size)
		{
			const auto previousSize = this->size<T>();
			if (previousSize == size) [[unlikely]]
			{
				// noop
				return;
			}
			else if (previousSize < size)
			{
				// add elements at newEnd
				reserve<T>(size);
				
				// init objects
				const auto *newEnd = reinterpret_cast<T *>(_data_begin) + size;
				for (T *current = reinterpret_cast<T *>(_data_begin) + previousSize; current < newEnd; ++current)
				{
					std::construct_at(current);
				}
				_data_end = _data_begin + size * sizeof(T);
			}
			else // previousSize > size
			{
				auto nRemovedElements = size - previousSize;
				// remove last elements
				for (T *current = reinterpret_cast<T *>(_data_end) - nRemovedElements;
				     current < reinterpret_cast<T *>(_data_end); ++current)
				{
					std::destroy_at(current);
				}
				_data_end = _data_begin + size * sizeof(T);
			}
		}
		
		/// Sets size<T> = size and does a reallocation if necessary.
		/// WARNING: This does not call constructors or destructors, meaning that you will be accessing unallocated memory.
		/// However, this method is considerably faster for large sizes
		/// \tparam T Type to use for resizing. Its default constructor will be called for new elements
		/// \param size Number of elements to contain in vector
		template<typename T>
		void resize_unsafe(std::size_t size)
		{
			auto previousSize = this->size<T>();
			if (previousSize == size) [[unlikely]]
			{
				// noop
				return;
			}
			else if (previousSize < size)
			{
				// add elements at end
				reserve<T>(size);
				_data_end = _data_begin + size * sizeof(T);
			}
			else // previousSize > size
			{
				_data_end = _data_begin + size * sizeof(T);
			}
		}
	
	private:
		template<typename T>
		void reserve_one_element()
		{
			auto target_size = byte_size() + sizeof(T);
			if (target_size <= byte_capacity())
			{
				// already can contain this many elements
				return;
			}
			auto next_capacity = next_size(byte_capacity(), target_size);
			set_capacity(next_capacity);
		}
	};
}