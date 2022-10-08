add_executable(arch_ecs_test
        doctest.h
        test_main.cpp
        byte_vector_test.cpp
        rtt_vector_test.cpp
        query_test.cpp
        archetype_test.cpp
        world_test.cpp
        command_buffer_test.cpp
        scheduler_test.cpp
        group_test.cpp)

target_compile_options(arch_ecs_test PUBLIC -std=c++20 -Wall -Wextra -Wpedantic -Winit-self)
target_compile_definitions(arch_ecs_test PUBLIC ARCH_INTERNAL_ASSERTIONS ARCH_SAFE_PTR_INIT ARCH_VERBOSE_TYPE_INFO)