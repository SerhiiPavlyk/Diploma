#include "pch.h"

#include "RequestHandler.h"

#include "PostDataParser.h"

#include <boost/bind.hpp>

#include "Server.h"

#include "DataBase.h"

#include "DataBaseMock.h"

#include "nlohmann/json.hpp"

RequestHandler::RequestHandler(Server& server)
	: m_server(server)
{
	m_socket.reset(new boost::asio::ip::tcp::socket(server.GetIOService()));
	m_db.reset(new DataBaseMock());
}

void RequestHandler::Answer()
{
	if (!m_socket)
		return;

	const std::size_t buffer_size = 8 * 1024;

	m_request.prepare(buffer_size);

	// reads request till the end
	boost::asio::async_read_until(*m_socket, m_request, "\r\n\r\n",
		boost::bind(&RequestHandler::Handle, shared_from_this(), _1, _2));
}


void RequestHandler::Handle(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	if (ec)
	{
		std::cout << ec << std::endl;
	}

	std::string requestLine(boost::asio::buffers_begin(m_request.data()),
		boost::asio::buffers_end(m_request.data()));
	std::string response;
	std::cout << requestLine << std::endl;
	try
	{
		if (requestLine.find("OPTIONS") != std::string::npos)
		{
			response = g_corp_header + "Content - Length: 0\r\n"+
				"\r\n";
		}
		else if (requestLine.find("POST /Login") != std::string::npos)
		{
			std::string body;
			GetBody(body);

			std::string email, password, userName;

			PostDataParser::CheckLogin(body, email, password);

			std::string jsonResponse;
			if (m_db->CheckUserData(email, password, userName))
				jsonResponse = "{\"Option\":\"Allow\",\n\"UserName\": \""+ userName +"\"}";
			else
				jsonResponse = "{\"Option\":\"NotAllow\"}";

			response = "HTTP/1.1 200 OK\r\nContent - Length: " +
				std::to_string(jsonResponse.length()) + "\r\n" +
				"Content-Type: application/json; charset=utf-8\r\n" +
				g_corp_access_all + 
				"\r\n" + jsonResponse + "\r\n\r\n";
		}
		else if (requestLine.find("POST /CheckToken") != std::string::npos)
		{
			std::string body;
			GetBody(body);
			std::string userName;

			PostDataParser::CheckUserName(body, userName);
			nlohmann::json jsonResponse;
			if (m_db->CheckUserData(userName))
				jsonResponse["Option"] = "Allow";
			else
				jsonResponse["Option"] = "NotAllow";

			response = "HTTP/1.1 200 OK\r\nContent - Length: " +
				std::to_string(jsonResponse.dump().length()) + "\r\n" +
				"Content-Type: application/json; charset=utf-8\r\n" +
				g_corp_access_all +
				"\r\n" + jsonResponse.dump() + "\r\n\r\n";
		}

		else if (requestLine.find("POST /SupportedBackupRules") != std::string::npos)
		{
			std::string body;
			GetBody(body);
			std::string userName;

			PostDataParser::CheckUserName(body, userName);

			nlohmann::json jsonResponse;
			if (m_db->CheckUserData(userName))
			{
				jsonResponse["Option"] = "Allow";
				std::string supportedFormats;
				m_db->GetUserBackupRules(userName, supportedFormats);
				jsonResponse.update( nlohmann::json::parse( supportedFormats));
			}
			else
				jsonResponse["Option"] = "NotAllow";

			response = "HTTP/1.1 200 OK\r\nContent - Length: " +
				std::to_string(jsonResponse.dump().length()) + "\r\n" +
				"Content-Type: application/json; charset=utf-8\r\n" +
				g_corp_access_all +
				"\r\n" + jsonResponse.dump();
		}

		else if (requestLine.find("POST /ServiceUserConfig") != std::string::npos)
		{
			std::string body;
			GetBody(body);

			std::string email, password, userName;

			PostDataParser::CheckLogin(body, email, password);

			nlohmann::json jsonResponse;
			if (m_db->CheckUserData(email, password, userName))
			{
				jsonResponse["Option"] = "Allow";
				std::string supportedFormatsBackup;
				m_db->GetUserBackupRules(userName, supportedFormatsBackup);
				jsonResponse.update(nlohmann::json::parse(supportedFormatsBackup));
				std::string supportedFormatsBlock;
				m_db->GetUserBlockRules(userName, supportedFormatsBlock);
				jsonResponse["config_block"] = supportedFormatsBlock;
			}
			else
				jsonResponse["Option"] = "NotAllow";

			response = "HTTP/1.1 200 OK\r\nContent - Length: " +
				std::to_string(jsonResponse.dump().length()) + "\r\n" +
				"Content-Type: application/json; charset=utf-8\r\n" +
				g_corp_access_all +
				"\r\n" + jsonResponse.dump() + "\r\n\r\n";
		}

		else if (requestLine.find("POST /SupportedBlockRules") != std::string::npos)
		{
			std::string body;
			GetBody(body);
			std::string userName;

			PostDataParser::CheckUserName(body, userName);

			nlohmann::json jsonResponse;
			if (m_db->CheckUserData(userName))
			{
				jsonResponse["Option"] = "Allow";
				std::string supportedFormats;
				m_db->GetUserBlockRules(userName, supportedFormats);
				jsonResponse.update(nlohmann::json::parse(supportedFormats));
			}
			else
				jsonResponse["Option"] = "NotAllow";

			response = "HTTP/1.1 200 OK\r\nContent - Length: " +
				std::to_string(jsonResponse.dump().length()) + "\r\n" +
				"Content-Type: application/json; charset=utf-8\r\n" +
				g_corp_access_all +
				"\r\n" + jsonResponse.dump() + "\r\n\r\n";
		}

		else if (requestLine.find("GET /HelloWorld") != std::string::npos)
		{
			std::string response_body = "HelloWorld!!!";
			response = "HTTP/1.1 200 OK\r\nContent - Length: " +
				std::to_string(response_body.length()) + "\r\n" +
				"Content-Type: application/json; charset=utf-8\r\n" +
				g_corp_access_all +
				"\r\n" + response_body + "\r\n\r\n";
		}

		else if (requestLine.find("POST /SaveConfig") != std::string::npos)
		{
			std::string body;
			GetBody(body);
			std::string userName;
			nlohmann::json jsonConfig;
			PostDataParser::CheckUserName(body, userName);
			PostDataParser::ParseConfig(body, jsonConfig);
			nlohmann::json jsonResponse;
			if (m_db->CheckUserData(userName))
			{
				jsonResponse["Option"] = "Allow";
				if (m_db->SaveConfig(userName, jsonConfig))
					jsonResponse["SaveResult"] = "Successfully";
				else
					jsonResponse["SaveResult"] = "UnSuccessfully";
			}
			else
				jsonResponse["Option"] = "NotAllow";

			response = "HTTP/1.1 200 OK\r\nContent - Length: " +
				std::to_string(jsonResponse.dump().length()) + "\r\n" +
				"Content-Type: application/json; charset=utf-8\r\n" +
				g_corp_access_all +
				"\r\n" + jsonResponse.dump() + "\r\n\r\n";
		}

		else
			response = g_corp_header +
					   "Content - Length: " + std::to_string(strlen("Page not found\r\n")) +
					   "\r\n\r\n"
					   "Page not found\r\n"
					   "\r\n";
	}
	catch (const std::exception&)
	{
		SendResponse(response);
	}
	std::cout << "Responce:\n" << response << std::endl;
	SendResponse(response);
}

void RequestHandler::HadlerAfterWrite(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	m_socket->close();
}

void RequestHandler::SendResponse(const std::string& response)
{
	std::ostream os(&m_response);
	os << response;
	boost::asio::async_write(*m_socket, m_response, boost::bind(&RequestHandler::HadlerAfterWrite, shared_from_this(), _1, _2));
}

void RequestHandler::GetBody(std::string& body)
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
	std::getline(requestStream, body);
}

RequestHandler::~RequestHandler() = default;
