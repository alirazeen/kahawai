#include "Networking.h"


bool InitNetworking()
{
#ifdef _MSC_VER
	//Initialize socket support WINDOWS ONLY!
	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD( 2, 2 );
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 || ( LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 2 )) {
			KahawaiLog("Could not find useable sock dll", KahawaiError);
			return false;
	}
	return true;
#else
	KahawaiLog("Kahawai Networking hasn't been implemented for non Win32 platforms", KahawaiError)
		return false;
#endif

}

SOCKET CreateSocketToServer(char* serverAddress, int port)
{
#ifndef _MSC_VER
	KahawaiLog("Kahawai Networking hasn't been implemented for non Win32 platforms",KahawaiError)
		return INVALID_SOCKET;
#endif
	//Initialize sockets and set any options
	int * p_int ;
	SOCKET serverSocket = NULL;

	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(serverSocket == -1){
		KahawaiLog("Error initializing socket",KahawaiError);
		return INVALID_SOCKET;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if( (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(serverSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
			KahawaiLog("Unable to set socket options",KahawaiError);
			free(p_int);
			return INVALID_SOCKET;
	}
	free(p_int);

	//Connect to the server
	struct sockaddr_in my_addr;

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = inet_addr(serverAddress);

	
	int opt = 1; // '1' means TRUE and '0' means FALSE when I use it with setsockopt below
	int val = setsockopt(serverSocket,IPPROTO_TCP, TCP_NODELAY,(char*)&opt, sizeof(opt));
	if (val != 0)
	{
		KahawaiLog("Error setting TCP_NODELAY option on CreateSocketToServer. Return value: %d\n", KahawaiError, val);
	}

	if( connect(serverSocket, (struct sockaddr*)&my_addr, sizeof(my_addr)) == SOCKET_ERROR ){
		KahawaiLog("Unable to establish connection to server",KahawaiError);
		return INVALID_SOCKET;
	}

	return serverSocket;

}


/**
 * Creates a connection to the offloading client 
 * @param host_port the port to listen to
 * @return true if the connection is successful
 */
SOCKET CreateSocketToClient(int host_port)
{
#ifndef _MSC_VER
	KahawaiLog("Kahawai Networking hasn't been implemented for non Win32 platforms",KahawaiError)
	return INVALID_SOCKET;
#endif

	//Initialize sockets and set any options
	int hsock;
	int * p_int ;
	hsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(hsock == -1){
		KahawaiLog("Error initializing socket",KahawaiError);
		return INVALID_SOCKET;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;
	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
			KahawaiLog("Error setting options", KahawaiError);
			free(p_int);
			return INVALID_SOCKET;
	}
	free(p_int);

	//Bind and listen
	struct sockaddr_in my_addr;

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY ;
	
	int opt = 1; // '1' means TRUE and '0' means FALSE when I use it with setsockopt below
	int val = setsockopt(hsock,IPPROTO_TCP, TCP_NODELAY,(char*)&opt, sizeof(opt));
	if (val != 0)
	{
		KahawaiLog("Error setting TCP_NODELAY option on CreateSocketToClient. Return value: %d\n", KahawaiError, val);
	}


	if( bind( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
		KahawaiLog("Error binding to socket, make sure nothing else is listening on this port",KahawaiError);
		return INVALID_SOCKET;
	}
	if(listen( hsock, 10) == -1 ){
		KahawaiLog("Error listening",KahawaiError);
		return INVALID_SOCKET;
	}	

	//Now lets to the server stuff

	sockaddr_in sadr;
	int	addr_size = sizeof(SOCKADDR);

			return accept( hsock, (SOCKADDR*)&sadr, &addr_size);

}