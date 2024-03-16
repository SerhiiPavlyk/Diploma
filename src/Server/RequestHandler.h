#pragma once
#include "pch.h"


class RequestHandler {
public:
    RequestHandler(boost::asio::ip::tcp::socket socket);
    bool Handle();
private:
    boost::asio::ip::tcp::socket m_socket;
};
