#include <iostream>
#include <string>
#include <thread>
#include <chrono>

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
        client.Connect("play2.funtime.su", "Bob_15");

        LOG_INFO("Connected to server!");
        LOG_INFO("==========================================================================");
        LOG_INFO("Console commands available:");
        LOG_INFO("  'test' - triggers automation sequence (makes bot say '!qq' in chat)");
        LOG_INFO("  '/any_command' - sends a direct command to the Minecraft server");
        LOG_INFO("  'any message' - sends a plain message to the server chat");
        LOG_INFO("==========================================================================");

        std::string input;
        while (std::getline(std::cin, input))
        {
            if (input.empty()) continue;

            if (input == "exit" || input == "quit") {
                LOG_INFO("Shutting down connection...");
                break;
            }

            // ==================================================================
            // БЛОК АВТОМАТИЗАЦИИ (Ваши кастомные команды для управления ботом)
            // ==================================================================

            // Команда бота "test"
            if (input == "test") {
                LOG_INFO("Automation sequence 'test' started.");

                // Действие 1: Бот пишет в игровой чат "!qq"
                client.SendChatMessage("!qq");

                // Действие 2: (Пример) Вы можете добавить задержку и выполнить еще действие:
                // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                // client.SendChatCommand("spawn");

                LOG_INFO("Sequence 'test' completed.");
                continue; // Пропускаем стандартную отправку этой строки в чат
            }

            // Вы можете легко добавить другие триггеры автоматизации здесь:
            /*
            if (input == "farm") {
                LOG_INFO("Starting farm automation...");
                client.SendChatMessage("Я пошел копать!");
                // Ваш список действий...
                continue;
            }
            */

            // ==================================================================

            // Стандартное поведение (если введено не кастомное слово автоматизации)
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