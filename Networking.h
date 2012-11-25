#ifndef KAHAWAI_NETWORKING
#define KAHAWAI_NETWORKING
#include "kahawaiBase.h"


bool InitNetworking();
SOCKET CreateSocketToServer(char* serverAddress, int port);
SOCKET CreateSocketToClient(int host_port);

#endif