#include "pch.h"
#include "Server.h"

#include "RequestHandler.h"
#include <boost/throw_exception.hpp>

Server::Server(unsigned short port) : m_ioService(),
m_acceptor(m_ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
m_stop(true) 
{
    std::cout << m_acceptor.local_endpoint().address().to_string() << std::endl;
}

void Server::Start() {
    m_stop = false;
    while (!m_stop) {
        boost::asio::ip::tcp::socket socket(m_ioService);
        m_acceptor.accept(socket);
        RequestHandler handler(std::move(socket));
        if (handler.Handle())
            m_acceptor.wait(boost::asio::ip::tcp::acceptor::wait_read);

    }

}

void Server::Stop()
{
    m_stop = true;
}
