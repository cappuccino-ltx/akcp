# akcp(asio 和 kcp 的封装库)

## akcp库的编译安装

## akcp库的使用

因为在udp的服务端是通过一个udp的套接字和所有的客户端进行通信的,所以服务端的系统套接字的接收缓冲区和发送缓冲区的大小不能太小, 这很重要,如果太小的话,就会导致在一瞬间请求过多的时候,出现大量丢包现象.

在akcp库中,建立套接字的时候,通过asio库的函数接口设置了发送缓冲区和接收缓冲区的大小,

```C++
// common.hh
// socket buffer
#define SERVER_SOCKET_RECEIVE_BUFFER_SIZE (64 * 1024 * 1024)
#define SERVER_SOCKET_SEND_BUFFER_SIZE (64 * 1024 * 1024)
#define CLIENT_SOCKET_RECEIVE_BUFFER_SIZE (1 * 1024 * 1024)
#define CLIENT_SOCKET_SEND_BUFFER_SIZE (1 * 1024 * 1024)
```

```C++
// io_socket.cc
// server
boost::asio::socket_base::receive_buffer_size receive_option(SERVER_SOCKET_RECEIVE_BUFFER_SIZE);
socket_.set_option(receive_option);
boost::asio::socket_base::send_buffer_size send_option(SERVER_SOCKET_SEND_BUFFER_SIZE);
socket_.set_option(send_option);

// client
boost::asio::socket_base::receive_buffer_size receive_option(CLIENT_SOCKET_RECEIVE_BUFFER_SIZE);
socket_.set_option(receive_option);
boost::asio::socket_base::send_buffer_size send_option(CLIENT_SOCKET_SEND_BUFFER_SIZE);
socket_.set_option(send_option);
```

但是在代码中调整是不够的,系统是有硬限制的,我们需要再启动服务和客户端之前进行调整.

在linux中

```bash
sudo sysctl -w net.core.rmem_max=134217728
sudo sysctl -w net.core.rmem_default=134217728
sudo sysctl -w net.core.wmem_max=134217728
sudo sysctl -w net.core.wmem_default=134217728
```

在windows中,我们一般只需要启动客户端程序,不需要将服务器程序启动在windows上并进行压力测试,所以不需要关注udp缓冲区大小,一般默认的就是足够的.

但是如果您需要在windows上进行服务器的启动和测试,那就需要您去了解windows相关的参数设置



## akcp库的测试性能