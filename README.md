# Arch ECS
 A performance oriented, achetype based ECS library written in ECS

# Usage Example
```c++
#include <archecs/arch_ecs.hpp>

struct my_float3
{
    float x, y, z;
};
struct my_other_float3
{
    float x, y, z;
};

void create_entities()
{
  arch::world my_world{};
  for (std::size_t i = 0; i != n_entities; ++i)
  {
    auto created = my_world.create_entity();
    float val = i;
    my_world.add_components(created, my_float3{val, val, val}, my_other_float3{val, val, val});
  }
}

struct dont_iterate_tag
{
};

void iterate_entities(arch::world &my_world)
{
  using arch::with;
  my_world.for_all(with<my_float3 &, my_other_float3 &> and not with<dont_iterate_tag>,
                   [](arch::entity, my_float3 &first, my_other_float3 &second)
                   {
                       // your logic goes here
                   });
}
```
