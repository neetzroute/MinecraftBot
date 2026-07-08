#include "../include/ResourcePackHandler.h"

#include <thread>
#include <chrono>

#include "botcraft/Game/ConnectionClient.hpp"
#include "botcraft/Network/NetworkManager.hpp"
#include "botcraft/Utilities/Logger.hpp" // Добавлено для логирования

#if PROTOCOL_VERSION >= 765 /* 1.20.3+ */
#include "protocolCraft/Packets/Configuration/Serverbound/ServerboundResourcePackPacket.hpp"
#include "protocolCraft/Packets/Game/Serverbound/ServerboundResourcePackPacket.hpp"
#endif

using namespace Botcraft;
using namespace ProtocolCraft;

ResourcePackHandler::ResourcePackHandler(ConnectionClient* client)
    : m_client(client)
{
}

#if PROTOCOL_VERSION >= 765 /* 1.20.3+ */
void ResourcePackHandler::AcceptResourcePack(const UUID& uuid, bool configuration)
{
    constexpr int successfully_loaded = 0;
    constexpr int accepted = 3;
    constexpr int downloaded = 4;

    if (!m_client) return;
    auto network_manager = m_client->GetNetworkManager();
    if (!network_manager) return;

    LOG_INFO("Server requested a resource pack. Action: Sending 'Accepted' status...");

    if (configuration) {
        auto accepted_packet = std::make_shared<ServerboundResourcePackConfigurationPacket>();
        accepted_packet->SetUuid(uuid);
        accepted_packet->SetAction(accepted);
        network_manager->Send(accepted_packet);

        auto downloaded_packet = std::make_shared<ServerboundResourcePackConfigurationPacket>();
        downloaded_packet->SetUuid(uuid);
        downloaded_packet->SetAction(downloaded);
        network_manager->Send(downloaded_packet);

        auto loaded_packet = std::make_shared<ServerboundResourcePackConfigurationPacket>();
        loaded_packet->SetUuid(uuid);
        loaded_packet->SetAction(successfully_loaded);
        network_manager->Send(loaded_packet);

        LOG_INFO("Configuration resource pack successfully accepted and simulated as loaded.");
        return;
    }

    auto accepted_packet = std::make_shared<ServerboundResourcePackPacket>();
    accepted_packet->SetUuid(uuid);
    accepted_packet->SetAction(accepted);
    network_manager->Send(accepted_packet);

    std::thread([network_manager, uuid]() {
        LOG_INFO("Downloading game resource pack...");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        auto downloaded_packet = std::make_shared<ServerboundResourcePackPacket>();
        downloaded_packet->SetUuid(uuid);
        downloaded_packet->SetAction(downloaded);
        network_manager->Send(downloaded_packet);

        LOG_INFO("Game resource pack downloaded. Simulating installation...");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        auto loaded_packet = std::make_shared<ServerboundResourcePackPacket>();
        loaded_packet->SetUuid(uuid);
        loaded_packet->SetAction(successfully_loaded);
        network_manager->Send(loaded_packet);

        LOG_INFO("Game resource pack successfully applied!");
    }).detach();
}

void ResourcePackHandler::Handle(ClientboundResourcePackPushPacket& msg)
{
    AcceptResourcePack(msg.GetUuid(), false);
}

void ResourcePackHandler::Handle(ClientboundResourcePackPushConfigurationPacket& msg)
{
    AcceptResourcePack(msg.GetUuid(), true);
}
#else
void ResourcePackHandler::Handle(ClientboundResourcePackPacket& msg)
{
    if (!m_client) return;
    auto network_manager = m_client->GetNetworkManager();
    if (!network_manager) return;

    LOG_INFO("Legacy server requested a resource pack. Action: Sending 'Accepted' status...");

    auto accepted_packet = std::make_shared<ServerboundResourcePackPacket>();
    accepted_packet->SetAction(3);
    network_manager->Send(accepted_packet);

    LOG_INFO("Simulating legacy resource pack download and load...");
    auto loaded_packet = std::make_shared<ServerboundResourcePackPacket>();
    loaded_packet->SetAction(0);
    network_manager->Send(loaded_packet);

    LOG_INFO("Legacy resource pack successfully applied!");
}
#endif