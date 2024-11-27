#include "Network.h"
#include <iostream>
#include <fstream>

Network::Network(std::string server, int port) : server {}, port {port} {
    this->server.Clear();
    this->server.ParseString(server.c_str());
    this->server.m_port = port;
}

void Network::run(std::function<void(std::vector<uint8_t>)> data_func) {
    steam_networking_sockets = SteamNetworkingSockets();
    char sz_addr[SteamNetworkingIPAddr::k_cchMaxString];
    server.ToString(sz_addr, sizeof(sz_addr), true);
    std::cout << "Connecting to: " << sz_addr << std::endl;
    SteamNetworkingConfigValue_t opt;
    opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)connection_status_change_callback);
    connection = steam_networking_sockets->ConnectByIPAddress(server, 1, &opt);
    if (connection == k_HSteamNetConnection_Invalid) {
        std::cerr << "Failed to create connection" << std::endl;
        return;
    }

    while (true) {
        poll_incoming_messages(data_func);
        poll_connection_state_changes();
    }
}

void Network::poll_incoming_messages(std::function<void(std::vector<uint8_t>)> data_func) {
    while (true) {
        ISteamNetworkingMessage *msg {nullptr};
        int num_msgs {steam_networking_sockets->ReceiveMessagesOnConnection(connection, &msg, 1)};
        if (num_msgs == 0)
            break;
        if (num_msgs < 0) {
            std::cerr << "Error checking for messages" << std::endl;
            break;
        }

        const char *raw_data = static_cast<const char *>(msg->m_pData);
        std::vector<uint8_t> data(raw_data, raw_data + msg->m_cbSize);
        data_func(data);

        msg->Release();
    }
}

void Network::poll_connection_state_changes() {
    callback_instance = this;
    steam_networking_sockets->RunCallbacks();
}

void Network::on_connection_status_changed(SteamNetConnectionStatusChangedCallback_t *info) {
    switch (info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_None:
            break;
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            if (info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting) {
                std::cerr << "Could not connect to host: " << info->m_info.m_szEndDebug << std::endl;
            } else if (info->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
                std::cerr << "Lost contact with the server: " << info->m_info.m_szEndDebug << std::endl;
            } else {
                std::cerr << "The server disconnected: " << info->m_info.m_szEndDebug << std::endl;
            }

            steam_networking_sockets->CloseConnection(info->m_hConn, 0, nullptr, false);
            connection = k_HSteamNetConnection_Invalid;
            break;
        }
        case k_ESteamNetworkingConnectionState_Connecting:
            break;
        case k_ESteamNetworkingConnectionState_Connected:
            std::cout << "Connected to server successfully!" << std::endl;
            break;
        default:
            break;
    }
}

void Network::connection_status_change_callback(SteamNetConnectionStatusChangedCallback_t *info) {
    callback_instance->on_connection_status_changed(info);
}

inline bool Network::connected() const {
    return connection != k_HSteamNetConnection_Invalid;
}

Network *Network::callback_instance = nullptr;

size_t Network::send_data(const std::vector<uint8_t> &data) {
    size_t len = data.size();
    return asio::write(socket, asio::buffer(data, len));
}

std::vector<uint8_t> Network::receive_data(int ammount_data) {
    std::vector<uint8_t> data;
    data.resize(ammount_data);
    asio::error_code error;
    size_t len = asio::read(socket, asio::buffer(data, ammount_data), error);
    std::cout << "Read " << len << " bytes of data from server" << std::endl;
    std::cout << "Error code: " << error << std::endl;

    std::ofstream f {"C:/Users/Atharv/source/repos/BattleRoyaleGame/BattleRoyaleGame/src/client/f.bin"};
    for (char c : data)
        f << c;
    f.close();

    return data;
}

SDL_Rect Network::receive_square() {
    auto sq_data {receive_data()};
}

std::vector<SDL_Rect> Network::receive_squares() {
    return vec;
}