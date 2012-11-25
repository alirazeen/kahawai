#pragma once
#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "InputSerializer.h"

 //The max size in bytes of each command representation
#define KAHAWAI_INPUT_COMMAND_BUFFER 1000


class InputHandlerServer
{
public:
	InputHandlerServer(int port, char* gameName);
	~InputHandlerServer(void);

	bool					Connect();
	void*					ReceiveCommand(int* length);
	void*					GetEmptyCommand();		

private:
	//Communication
	SOCKET					_inputSocket;
	int						_port;

	//Serialization
	InputSerializer*		_serializer;
	char*					_incomingCommand;
};

#endif