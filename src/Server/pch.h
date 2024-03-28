#pragma once

#define _WIN32_WINNT 0x0601
#define _CRT_SECURE_NO_WARNINGS
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <ostream>
#include <istream>
#include <ctime>
#include <string>
#include <boost/shared_ptr.hpp>
#include <memory>
#include <boost/enable_shared_from_this.hpp>
#include <thread>
#include <string>
#include <regex>
#include <boost/bind.hpp>

const std::string g_corp_header = "HTTP/1.1 200 OK\r\n"
								  "Access-Control-Allow-Origin: *\r\n"
								  "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
								  "Access-Control-Allow-Headers: X-PINGOTHER, Content-Type\r\n"
								  "Access-Control-Max-Age: 86400\r\n"
								  "Vary: Accept-Encoding, Origin\r\n"
								  "Keep-Alive : timeout = 2, max = 100\r\n"
								  "Connection: Keep-Alive\r\n";

const std::string g_corp_access_all =	"Access-Control-Allow-Origin: *\r\n"
										"Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
										"Access-Control-Allow-Headers: X-PINGOTHER, Content-Type\r\n"
										"Connection: Keep-Alive\r\n";
									
