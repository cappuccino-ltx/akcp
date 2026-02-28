

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

void address::string_to_host(const std::string& host, std::string* ip, std::string* port){
    assert(ip && port);
    ip->clear();
    port->clear();
    int pos = host.find(':');
    *ip = host.substr(0,pos);
    *port = host.substr(pos + 1);
    return ;
}

} // namespace util
} // namespace kcp