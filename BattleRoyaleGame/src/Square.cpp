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

std::mutex m;
void continuously_update_squares(std::vector<SDL_Rect> *squares, Network *net);

struct AppData {
    std::vector<SDL_Rect> squares;
    SDL_Window *window;
    SDL_Renderer *renderer;
    uint64_t last_tick;
    Network net;
    SDL_Rect sq;
    std::thread thread;

    AppData(SDL_Window *wid, SDL_Renderer *rend, std::string server, std::string port)
        : squares {}, window {wid}, renderer {rend},
        last_tick {SDL_GetTicks()}, net {server, port}, sq {net.receive_square()},
        thread {continuously_update_squares, &squares, &net} {
        std::cout << "squasqe: " << sq.x << ", " << sq.y
            << "  " << sq.w << ", " << sq.h << std::endl;
        thread.detach();
    };
};

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

void continuously_update_squares(std::vector<SDL_Rect> *squares, Network *net) {
    while (true) {
        auto vec = net->receive_squares();
        m.lock();
        *squares = vec;
        m.unlock();
    }
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
    auto port {argv[2]};
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
        int &y {ad.sq.y};
        int &x {ad.sq.x};
        if (event->key.key == SDLK_DOWN) {
            y++;
        } else if (event->key.key == SDLK_UP) {
            y--;
        } else if (event->key.key == SDLK_RIGHT) {
            x++;
        } else if (event->key.key == SDLK_LEFT) {
            x--;
        }
        ::capnp::MallocMessageBuilder message;
        Square::Builder new_sq = message.initRoot<Square>();
        new_sq.setX(x);
        new_sq.setY(y);
        new_sq.setHeight(ad.sq.h);
        new_sq.setWidth(ad.sq.w);

        kj::Array<capnp::word> dataArr = capnp::messageToFlatArray(message);
        kj::ArrayPtr<kj::byte> bytes = dataArr.asBytes();

        std::vector<uint8_t> data(bytes.begin(), bytes.end());
        ad.net.send_data(data);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppData &ad {*static_cast<AppData *>(appstate)};
    
    SDL_SetRenderDrawColor(ad.renderer, 0, 0, 0, 255);
    SDL_RenderClear(ad.renderer);

    draw_square(ad.sq, ad.renderer);
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
}

