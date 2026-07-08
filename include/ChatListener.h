#pragma once

#include <vector>
#include <string>
#include <memory>

#include "botcraft/Game/ManagersClient.hpp"

class ResourcePackHandler;

namespace ProtocolCraft {
    class ClientboundGameProfilePacket;
    class ClientboundLoginFinishedPacket;
    class ClientboundChatPacket;
    class ClientboundPlayerChatPacket;
    class ClientboundSystemChatPacket;
    class ClientboundDisguisedChatPacket;
    class ClientboundSetActionBarTextPacket;
    class ClientboundSetTitleTextPacket;
}

class ChatListener : public Botcraft::ManagersClient
{
public:
    ChatListener();
    virtual ~ChatListener() override;

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

private:
    void ProcessChatMsg(const std::vector<std::string>& splitted_msg);

private:
    std::shared_ptr<ResourcePackHandler> resource_pack_handler;
};