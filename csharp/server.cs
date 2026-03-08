

using AKcp;
using System;
using System.Text;

class Server
{
    private AKcpServer _server;

    public Server()
    {
        _server = new AKcpServer(8080);
        _server.SetConnectCallback(ConnectCallback);
        _server.SetReceiveCallback(ReceiveCallback);
        _server.EnableTickUpdate();
    }
    private void ReceiveCallback(Channel channel, byte[] data)
    {
        Console.WriteLine($"{channel.GetEndPoint()} :  {Encoding.UTF8.GetString(data)}");
        channel.SendMessage(data);
    }
    private void ConnectCallback(Channel channel, bool isLink)
    {
        if (isLink)
        {
            Console.WriteLine($"{channel.GetEndPoint()} : 获取连接");
        }
        else
        {
            Console.WriteLine($"{channel.GetEndPoint()} : 断开连接");
        }
    }
    void Start()
    {
        _server.Start();
    }

    public static async Task Main()
    {
        Server server = new Server();
        server.Start();
        await Task.Delay(-1);
    }
}