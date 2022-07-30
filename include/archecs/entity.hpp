#pragma once

#include <limits>

namespace arch
{
	using entity_id_t = std::uint32_t;
	using version_t = std::uint32_t;
	
	struct entity
	{
		entity_id_t id;
		version_t version;
		
		constexpr bool operator==(entity other) const noexcept
		{
			return (id == other.id) & (version == other.version);
		}
		
		constexpr bool operator!=(entity other) const noexcept
		{
			return (id != other.id) || (version != other.version);
		}
		
		constexpr bool operator<(entity other) const
		{
			static_assert(sizeof(entity_id_t) + sizeof(version_t) <= sizeof(std::uint_fast64_t));
			constexpr std::uint_fast64_t id_shift = sizeof(entity_id_t) * 8;
			
			std::uint_fast64_t entity_ver_id = std::uint_fast64_t(version) | (std::uint_fast64_t(id) << id_shift);
			std::uint_fast64_t other_entity_ver_id = std::uint_fast64_t(other.version) | (std::uint_fast64_t(other.id) << id_shift);
			return entity_ver_id < other_entity_ver_id;
		}
		
		constexpr bool operator>(entity other) const
		{
			static_assert(sizeof(entity_id_t) + sizeof(version_t) <= sizeof(std::uint64_t));
			std::uint64_t entity_ver_id = std::uint64_t(version) | (std::uint64_t(id) << sizeof(entity_id_t));
			std::uint64_t other_entity_ver_id = std::uint64_t(other.version) | (std::uint64_t(other.id) << sizeof(entity_id_t));
			return entity_ver_id > other_entity_ver_id;
		}
		
		[[nodiscard]]
		static constexpr entity null()
		{
			return {std::numeric_limits<entity_id_t>::max(), std::numeric_limits<version_t>::max()};
		}
	};
	
	struct virtual_entity
	{
		constexpr virtual_entity() = default;
		
		constexpr explicit virtual_entity(entity_id_t id) : id(id)
		{
		}
		
		entity_id_t id;
		
		constexpr bool operator==(const virtual_entity &other) const noexcept
		{
			return (id == other.id);
		}
		
		constexpr bool operator!=(const virtual_entity &other) const noexcept
		{
			return (id != other.id);
		}
		
		[[nodiscard]]
		static constexpr virtual_entity null()
		{
			return virtual_entity(std::numeric_limits<entity_id_t>::max());
		}
	};
}