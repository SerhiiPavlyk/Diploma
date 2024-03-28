#pragma once
#include <pqxx/pqxx>
class DataBase
{
public:

    DataBase();

    bool CheckUserData(const std::string& email, const std::string& password);

private:
    // Establish a connection to the PostgreSQL database
    pqxx::connection m_conn;
};

