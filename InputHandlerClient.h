#pragma once
#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "InputSerializer.h"

class InputHandlerClient
{
public:
	InputHandlerClient(char* serverIP, int port, char* gameName);
	~InputHandlerClient(void);

	static DWORD WINAPI		AsyncInputHandler(void* Param);
	void					SendCommand(void* command);
	void*					GetEmptyCommand();	
	bool					Connect();

protected:
	void					SendCommandsAsync();

	//Communication
	SOCKET					_inputSocket;
	char*					_serverIP;
	int						_port;
	bool					_connected;

	//Network Input queue synchronization
	CONDITION_VARIABLE		_inputFullCV;
	CONDITION_VARIABLE		_inputReadyCV;
	CRITICAL_SECTION		_inputBufferCS;
	bool					_inputQueued;

	//Serialization
	InputSerializer*		_serializer;
	char*					_queuedCommand;
	int						_commandLength;

};
#endif
