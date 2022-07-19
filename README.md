# Arch ECS
 A performance oriented, achetype-based ECS library written in ECS
 
# Features
- Type-safe [entity query system](#entity-query-system) that is easy to read.
- Zero overhead entity iteration
- Entity command buffers allow for modifying the components of entities in a reliable manner
- Update groups allow for easy scheduling and bundling of individual systems. 


# Example Code
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

# Entity Query System
Arch ECS' query system is designed to be written in a very natural way that makes it both easy to read and quick to understand. Since queries are defined at compile time, type safety is still guaranteed.
```c++
using arch::with;
using arch::with_optional;
using arch::with_exactly;
my_world.for_all(with<component1 &> and with_optional<component2>,
                 [](arch::entity, component1 &my_component1, component2 *my_component2)
                 {
                 });
// you can also use 'auto' instead of restating your component types
my_world.for_all(with<component1 &, component2 &> and with_optional<component3>,
                 [](arch::entity, auto my_component1, auto my_component2, auto my_component3)
                 {
                 });
```
