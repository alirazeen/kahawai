#include "kahawaiBase.h"
#ifdef KAHAWAI

#include "InputHandlerServer.h"
#include "Doom3Serializer.h"
#include "config.h"



void* InputHandlerServer::ReceiveCommand(int* length)
{
	int receivedBytes = 0;
	size_t commandSize = 0;

	if(recv(_inputSocket,(char*)&commandSize,sizeof(size_t),0) == SOCKET_ERROR)
	{
		KahawaiLog("Failed to receive command from client", KahawaiError);
		return NULL;
	}
	
	int burst = 1;
	while(receivedBytes < commandSize && burst > 0)
	{
		burst = recv(_inputSocket,_incomingCommand,commandSize,0);
		receivedBytes+=burst;
	}

	if(receivedBytes < commandSize)
	{
		KahawaiLog("Didn't receive all in a single burst.", KahawaiError);
		return NULL;
	}

	*length = commandSize;

	return _serializer->Deserialize(_incomingCommand);

}

bool InputHandlerServer::Connect()
{

	//Connect to offloading client
	_inputSocket = CreateSocketToClient(_port);



	return _inputSocket != INVALID_SOCKET;
}

void* InputHandlerServer::GetEmptyCommand()
{
	return _serializer->GetEmptyCommand();
}


InputHandlerServer::InputHandlerServer(int port, char* gameName)
	:_port(port)
{
	//Obtain the game specific serializer
	if(strcmp(gameName,CONFIG_DOOM3)==0)
	{
		_serializer = new Doom3Serializer();
	}
	else
	{
		KahawaiLog("Input is only supported for the Doom 3 Engine", KahawaiError);
	}

	//Initialize command buffer
	_incomingCommand = new char[KAHAWAI_INPUT_COMMAND_BUFFER];

}




InputHandlerServer::~InputHandlerServer(void)
{
	if(_serializer!=NULL)
		delete _serializer;
	_serializer = NULL;

	if(_incomingCommand!=NULL)
		delete[] _incomingCommand;
	_incomingCommand = NULL;
}

#endif