#include "pch.h"
#include "DataBase.h"

DataBase::DataBase()
    : m_conn("postgres://prod_dtv7_user:7e9NQLVQih3xr3tlkJLCbUFqgQxQLBJS@dpg-cnn2qpev3ddc73fmmh90-a.frankfurt-postgres.render.com/prod_dtv7")
{

}

bool DataBase::CheckUserData(const std::string& email, const std::string& password)
{

    if (m_conn.is_open())
    {
        std::cout << "Connected to database successfully!" << std::endl;

        // Perform database operations here...

        pqxx::work txn(m_conn);

        // Execute SELECT query to retrieve all rows from a table
        pqxx::result result = txn.exec("SELECT email, password FROM public.\"Users\"");

        // Iterate over the result set
        for (const auto& row : result)
        {
            if (email == row.at("email").c_str() 
                && password == row.at("password").c_str())
            {
                txn.commit();
                return true;
            }
        }

        // Commit the transaction
        txn.commit();

    }
    else
    {
        std::cerr << "Failed to connect to database" << std::endl;
    }

    return false;
}
