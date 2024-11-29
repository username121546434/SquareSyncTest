#include <iostream>
#include <map>
#include <thread>
#include <chrono>
#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include "Server.h"

bool quit = false;

int main(int argv, char **argc) {
	int port {std::stoi(argc[1])};

	SteamDatagramErrMsg err;
	if (!GameNetworkingSockets_Init(nullptr, err)) {
		std::cerr << "GameNetworkingSockets_Init failed code " << err << std::endl;
		return 1;
	}
	{
		Server server {};
		server.run(port);
	}
	GameNetworkingSockets_Kill();
}

