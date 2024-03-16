#pragma once

#include "pch.h"

class Server {
public:
    Server(unsigned short port);
    void Start();
    void Stop();
private:
    boost::asio::io_service m_ioService;
    boost::asio::ip::tcp::acceptor m_acceptor;
    bool m_stop;
};
