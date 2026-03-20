#pragma once 

#include <cstdint>

namespace kcp{
namespace util{

class time{
public:
    
    static uint64_t clock_64();

    static uint32_t clock_32();

    static uint64_t clock_32_to_64(uint64_t now_64, uint32_t next_32);
};
    
} // namespace util
} // namespace kcp