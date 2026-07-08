#include "../include/ChatListener.h"
#include "../include/ResourcePackHandler.h"

#include <sstream>
#include <iterator>

#include "botcraft/Game/World/World.hpp"
#include "botcraft/Network/NetworkManager.hpp"
#include "botcraft/Utilities/Logger.hpp"

using namespace Botcraft;
using namespace ProtocolCraft;

ChatListener::ChatListener()
{
}

ChatListener::~ChatListener()
{
}

#if PROTOCOL_VERSION < 768
void ChatListener::Handle(ClientboundGameProfilePacket& msg)
#else
void ChatListener::Handle(ClientboundLoginFinishedPacket& msg)
#endif
{
    if (!resource_pack_handler) {
        LOG_INFO("Login sequence completed. Initializing ResourcePackHandler...");
        resource_pack_handler = std::make_shared<ResourcePackHandler>(this);

        auto network_manager = GetNetworkManager();
        if (network_manager) {
            network_manager->AddHandler(resource_pack_handler.get());
            LOG_INFO("ResourcePackHandler registered in NetworkManager.");
        }
    }
}

#if PROTOCOL_VERSION < 759
void ChatListener::Handle(ClientboundChatPacket& msg)
{
    ManagersClient::Handle(msg);

    std::istringstream ss{ msg.GetMessage().GetText() };
    const std::vector<std::string> splitted({ std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{} });

    ProcessChatMsg(splitted);
}
#else
void ChatListener::Handle(ClientboundPlayerChatPacket& msg)
{
#if PROTOCOL_VERSION == 759
    std::istringstream ss{ msg.GetSignedContent().GetText() };
#elif PROTOCOL_VERSION == 760
    std::istringstream ss{ msg.GetMessage_().GetSignedBody().GetContent().GetText() };
#else
    std::istringstream ss{
        msg.GetUnsignedContent().has_value()
            ? msg.GetUnsignedContent().value().GetText()
            : msg.GetBody().GetContent()
    };
#endif
    const std::vector<std::string> splitted({ std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{} });

    ProcessChatMsg(splitted);
}

void ChatListener::Handle(ClientboundSystemChatPacket& msg)
{
    std::istringstream ss{ msg.GetContent().GetText() };
    const std::vector<std::string> splitted({ std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{} });

    ProcessChatMsg(splitted);
}
#endif

#if PROTOCOL_VERSION > 760
void ChatListener::Handle(ClientboundDisguisedChatPacket& msg)
{
    std::istringstream ss{ msg.GetMessage().GetText() };
    const std::vector<std::string> splitted({ std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{} });

    ProcessChatMsg(splitted);
}
#endif

void ChatListener::Handle(ClientboundSetActionBarTextPacket& msg)
{
    std::istringstream ss{ msg.GetText().GetText() };
    const std::vector<std::string> splitted({ std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{} });

    ProcessChatMsg(splitted);
}

void ChatListener::Handle(ClientboundSetTitleTextPacket& msg)
{
    std::istringstream ss{ msg.GetText().GetText() };
    const std::vector<std::string> splitted({ std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{} });

    ProcessChatMsg(splitted);
}

void ChatListener::ProcessChatMsg(const std::vector<std::string>& splitted_msg)
{
    // 1. Восстанавливаем полную строку из вектора слов
    std::string raw_msg = "";
    for (const auto& word : splitted_msg) {
        raw_msg += word + " ";
    }
    if (!raw_msg.empty()) {
        raw_msg.pop_back();
    }

    if (raw_msg.empty()) {
        return;
    }

    // 2. Очищаем строку от цветовых кодов форматирования параграфа (§) Minecraft
    std::string clean_msg = "";
    for (size_t i = 0; i < raw_msg.length(); ++i) {
        if (i + 2 < raw_msg.length() &&
            static_cast<unsigned char>(raw_msg[i]) == 0xC2 &&
            static_cast<unsigned char>(raw_msg[i+1]) == 0xA7) {
            i += 2;
            continue;
        }
        if (static_cast<unsigned char>(raw_msg[i]) == 0xA7 && i + 1 < raw_msg.length()) {
            i += 1;
            continue;
        }
        clean_msg += raw_msg[i];
    }

    // Логируем очищенное входящее сообщение, чтобы вы видели, что происходит в чате
    LOG_INFO("[CHAT RECEIVE]: " << clean_msg);

    // 3. Стандартный авто-переход на Анархию
    std::istringstream iss{clean_msg};
    std::vector<std::string> clean_splitted({
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
    });

    for (const auto &i : clean_splitted) {
        if (i == "╚═══════════════════╝") {
            LOG_INFO("Trigger border found in chat! Sending join command: /an509");
            SendChatCommand("an509");
        }
    }
}