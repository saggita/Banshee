#include <chrono>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include "memory/local_memory_api.h"
#include "memory/local_memory_area.h"
#include "intersection/intersection_api.h"

#include "math/matrix.h"

int main()
{
    LocalMemoryApi api;
    
    float a[10] = { 1.f, 2.f , 3.f, 4.f, 5.f };
    
    struct fff
    {
        int a, b, c;
      
        fff() : a(10), b(16), c(20){}
    };
    
    auto mem_area = api.Allocate<float>(10, a);
    auto mem_area1 = api.Allocate<fff>(1034);
    
    
    float const* ptr = mem_area->ptr();
    for (int i = 0; i < 10; ++i)
        std::cout << ptr[i] << " ";
    
    for (int i = 0; i < 10; ++i)
        std::cout << mem_area1->ptr()[i].b << " ";

    fff f;
    api.Set(*mem_area1, f, 0, 1034);

    for (int i = 0; i < 10; ++i)
        std::cout << mem_area1->ptr()[i].b << " ";

    return 0;
}