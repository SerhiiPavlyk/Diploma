#include "pch.h"
#include "Server.h"

#include "RequestHandler.h"
#include <boost/throw_exception.hpp>

Server::Server(unsigned short port) : m_ioService(),
m_acceptor(m_ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
m_stop(false) 
{
    std::cout << m_acceptor.local_endpoint().address().to_string() << std::endl;
}

void Server::Start() 
{
    m_thread.reset(new std::thread(&Server::ThreadFunction, this));
}

void Server::Stop()
{
    m_stop = true;
}

void Server::ThreadFunction()
{
    StartAccept();
    m_ioService.run();
}

void Server::StartAccept()
{
    boost::shared_ptr<RequestHandler> req(new RequestHandler(*this));
    m_acceptor.async_accept(*(req->GetSocket()),
        boost::bind(&Server::HandleAccept, this, req, _1));
}

void Server::HandleAccept(boost::shared_ptr<RequestHandler> req, const boost::system::error_code& error)
{
    if (!error) { req->Answer(); }
    StartAccept();
}
