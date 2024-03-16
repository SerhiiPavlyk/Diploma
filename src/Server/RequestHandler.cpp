#include "pch.h"

#include "RequestHandler.h"

#include "PostDataParser.h"

#include <boost/bind.hpp>

RequestHandler::RequestHandler(boost::asio::ip::tcp::socket socket) : m_socket(std::move(socket)) {}


bool RequestHandler::Handle() {
    std::string requestLine;
    std::string response;
    bool needToWait = false;
    try {
        boost::asio::streambuf request;
        boost::asio::read_until(m_socket, request, "\r\n");
        {
            std::istream requestStream(&request);
            std::getline(requestStream, requestLine);
        }
        if (requestLine.find("GET /index.html") != std::string::npos) {
            response = "HTTP/1.1 200 OK\r\n\r\n" + g_indexHTML;
            needToWait = true;
        }
        else if (requestLine.find("GET /home.html") != std::string::npos) {
            response = "HTTP/1.1 200 OK\r\n\r\n" + g_homeHTML;
            needToWait = true;
        }
        else if (requestLine.find("POST /Calculator") != std::string::npos) {
            boost::asio::read_until(m_socket, request, "\r\n\r\n");
            std::istream requestStream(&request);
            std::string headers;
            while (true) {
                std::string header;
                std::getline(requestStream, header);
                headers += header + "\n";
                if (header == "\r")
                    break;
            }
            std::string postData;
            std::getline(requestStream, postData);

            std::unordered_map<std::string, double> postDataMap = PostDataParser::Parse(postData);

            double sum = 0.0;
            for (const auto& pair : postDataMap) {
                sum += pair.second;
            }
            std::string resOfCalcHTML = g_homeRes;

            for (const auto& pair : postDataMap) {
                size_t posOfChange = resOfCalcHTML.find(pair.first);
                if (posOfChange == std::string::npos)
                {
                    response = g_notFoundResponse;
                    throw std::exception("Error HTML format\n");
                }
                resOfCalcHTML.replace(posOfChange, pair.first.size(), std::to_string(pair.second));
            }
            const std::string whatToChange = "result";
            size_t posOfChange = resOfCalcHTML.find(whatToChange);

            if (posOfChange == std::string::npos)
            {
                response = g_notFoundResponse;
                throw std::exception("Error HTML format\n");
            }
            resOfCalcHTML.replace(posOfChange, whatToChange.size(), std::to_string(sum));

            response = "HTTP/1.1 200 OK\r\n\r\n" + resOfCalcHTML;
        }

        else if (requestLine.find("POST /Login") != std::string::npos)
        {
            boost::asio::read_until(m_socket, request, "\r\n\r\n");
            std::istream requestStream(&request);
            std::string headers;
            while (true)
            {
                std::string header;
                std::getline(requestStream, header);
                headers += header + "\n";
                if (header == "\r")
                    break;
            }
            std::string postData;
            std::getline(requestStream, postData);

            response = "HTTP/1.1 200 OK\r\n\r\n" + postData;
        }

        else {
            response = g_notFoundResponse;
            needToWait = true;
        }
    }
    catch (const std::exception&)
    {
        boost::asio::write(m_socket, boost::asio::buffer(response));
        return needToWait;
    }
    boost::asio::write(m_socket, boost::asio::buffer(response));
    return needToWait;
}


