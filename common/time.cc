
#include "time.hh"
#include <chrono>

namespace kcp{
namespace util{

uint64_t time::clock_64(){
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

uint32_t time::clock_32(){
    using namespace std::chrono;
    return (uint32_t)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

uint64_t time::clock_32_to_64(uint64_t now_64, uint32_t next_32){
    uint32_t now_32 = (uint32_t)now_64;
    int32_t diff = (int32_t)(next_32 - now_32);
    return now_64 + (diff > 0 ? (uint32_t)diff : 0);
}
    
} // namespace util
} // namespace kcp