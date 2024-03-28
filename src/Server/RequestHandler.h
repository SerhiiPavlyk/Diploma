#pragma once
#include "pch.h"

class Server;
class DataBase;

class RequestHandler : public boost::enable_shared_from_this<RequestHandler>
{
public:
    //RequestHandler(boost::asio::ip::tcp::socket&& socket);
    RequestHandler(Server& server);
    void Answer();
    void Handle(const boost::system::error_code& ec, std::size_t bytes_transferred);
    boost::shared_ptr<boost::asio::ip::tcp::socket> GetSocket() { return m_socket; }
    void afterWrite(const boost::system::error_code& ec, std::size_t bytes_transferred);
    ~RequestHandler();
private:
    boost::shared_ptr<boost::asio::ip::tcp::socket> m_socket;
    Server& m_server;
    boost::asio::streambuf m_response;
    boost::asio::streambuf m_request;
    std::unique_ptr<DataBase> m_db;
};
