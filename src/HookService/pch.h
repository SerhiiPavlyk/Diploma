#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>


#include <tchar.h>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <locale>
#include <codecvt>
#include <map>
#include <cstdio> // For std::remove

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/iostreams/copy.hpp>