

#include "address.hh"


namespace kcp{
namespace util{

void address::point_to_string(const udp::endpoint& point, std::string* host){
    assert(host);
    host->clear();
    host->reserve(22); // 255.255.255.255:65535
    host->append(point.address().to_string());
    host->push_back(':');
    host->append(std::to_string(point.port()));
    return ;
}

void address::host_to_string(const std::string& ip, int port, std::string* host){
    assert(host);
    host->clear();
    host->reserve(22); // 255.255.255.255:65535
    host->append(ip);
    host->push_back(':');
    host->append(std::to_string(port));
    return ;
}

} // namespace util
} // namespace kcp