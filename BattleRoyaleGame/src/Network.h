#pragma once
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <functional>

constexpr int max_message_len {32};
constexpr int max_message_len_squares {48};

class Network {
private:
    HSteamNetConnection connection;
    ISteamNetworkingSockets *steam_networking_sockets;
    SteamNetworkingIPAddr server;
    int port;
    static Network *callback_instance;
public:
    Network(std::string server, int port);
    void run(std::function<void(std::vector<uint8_t>)> data_func);

    void poll_incoming_messages(std::function<void(std::vector<uint8_t>)> data_func);
    void poll_connection_state_changes();

    void on_connection_status_changed(SteamNetConnectionStatusChangedCallback_t *info);
    static void connection_status_change_callback(SteamNetConnectionStatusChangedCallback_t *info);

    inline bool connected() const;

    size_t send_data(const std::vector<uint8_t> &data);
    std::vector<uint8_t> receive_data(int ammount_data = max_message_len);
    SDL_Rect receive_square();
    std::vector<SDL_Rect> receive_squares();
};

