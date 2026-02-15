

#include "protocol.hh"

namespace kcp{
std::string protocol::format_connect_response(uint32_t conv){
    std::string ret;
    ret.resize(KCP_PACKAGE_SIZE);
    sprintf(ret.data(),KCP_CONNECT_RESPONSE,std::to_string(conv).c_str());
    return ret;
}
uint32_t protocol::parse_conv_from_response(const char* data){
    int ret = 0;
    for(int i = 2;i < 12; i++) {
        if(data[i] == ' ') continue;
        if(data[i] >= '0' && data[i] <= '9') {
            ret *= 10;
            ret += data[i] - '0';
        }else {
            return 0;
        }
    }
    return ret;
}
} // namespace kcp