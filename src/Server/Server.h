#pragma once

#include "pch.h"

class RequestHandler;

class Server
{
public:
    Server(unsigned short port);
    void Start();
    void Stop();
    boost::asio::io_service& GetIOService() { return m_ioService; }
private:
    void ThreadFunction();
    void HandleAccept(boost::shared_ptr<RequestHandler> req, const boost::system::error_code& error);
    void StartAccept();
private:
    boost::asio::io_service m_ioService;
    boost::asio::ip::tcp::acceptor m_acceptor;
    bool m_stop;
    boost::shared_ptr<std::thread> m_thread;
};
