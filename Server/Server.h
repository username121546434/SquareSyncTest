#pragma once
#include <vector>
#include <steam/steamnetworkingsockets.h>
#include "Serialization.h"
#include <map>
#include <string>
#include <capnp/orphan.h>

class Server {
    static size_t players_connected;
    static Server *instance;

    HSteamListenSocket listen_socket;
    HSteamNetPollGroup poll_group;
    ISteamNetworkingSockets *networking_sockets;

    ::capnp::MallocMessageBuilder msg_for_orphanage;
    capnp::Orphanage orphanage;
    std::map<HSteamNetConnection, capnp::Orphan<Square>> clients;
    bool quit;
public:
    Server();

    SteamNetworkingIPAddr server_addr;
    void run(int port);

    void poll_incoming_messages();
    void on_connection_status_changed(SteamNetConnectionStatusChangedCallback_t *info);

    void create_new_player(HSteamNetConnection conn);

    void handle_player_input(Input::Reader input, HSteamNetConnection player);

    void send_event(Event::Type::Which type, std::string error = "");
    void send_disconnect_event();
    void send_connect_event();
    void send_appstate_update();

    void send_data(const kj::ArrayPtr<const char> data);
    
    static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *info) {
        instance->on_connection_status_changed(info);
    }

    void PollConnectionStateChanges() {
        instance = this;
        networking_sockets->RunCallbacks();
    }
};

