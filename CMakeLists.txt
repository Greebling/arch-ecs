cmake_minimum_required(VERSION 3.20)
project(arch_ecs
        VERSION 0.1
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_library(arch_ecs INTERFACE
        include/archecs/internal/byte_vector.hpp
        include/archecs/internal/constructor_vtable.hpp
        include/archecs/internal/dynamic_vector.hpp
        include/archecs/internal/helper_macros.hpp
        include/archecs/internal/helpers.hpp
        include/archecs/internal/rtt_vector.hpp
        include/archecs/archetype.hpp
        include/archecs/arch_ecs.hpp
        include/archecs/command_buffer.hpp
        include/archecs/entity.hpp
        include/archecs/queries.hpp
        include/archecs/type_id.hpp
        include/archecs/world.hpp
        include/archecs/internal/scheduler.hpp
        include/archecs/system.hpp
        include/archecs/update_group.hpp)

target_include_directories(arch_ecs INTERFACE include)

target_compile_options(arch_ecs INTERFACE -std=c++20)

# test build
if(CMAKE_PROJECT_NAME STREQUAL "arch_ecs" AND BUILD_TEST)
    include(CTest)
    add_subdirectory(test)
    target_link_libraries(arch_ecs_test PRIVATE arch_ecs)
endif()

# benchmark build
if(CMAKE_PROJECT_NAME STREQUAL "arch_ecs" AND BUILD_BENCHMARK)
    add_subdirectory(benchmark)
    target_link_libraries(arch_ecs_benchmark PRIVATE arch_ecs)
endif()