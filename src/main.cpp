#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

#include "../include/ChatListener.h"
#include "botcraft/Utilities/Logger.hpp"

int main()
{
    try
    {
        Botcraft::Logger::GetInstance().SetLogLevel(Botcraft::LogLevel::Info);
        Botcraft::Logger::GetInstance().SetFilename("");
        Botcraft::Logger::GetInstance().RegisterThread("main");

        ChatListener client;

        LOG_INFO("Starting connection process");
        client.Connect("play2.funtime.su", "Aaagawg"); // Ник в коде! Какой ужас! Какой я плохой программист!

        LOG_INFO("Connected to server!");

        while (true)
        {
            std::string line;
            std::cin >> line;

            std::cout << "Серьезно, ты пишешь \'" << line << "\'? Иди отсюда, малолетний ребенок!" << std::endl;
        }

        client.Disconnect();
        return 0;
    }
    catch (std::exception& e)
    {
        LOG_FATAL("Exception: " << e.what());
        return 1;
    }
    catch (...)
    {
        LOG_FATAL("Unknown exception");
        return 2;
    }
}
