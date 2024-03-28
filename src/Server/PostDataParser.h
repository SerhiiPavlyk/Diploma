#pragma once

class PostDataParser 
{
public:
    static void CheckLogin(const std::string Data, std::string& login, std::string& password);
};
