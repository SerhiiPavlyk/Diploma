#pragma once

#include <string>
class PostDataParser 
{
public:
    static void CheckLogin(const std::string& data, std::string& login, std::string& password);
};
