#include "Server.h"
#include <thread>
#include <assert.h>
#include <chrono>

size_t Server::players_connected = 0;
Server *Server::instance = nullptr;

Server::Server()
    : quit {false}, server_addr {}, msg_for_orphanage {},
    networking_sockets {nullptr}, listen_socket {k_HSteamListenSocket_Invalid}, poll_group {k_HSteamNetPollGroup_Invalid} {
    orphanage = msg_for_orphanage.getOrphanage();
}

void Server::run(int port) {
    networking_sockets = SteamNetworkingSockets();

    orphanage = msg_for_orphanage.getOrphanage();
    
    server_addr.Clear();
    server_addr.m_port = port;

    SteamNetworkingConfigValue_t opt;
    opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)SteamNetConnectionStatusChangedCallback);
    listen_socket = networking_sockets->CreateListenSocketIP(server_addr, 1, &opt);
    if (listen_socket == k_HSteamListenSocket_Invalid) {
        std::cerr << "Failed to listen on port " << port << std::endl;
        return;
    }
    poll_group = networking_sockets->CreatePollGroup();
    if (poll_group == k_HSteamNetPollGroup_Invalid) {
        std::cerr << "Failed to create poll group and list on port " << port << std::endl;
        return;
    }
    std::cout << "Started listening on port " << port << std::endl;

    while (!quit) {
        poll_incoming_messages();
        PollConnectionStateChanges();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Closing all connections..." << std::endl;
}

void Server::poll_incoming_messages() {
    while (!quit) {
        ISteamNetworkingMessage *incoming_msg = nullptr;
        int num_msgs {this->networking_sockets->ReceiveMessagesOnPollGroup(poll_group, &incoming_msg, 1)};
        if (num_msgs == 0)
            break;
        if (num_msgs < 0) {
            std::cerr << "FATAL Error checking for messages" << std::endl;
            std::cerr << __FILE__ << " at line " << __LINE__ << std::endl;
            quit = true;
            break;
        }
        assert(num_msgs == 1 && incoming_msg);
        auto client = clients.find(incoming_msg->m_conn);
        assert(client != clients.end());

        std::vector<uint8_t> data;
        data.reserve(incoming_msg->m_cbSize);
        for (int i {0}; i < incoming_msg->m_cbSize; i++)
            data.push_back(static_cast<uint8_t*>(incoming_msg->m_pData)[i]);
        
        Input::Reader input = get_input(data);
        handle_player_input(input, incoming_msg->m_conn);

        incoming_msg->Release();
    }
}

void Server::on_connection_status_changed(SteamNetConnectionStatusChangedCallback_t *info) {
    switch (info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_None:
            break;
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
            if (info->m_eOldState != k_ESteamNetworkingConnectionState_Connected) {
                assert(info->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
            } else {
                auto client {clients.find(info->m_hConn)};
                assert(client != clients.end());
                
                std::string debug_message;
                if (info->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
                    debug_message = "Problem detected locally";
                    std::cout << info->m_info.m_szConnectionDescription << " had a connection difficulty " << debug_message << std::endl;
                } else {
                    debug_message = "Connection closed by peer";
                    std::cout << info->m_info.m_szConnectionDescription << " has disconnected" << std::endl;
                }

                std::cout << info->m_info.m_szConnectionDescription << " " << debug_message
                          << " " << info->m_info.m_eEndReason << " " << info->m_info.m_szEndDebug << std::endl;
                clients.erase(client);
            }
            send_disconnect_event();
            networking_sockets->CloseConnection(info->m_hConn, 0, nullptr, false);
            send_appstate_update();
            break;
        }
        case k_ESteamNetworkingConnectionState_Connecting: {
            assert(clients.find(info->m_hConn) == clients.end());
            std::cout << "Connection request from " << info->m_info.m_szConnectionDescription << std::endl;

            if (networking_sockets->AcceptConnection(info->m_hConn) != k_EResultOK) {
                networking_sockets->CloseConnection(info->m_hConn, 0, nullptr, false);
                std::cout << "Can't accept connection for some reason" << std::endl;
                break;
            }
            if (!networking_sockets->SetConnectionPollGroup(info->m_hConn, poll_group)) {
                networking_sockets->CloseConnection(info->m_hConn, 0, nullptr, false);
                std::cout << "Failed to set poll group" << std::endl;
                break;
            }

            send_connect_event();
            create_new_player(info->m_hConn);
            send_appstate_update();
        }
        case k_ESteamNetworkingConnectionState_Connected:
            break;
        default:
            break;
    }
}

void Server::create_new_player(HSteamNetConnection conn) {
    clients[conn] = orphanage.newOrphan<Square>();
    auto data {clients[conn].get()};
    data.setHeight(20);
    data.setWidth(20);
    data.setX(200);
    data.setY(200);
}

void Server::handle_player_input(Input::Reader input, HSteamNetConnection player) {
    auto client = clients.find(player);
    assert(client != clients.end());
    auto sq = client->second.get();

    switch (input.getType().which()) {
        case Input::Type::Which::KEYPRESS:
            switch (input.getType().getKeypress()) {
                case Input::KeyboardInput::DOWN:
                    sq.setY(sq.getY() + 1);
                    break;
                case Input::KeyboardInput::UP:
                    sq.setY(sq.getY() - 1);
                    break;
                case Input::KeyboardInput::RIGHT:
                    sq.setX(sq.getX() + 1);
                    break;
                case Input::KeyboardInput::LEFT:
                    sq.setX(sq.getX() - 1);
                    break;
            }
            break;
        case Input::Type::Which::DISCONNECT_REQUEST:
            std::cout << "Someone sent a disconnect request" << std::endl;
            send_disconnect_event();
            networking_sockets->CloseConnection(player, 0, nullptr, false);
            send_appstate_update();
            break;
        default:
            break;
    }
}

void Server::send_event(Event::Type::Which type, std::string error) {
    ::capnp::MallocMessageBuilder msg_builder;

    auto msg = create_event_message_to_client(
        msg_builder,
        type,
        std::chrono::system_clock::now().time_since_epoch().count(),
        error
    );
    send_data(msg);
}

void Server::send_disconnect_event() {
    send_event(Event::Type::Which::DISCONNECT);
}

void Server::send_connect_event() {
    send_event(Event::Type::Which::CONNECT);
}

void Server::send_appstate_update() {
    std::vector<Square::Builder> squares;
    squares.reserve(clients.size());
    for (auto it = clients.begin(); it != clients.end(); it++)
        squares.push_back(it->second.get());

    ::capnp::MallocMessageBuilder msg_builder;
    auto msg = create_squares_message_to_client(
        msg_builder,
        squares,
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    send_data(msg);
}

void Server::send_data(const std::vector<uint8_t> &data) {
    for (auto it = clients.begin(); it != clients.end(); it++) {
        auto err = networking_sockets->SendMessageToConnection(it->first, data.data(), data.size(), k_nSteamNetworkingSend_Reliable, nullptr);
        assert(err == k_EResultOK);
        if (err != k_EResultOK)
            std::cerr << "Failed to send data, error code: " << err << std::endl;
    }
}

