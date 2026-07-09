#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

#include "../include/ChatListener.h"
#include "botcraft/Utilities/Logger.hpp"

// Вспомогательная функция для удаления пробелов по краям строки
std::string Trim(const std::string& str)
{
    const size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first)
    {
        return "";
    }
    const size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

int main()
{
    try
    {
        Botcraft::Logger::GetInstance().SetLogLevel(Botcraft::LogLevel::Info);
        Botcraft::Logger::GetInstance().SetFilename("");
        Botcraft::Logger::GetInstance().RegisterThread("main");

        ChatListener client;

        LOG_INFO("Starting connection process");
        client.Connect("play2.funtime.su", "Bob_15");

        LOG_INFO("Connected to server!");
        LOG_INFO("==========================================================================");
        LOG_INFO("Console commands available:");
        LOG_INFO("  'start <max_price>' - starts auto-buy sequence with limit (e.g. 'start 200000')");
        LOG_INFO("  'stop' - halts the auto-buy loop");
        LOG_INFO("  'test' - triggers single test check");
        LOG_INFO("  '/any_command' - sends a direct command to the Minecraft server");
        LOG_INFO("  'any message' - sends a plain message to the server chat");
        LOG_INFO("==========================================================================");

        std::string raw_input;
        while (std::getline(std::cin, raw_input))
        {
            std::string input = Trim(raw_input);
            if (input.empty()) continue;

            if (input == "exit" || input == "quit") {
                LOG_INFO("Shutting down connection...");
                break;
            }

            // ==================================================================
            // БЛОК АВТОМАТИЗАЦИИ (Ваши кастомные команды для управления ботом)
            // ==================================================================

            // Тестовая команда
            if (input == "test") {
                client.StartAhSearch();
                continue;
            }

            // Команда остановки
            if (input == "stop") {
                client.StopAutoBuy();
                continue;
            }

            // Команда старта (формат: start <макс_цена>)
            if (input.rfind("start", 0) == 0) {
                std::string price_str = Trim(input.substr(5));
                if (price_str.empty()) {
                    LOG_ERROR("Usage: start <max_price> (e.g. start 200000)");
                    continue;
                }

                try {
                    long long max_price = std::stoll(price_str);
                    client.StartAutoBuy(max_price);
                }
                catch (const std::exception& e) {
                    LOG_ERROR("Invalid price value: " << price_str << " (" << e.what() << ")");
                }
                continue;
            }

            // ==================================================================

            // Стандартное поведение
            if (input[0] == '/') {
                client.SendChatCommand(input.substr(1));
            }
            else {
                client.SendChatMessage(input);
            }
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
