#pragma once 
#include <boost/asio/system_timer.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/ip/udp.hpp>
#include <memory>
#include <vector>


namespace kcp{

namespace asio = boost::asio;
using udp = asio::ip::udp;

using std::placeholders::_1;
using std::placeholders::_2;

using packet = std::shared_ptr<std::vector<uint8_t>>;
// muliti thread exchange data
using link_data = std::pair<uint32_t, udp::endpoint>;
using package = std::pair<udp::endpoint, packet>;

#define RECEIVE_BUFFER_SIZE (1024 * 2)
#define CONNECTION_TIMEOUT (10 * 1000)
#define HALF_CONNECTION_TIMEOUT (5 * 1000)

// socket buffer
#define SERVER_SOCKET_RECEIVE_BUFFER_SIZE (64 * 1024 * 1024)
#define SERVER_SOCKET_SEND_BUFFER_SIZE (64 * 1024 * 1024)
#define CLIENT_SOCKET_RECEIVE_BUFFER_SIZE (200 * 1024)
#define CLIENT_SOCKET_SEND_BUFFER_SIZE (200 * 1024)

#define BATCH_IO_BUFFER_SIZE 2048
#define BATCH_IO_BUFFER_NUM 32

// 160 ~ 0xffffffff - 160
#define KCP_CONV_MIN 0xa0
#define KCP_CONV_MAX 0xffffff5f
#define MAX_TIMEOUT ((uint64_t)-1)
#define MAX_TIMEOUT_TIMER (INT_MAX)

#define KCP_MODE_DEFAULT 1
#define KCP_MODE_MODERATE 2
#define KCP_MODE_FAST 3

#define  KCP_MODE KCP_MODE_MODERATE
#if KCP_MODE == KCP_MODE_DEFAULT
// kcp default configuration
#define KCP_NODELAY 0
#define KCP_INTERVAL 30
#define KCP_RESEND 0
#define KCP_NC 0
#define KCP_RTT 800
#elif KCP_MODE == KCP_MODE_MODERATE
// kcp fast mode reteins congestion control configuration
#define KCP_NODELAY 1
#define KCP_INTERVAL 30
#define KCP_RESEND 2
#define KCP_NC 0
#define KCP_RTT 200
#elif KCP_MODE == KCP_MODE_FAST
// kcp extreme fast mode configuration
#define KCP_NODELAY 1
#define KCP_INTERVAL 30
#define KCP_RESEND 2
#define KCP_NC 1
#define KCP_RTT 150
#endif

#define KCP_DISCONNECT_WAIT_TIMEOUT (KCP_RTT * 4)

#define KCP_PACKAGE "\1\2##########\3\4"
#define KCP_KEEPALIVE_REQUEST "\1\2PING######\3\4"
#define KCP_KEEPALIVE_RESPONSE "\1\2PONG######\3\4"
#define KCP_CONNECT_REQUEST "\1\2CONNECT###\3\4"
#define KCP_CONNECT_RESPONSE "\1\2%10s\3\4" // snprintf
#define KCP_CONNECT_RESPONSE_ACK "\4\3%10s\2\1" // snprintf
#define KCP_DISCONNECT_REQUEST "\1\2DISCONNECT\3\4"
#define KCP_DISCONNECT_RESPONSE KCP_PACKAGE
#define KCP_PACKAGE_SIZE sizeof(KCP_PACKAGE)

#define KCP_CONNECT_TIME_WAIT KCP_RTT
#define KCP_CONNECT_TIME_WAIT_MAX (KCP_CONNECT_TIME_WAIT << 5)
#define kcp_timewait(time) (time == 0 ? KCP_CONNECT_TIME_WAIT : (time > KCP_CONNECT_TIME_WAIT_MAX ? -1 : (time << 1)))

} // namespace kcp
