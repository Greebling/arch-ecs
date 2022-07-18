#include <iostream>

#include <archecs/world.hpp>

struct Test01{};
struct Test02{};

int main()
{
	arch::world myWorld{};
	auto id1 = arch::internal::id_of<Test01>();
	auto id2 = arch::internal::id_of<Test02>();
	
	std::cout << id1.value << "|" << id2.value;
	
	myWorld.add_components<Test01>(myWorld.create_entity());
	
	
	return 0;
}
