#define SDL_MAIN_USE_CALLBACKS 1
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "constants.h"
#include <steam/isteamnetworkingsockets.h>
#include <io.h>
#include "Network.h"
#include <mutex>
#include <thread>
#include "Serialization.h"

std::mutex m;
void continuously_update_squares(const std::vector<uint8_t> &message_from_server, std::vector<SDL_Rect> *sqs);

struct AppData {
    std::vector<SDL_Rect> squares;
    SDL_Window *window;
    SDL_Renderer *renderer;
    uint64_t last_tick;
    Network net;
    std::thread thread;

    AppData(SDL_Window *wid, SDL_Renderer *rend, std::string server, int port)
        : squares {}, window {wid}, renderer {rend},
        last_tick {SDL_GetTicks()}, net {server, port},
        thread {std::bind(&Network::run, &net, [this](const std::vector<uint8_t> &data) {return continuously_update_squares(data, &this->squares); })}
    {
        thread.detach();
    };
};

void continuously_update_squares(const std::vector<uint8_t> &message_from_server, std::vector<SDL_Rect> *sqs) {
    ::capnp::FlatArrayMessageReader reader {get_message_reader(message_from_server)};
    
    auto msg {get_message_to_client(reader)};
    auto time_now {std::chrono::system_clock::now().time_since_epoch().count()};

    // std::cout << "Latency: " << time_now - msg.getTimestamp() << std::endl;

    auto data {msg.getData()};
    switch (data.which()) {
        case MessageToClient::Data::EVENT: {
            auto event {data.getEvent().getType()};
            switch (event.which()) {
                case Event::Type::CONNECT:
                    std::cout << "Someone joined" << std::endl;
                    break;
                case Event::Type::DISCONNECT:
                    std::cout << "Someone left" << std::endl;
                    break;
                case Event::Type::ERROR:
                    std::cerr << "An error occurred" << std::endl;
                    std::cerr << event.getError().cStr() << std::endl;
                    break;
                case Event::Type::UNSET:
                    std::cerr << "The event type isn't supposed to be unset!!!" << std::endl;
            }
            break;
        }
        case MessageToClient::Data::SQUARES: {
            auto squares {data.getSquares().getPeople()};
            m.lock();
            sqs->clear();
            for (auto it {squares.begin()}; it != squares.end(); it++) {
                auto sq {*it};
                SDL_Rect x {
                    static_cast<int>(sq.getX()), static_cast<int>(sq.getY()),
                    static_cast<int>(sq.getWidth()), static_cast<int>(sq.getHeight())
                };
                sqs->push_back(x);
            }
            m.unlock();
        }
    }
}


void draw_square(const SDL_Rect &sq, SDL_Renderer *renderer) {
    // std::cout << "dsqawing squasqe: " << sq.x << ", " << sq.y
    //     << "  " << sq.w << ", " << sq.h << std::endl;
    SDL_FRect fr;
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1);
    SDL_RectToFRect(&sq, &fr);
    SDL_RenderFillRect(renderer, &fr);
}

void draw_square(const Square::Reader &sq, SDL_Renderer *renderer) {
    SDL_Rect r {sq.getX(), sq.getY(), sq.getWidth(), sq.getHeight()};
    return draw_square(r, renderer);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Test", window_width, window_height, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    
    SteamDatagramErrMsg err;
    if (!GameNetworkingSockets_Init(nullptr, err)) {
        std::cerr << "GameNetworkingSockets_Init failed: " << err << std::endl;
        return SDL_APP_FAILURE;
    }

    std::cout << argv[0] << std::endl;
    std::string server {argv[1]};
    auto port {std::stoi(argv[2])};
    std::cout << server << std::endl;
    std::cout << port << std::endl;
    
    AppData *ad = new AppData(window, renderer, server, port);
    *appstate = ad;
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    AppData &ad {*static_cast<AppData *>(appstate)};
    
    if (event->type == SDL_EVENT_KEY_DOWN) {
        InputType inp;
        if (event->key.key == SDLK_DOWN) {
            inp.keypress = Input::KeyboardInput::DOWN;
        } else if (event->key.key == SDLK_UP) {
            inp.keypress = Input::KeyboardInput::UP;
        } else if (event->key.key == SDLK_RIGHT) {
            inp.keypress = Input::KeyboardInput::RIGHT;
        } else if (event->key.key == SDLK_LEFT) {
            inp.keypress = Input::KeyboardInput::LEFT;
        } else {
            return SDL_APP_CONTINUE;
        }
        ::capnp::MallocMessageBuilder message;
        auto data {create_input(message, Input::Type::Which::KEYPRESS, inp)};

        ad.net.send_data(data);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppData &ad {*static_cast<AppData *>(appstate)};
    
    SDL_SetRenderDrawColor(ad.renderer, 0, 0, 0, 255);
    SDL_RenderClear(ad.renderer);

    m.lock();
    const auto &squares {ad.squares};
    for (const auto &s : squares) {
        draw_square(s, ad.renderer);
    }
    m.unlock();

    SDL_RenderPresent(ad.renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    AppData &ad {*static_cast<AppData *>(appstate)};
    ad.net.quit = true;
	GameNetworkingSockets_Kill();
}

