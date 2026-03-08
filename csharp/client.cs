using System;
using System.Net;
using System.Text;
using AKcp;


class Client
{
    private AKcpClient _client;
    private Channel?  _channel;
    public Client()
    {
        _client = new AKcpClient();
        _client.SetReceiveCallback(ReceiveCallback);
        _client.SetConnectCallback(ConnectCallback);
        _client.EnableTickUpdate();
    }

    private void ReceiveCallback(Channel channel, byte[] data)
    {
        string message = Encoding.UTF8.GetString(data);
        Console.WriteLine($"{channel.GetEndPoint()} :  {message}");
    }
    private void ConnectCallback(Channel channel, bool isLink)
    {
        if (isLink)
        {
            _channel = channel;
            Console.WriteLine($"{channel.GetEndPoint()} : 获取连接");
            channel.SendMessage("你好");
        }
        else
        {
            _channel = null;
            Console.WriteLine($"{channel.GetEndPoint()} : 断开连接");
        }
    }
    async public void  Start()
    {
        _client.Start();
        _client.Connect("127.0.0.1", 8080);
        Console.ReadLine();
        _channel.Disconnect();
        _client.Stop();
    }

    public static void Main()
    {
        Client t = new Client();
        t.Start();
    }
}