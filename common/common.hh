#pragma once 
#include <boost/asio/system_timer.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <iostream>

namespace kcp{

namespace asio = boost::asio;
using udp = asio::ip::udp;

using std::placeholders::_1;
using std::placeholders::_2;

using byte = unsigned char;
using packet = std::shared_ptr<std::vector<byte>>;
// muliti thread exchange data
using link_data = std::pair<uint32_t, udp::endpoint>;
using package = std::pair<udp::endpoint, packet>;

#define RECEIVE_BUFFER_SIZE 1024 * 2
#define CONNECTION_TIMEOUT 10 * 1000
// 160 ~ 0xffffffff - 160
#define KCP_CONV_MIN 0xa0
#define KCP_CONV_MAX 0xffffff5f
#define MAX_TIMEOUT 0xffffffffffffffff
// 
#define UPDATE_INTERVAL 10

#define KCP_PACKAGE "\1\2##########\3\4"
#define KCP_KEEPALIVE_REQUEST "\1\2PING######\3\4"
#define KCP_KEEPALIVE_RESPONSE "\1\2PONG######\3\4"
#define KCP_CONNECT_REQUEST "\1\2CONNECT###\3\4"
#define KCP_CONNECT_RESPONSE "\1\2%10s\3\4" // snprintf
#define KCP_DISCONNECT_REQUEST "\1\2DISCONNECT\3\4"
#define KCP_DISCONNECT_RESPONSE KCP_PACKAGE
#define KCP_PACKAGE_SIZE sizeof(KCP_PACKAGE)

} // namespace kcp