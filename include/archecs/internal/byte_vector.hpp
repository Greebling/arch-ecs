#pragma once

#include <cstddef>
#include <cstring>
#include <memory_resource>
#include "helper_macros.hpp"

namespace arch::det
{
	
	/// A vector implementation that only stores bytes
	class byte_vector
	{
	public:
		constexpr byte_vector() noexcept = default;
		
		constexpr explicit byte_vector(std::pmr::memory_resource &resource) noexcept
				: _resource(&resource)
		{
		}
		
		byte_vector(byte_vector &&other) noexcept
				: _resource(other._resource),
				  _data_begin(other._data_begin),
				  _data_end(other._data_end),
				  _capacity_end(other._capacity_end)
		{
			other._data_begin = nullptr;
			other._data_end = nullptr;
			other._capacity_end = nullptr;
			other._resource = nullptr;
		}
		
		byte_vector(const byte_vector &other) noexcept
				: _resource(other._resource)
		{
			if (other._data_begin != nullptr)
			{
				set_capacity(other.byte_capacity());
				std::memcpy(_data_begin, other._data_begin, other.byte_size());
				_data_end = _data_begin + other.byte_size();
				_capacity_end = _data_begin + other.byte_capacity();
			}
		}
		
		~byte_vector()
		{
			if (_data_begin != nullptr)
			{
				_resource->deallocate(_data_begin, byte_capacity());
			}
		}
		
		byte_vector &operator=(byte_vector &&other) noexcept
		{
			if (this != &other)
			{
				std::construct_at(this, arch_fwd(other));
			}
			return *this;
		}
		
		byte_vector &operator=(const byte_vector &other) noexcept
		{
			if (this != &other)
			{
				std::construct_at(this, other);
			}
			return *this;
		}
	
	public:
		template<typename T>
		void initialize()
		{
			set_capacity(sizeof(T) * 8);
		}
		
		/// \return the size of contained elements in bytes
		[[nodiscard]]
		std::size_t byte_size() const noexcept
		{
			return reinterpret_cast<std::uintptr_t>(_data_end) - reinterpret_cast<std::uintptr_t>(_data_begin);
		}
		
		/// \return the number of bytes reserved for the vector
		[[nodiscard]]
		std::size_t byte_capacity() const noexcept
		{
			return _capacity_end - _data_begin;
		}
		
		/// Similar to push_back, just with using raw byte data
		/// \param data to add to the vector
		/// \param size of the data to add. Note that the size of the type already contained must be equal to size
		void push_back_bytes(const void *data, std::size_t size)
		{
			const auto target_size = byte_size() + size;
			if (byte_capacity() < target_size)
			{
				auto next_capacity = next_size(byte_capacity(), target_size);
				set_capacity(next_capacity);
			}
			
			std::memcpy(_data_end, data, size);
			_data_end = _data_begin + target_size;
		}
		
		/// Similar to push_back, just with using raw byte data
		/// \param data to add to the vector
		/// \param size of the data to add. Note that the size of the type already contained must be equal to size
		void push_back_bytes(std::size_t size)
		{
			const auto target_size = byte_size() + size;
			if (byte_capacity() < target_size)
			{
				auto next_capacity = next_size(byte_capacity(), target_size);
				set_capacity(next_capacity);
			}
			
			_data_end = _data_begin + target_size;
		}
		
		/// Reduces the size of the vector by size bytes
		/// \param size
		void pop_back_bytes(std::size_t size) noexcept
		{
			_data_end -= size;
		}
		
		void *get_bytes(std::size_t offset)
		{
			return _data_begin + offset;
		}
	
	protected:
		void set_capacity(std::size_t target_capacity)
		{
			const auto previous_size = byte_size();
			const auto next_capacity = target_capacity;
			auto *next = reinterpret_cast<std::byte *>(_resource->allocate(next_capacity));
			
			std::memcpy(next, _data_begin, previous_size);
			_data_begin = next;
			_data_end = next + previous_size;
			_capacity_end = _data_begin + next_capacity;
		}
		
		/// Growth policy of the vector. Returns the next power of 2 that is equal to or larger than n
		/// \param minimum amount we want to at least store.
		/// \return the size the capacity should grow to
		[[nodiscard]] static constexpr std::size_t next_size(std::size_t current, std::size_t minimum) noexcept
		{
			while (current < minimum)
			{
				current = grow_once(current);
			}
			return current;
		}
		
		[[nodiscard]] static constexpr std::size_t grow_once(std::size_t n) noexcept
		{
			if (n < 4)
			{
				return 4;
			}
			return n * 2;
		}
	
	protected:
		std::pmr::memory_resource *_resource{};
		std::byte *_data_begin{};
		/// end of the current elements. note that only the element before sizeEnd is valid
		std::byte *_data_end{};
		/// end of the allocated memory for the vector. note that only the byte before _capacityEnd is usable
		std::byte *_capacity_end{};
	};
}
