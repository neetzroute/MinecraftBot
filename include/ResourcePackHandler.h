#pragma once

#include <memory>
#include "protocolCraft/Handler.hpp" // Легковесный базовый класс для обработки пакетов

namespace Botcraft {
    class ConnectionClient;
}

namespace ProtocolCraft {
    class ClientboundResourcePackPushPacket;
    class ClientboundResourcePackPushConfigurationPacket;
    class ClientboundResourcePackPacket;
}

// Наследуемся от чистого ProtocolCraft::Handler
class ResourcePackHandler : public ProtocolCraft::Handler
{
public:
    explicit ResourcePackHandler(Botcraft::ConnectionClient* client);
    virtual ~ResourcePackHandler() override = default;

#if PROTOCOL_VERSION >= 765 /* 1.20.3+ */
    void Handle(ProtocolCraft::ClientboundResourcePackPushPacket& msg) override;
    void Handle(ProtocolCraft::ClientboundResourcePackPushConfigurationPacket& msg) override;
#else
    void Handle(ProtocolCraft::ClientboundResourcePackPacket& msg) override;
#endif

private:
#if PROTOCOL_VERSION >= 765 /* 1.20.3+ */
    void AcceptResourcePack(const ProtocolCraft::UUID& uuid, bool configuration);
#endif

    Botcraft::ConnectionClient* m_client;
};