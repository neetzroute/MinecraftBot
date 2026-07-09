#pragma once

#include <vector>
#include <string>
#include <memory>
#include <chrono> // Для работы со временем

#include "botcraft/Game/ManagersClient.hpp"
#include "protocolCraft/Types/Item/Slot.hpp" // Добавлено для вектора слотов

namespace ProtocolCraft {
    class ClientboundGameProfilePacket;
    class ClientboundLoginFinishedPacket;
    class ClientboundChatPacket;
    class ClientboundPlayerChatPacket;
    class ClientboundSystemChatPacket;
    class ClientboundDisguisedChatPacket;
    class ClientboundSetActionBarTextPacket;
    class ClientboundSetTitleTextPacket;

    class ClientboundOpenScreenPacket;
    class ClientboundContainerSetContentPacket;
    class ClientboundContainerSetSlotPacket;
}

class ResourcePackHandler;

class ChatListener : public Botcraft::ManagersClient
{
public:
    ChatListener();
    virtual ~ChatListener() override;

    void StartAutoBuy(long long max_price);
    void StopAutoBuy();
    void StartAhSearch();

    #if PROTOCOL_VERSION < 768
    void Handle(ProtocolCraft::ClientboundGameProfilePacket& msg) override;
    #else
    void Handle(ProtocolCraft::ClientboundLoginFinishedPacket& msg) override;
    #endif

    #if PROTOCOL_VERSION < 759
    void Handle(ProtocolCraft::ClientboundChatPacket& msg) override;
    #else
    void Handle(ProtocolCraft::ClientboundPlayerChatPacket& msg) override;
    void Handle(ProtocolCraft::ClientboundSystemChatPacket& msg) override;
    #endif

    #if PROTOCOL_VERSION > 760
    void Handle(ProtocolCraft::ClientboundDisguisedChatPacket& msg) override;
    #endif

    void Handle(ProtocolCraft::ClientboundSetActionBarTextPacket& msg) override;
    void Handle(ProtocolCraft::ClientboundSetTitleTextPacket& msg) override;

    void Handle(ProtocolCraft::ClientboundOpenScreenPacket& msg) override;
    void Handle(ProtocolCraft::ClientboundContainerSetContentPacket& msg) override;
    void Handle(ProtocolCraft::ClientboundContainerSetSlotPacket& msg) override;

private:
    void ProcessChatMsg(const std::vector<std::string>& splitted_msg);
    long long AnalyzeSlot(const ProtocolCraft::Slot& slot, int slot_index, bool& is_trophy, bool& is_confirm, bool& is_refresh);

    std::string CleanText(const std::string& raw);
    void SendClick(int container_id, int state_id, int slot_num, int button_num, int click_type);

private:
    std::shared_ptr<ResourcePackHandler> resource_pack_handler;

    // Состояния автоматизации
    bool m_waiting_for_ah;
    int m_ah_container_id;
    int m_state_id;

    // Параметры авто-скупки
    bool m_auto_buying;
    long long m_max_price;

    // Контроль меню подтверждения и кликов
    bool m_click_pending;
    bool m_buying_in_progress; // Флаг блокировки повторных кликов
    bool m_is_confirm_screen;
    bool m_confirm_clicked;
    std::chrono::steady_clock::time_point m_last_click_time;

    // Локальная копия содержимого контейнера
    std::vector<ProtocolCraft::Slot> m_container_items;
};
