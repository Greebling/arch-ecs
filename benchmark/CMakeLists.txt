include(FetchContent)


FetchContent_Declare(
        nanobench
        GIT_REPOSITORY https://github.com/martinus/nanobench.git
        GIT_TAG v4.1.0
        GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nanobench)


add_executable(arch_ecs_benchmark main.cpp)

add_subdirectory(libs/entt-3.10.1)

target_link_libraries(arch_ecs_benchmark PRIVATE nanobench)

target_link_libraries(arch_ecs_benchmark PRIVATE EnTT::EnTT)
target_include_directories(arch_ecs_benchmark PRIVATE libs/entt-3.10.1/src)
target_compile_options(arch_ecs_benchmark PUBLIC -ffast-math)