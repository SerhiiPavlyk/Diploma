#include "pch.h"

#include "Server.h"
#include <thread>
int main(int argc, char *argv[])
{
    try {
        unsigned short port = { 0 };
        if (argc == 2)
        {
            port = static_cast<unsigned short>( std::stoi(argv[1]));
        }
        else
        {
            port = 8080;
        }
        Server server(port);
        std::thread ServerThread([&] {server.Start(); });
        std::string commandFromUser;
        while (commandFromUser != "stop")
        {
            std::cin >> commandFromUser;
            for (char &c : commandFromUser) {
                c = static_cast<char>(std::tolower(c));
            }
        }
        server.Stop();
        if (ServerThread.joinable())
        {
            ServerThread.join();
        }
    }
   
    catch (const std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    
    return 0;
}