#include "pch.h"

#include "RequestHandler.h"

#include "PostDataParser.h"

#include <boost/bind.hpp>

#include "Server.h"

#include "DataBase.h"

RequestHandler::RequestHandler(Server& server) 
    : m_server(server)
{
    m_socket.reset(new boost::asio::ip::tcp::socket(server.GetIOService()));
    m_db.reset(new DataBase());
}

void RequestHandler::Answer()
{
        if (!m_socket) return;

        // reads request till the end
        boost::asio::async_read_until(*m_socket, m_request, "\r\n\r\n",
            boost::bind(&RequestHandler::Handle, shared_from_this(), _1, _2));
}

void StrToStrBuf(const std::string& str, boost::asio::streambuf& buffer)
{
    std::ostream os(&buffer);
    os << str;
}

void RequestHandler::Handle(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    std::string requestLine(boost::asio::buffers_begin(m_request.data()),
        boost::asio::buffers_end(m_request.data()));
    std::string response;
    bool needToWait = false;
    std::cout << requestLine << std::endl;
    try {

        if (requestLine.find("OPTIONS") != std::string::npos)
        {
            response = "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: POST\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 0\r\n"
            "\r\n";
        }
        
        if (requestLine.find("GET /index.html") != std::string::npos) {
            response = "HTTP/1.1 200 OK\r\nContent - Length: " +
                std::to_string(g_indexHTML.length()) + "\r\n" +
                "Content-Type: text/html; charset=utf-8\r\n" +
                "Access-Control-Allow-Origin: *\r\n" + // Set the CORS header
                "\r\n" + g_indexHTML + "\r\n\r\n";
            needToWait = true;
        }

        else if (requestLine.find("GET /home.html") != std::string::npos) {
            response = "HTTP/1.1 200 OK\r\nContent - Length: " +
                std::to_string(g_homeHTML.length()) + "\r\n" +
                "Content-Type: text/html; charset=utf-8\r\n" +
                "Access-Control-Allow-Origin: *\r\n" + // Set the CORS header
                "\r\n" + g_homeHTML + "\r\n\r\n";
            needToWait = true;
        }
        
        
        else if (requestLine.find("POST /Calculator") != std::string::npos) {
            std::istream requestStream(&m_request);
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
                    throw std::runtime_error("Error HTML format\n");
                }
                resOfCalcHTML.replace(posOfChange, pair.first.size(), std::to_string(pair.second));
            }
            const std::string whatToChange = "result";
            size_t posOfChange = resOfCalcHTML.find(whatToChange);

            if (posOfChange == std::string::npos)
            {
                response = g_notFoundResponse;
                throw std::runtime_error("Error HTML format\n");
            }
            resOfCalcHTML.replace(posOfChange, whatToChange.size(), std::to_string(sum));

            response = "HTTP/1.1 200 OK\r\nContent - Length: " +
                std::to_string(resOfCalcHTML.length()) + "\r\n" +
                "Content-Type: text/html; charset=utf-8\r\n" +
                "Access-Control-Allow-Origin: *\r\n" + // Set the CORS header
                "\r\n" + resOfCalcHTML + "\r\n\r\n";
        }
        

        else if (requestLine.find("/Login") != std::string::npos)
        {
            std::istream requestStream(&m_request);
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

            std::string email, password;

            PostDataParser::CheckLogin(postData, email, password);

            std::string userName;
            if (m_db->CheckUserData(email, password))
            {
               
                userName = "{\"UserName\":\"Known\"}";
            }
            else
            {
                userName = "{\"UserName\":\"UnKnown\"}";
            }

            response = "HTTP/1.1 200 OK\r\nContent - Length: " +
                std::to_string(userName.length()) + "\r\n" +
                "Content-Type: application/json; charset=utf-8\r\n" +
                "Access-Control-Allow-Origin: *\r\n" + // Set the CORS header
                "Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept\r\n" +
                "\r\n" + userName + "\r\n\r\n";
            
        }

        else {
            response = g_notFoundResponse;
            needToWait = true;
        }
    }
    catch (const std::exception&)
    {
        StrToStrBuf(response, m_response);
        boost::asio::async_write(*m_socket, m_response, boost::bind(&RequestHandler::afterWrite, shared_from_this(), _1, _2));
    }
    std::cout << "Responce:" << response << std::endl;
    StrToStrBuf(response, m_response);
    boost::asio::async_write(*m_socket, m_response, boost::bind(&RequestHandler::afterWrite, shared_from_this(), _1, _2));
}


void RequestHandler::afterWrite(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
   // m_socket->close();
}

RequestHandler::~RequestHandler() = default;
