

#include "number.hh"
#include <random>

namespace kcp{
namespace util{

long long number::random(long long min_number, long long max_number){
    static std::random_device rd;
    static std::mt19937 generator(rd());
    return std::uniform_int_distribution<long long>(min_number,max_number)(generator);
}


}; // namespace util
} //namespace kcp