#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "InputHandlerClient.h"
#include "config.h"
#include "Doom3Serializer.h"

//TODO: Abstract input handling to allow game specific and game agnostic input handling
//Through inheritance is possible to transform this class into "GameSpecificInputHandlerClient" and 
//code a generic DirectInput intercepting input handler


bool InputHandlerClient::Connect()
{
	//Connect to offloading server
	_inputSocket = CreateSocketToServer(_serverIP,_port);

	//Spawn listening thread
	bool threadCreated = CreateKahawaiThread(AsyncInputHandler,this);

	_connected = threadCreated && (_inputSocket!=INVALID_SOCKET);

	if(_connected)
	{
		WakeConditionVariable(&_inputReadyCV);
	}

	return _connected;
}

InputHandlerClient::InputHandlerClient(char* serverIP, int port, char* gameName)
	:_port(port),
	_serverIP(serverIP),
	_inputSocket(0),
	_connected(false)
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

	//Initialize Synchronization
	InitializeCriticalSection(&_inputBufferCS);
	InitializeConditionVariable(&_inputFullCV);
	InitializeConditionVariable(&_inputReadyCV);


}


InputHandlerClient::~InputHandlerClient(void)
{
}

DWORD WINAPI InputHandlerClient::AsyncInputHandler(void* Param)
{
	InputHandlerClient* This = (InputHandlerClient*) Param;	
	This->SendCommandsAsync();

	return 0;

}

void InputHandlerClient::SendCommandsAsync()
{
	char* command = NULL;

	//TODO: Link to kahawai offloading state
	while(true)
	{
		EnterCriticalSection(&_inputBufferCS);
		{
			while(_commandQueue.empty() || !_connected)
			{
				SleepConditionVariableCS(&_inputReadyCV,&_inputBufferCS,INFINITE);
			}

			command = _commandQueue.front();			
			_commandQueue.pop();

		}
		//Wake the game thread if it is waiting to send more frames.
		WakeConditionVariable(&_inputFullCV);
		LeaveCriticalSection(&_inputBufferCS);

		//Send the command to the server
		if(send(_inputSocket,command,_commandLength,0)==SOCKET_ERROR)
		{
			KahawaiLog("Unable to send input to server", KahawaiError);

		}

		delete[] command;
		command = NULL;

	}

}


void InputHandlerClient::SendCommand(void* command)
{
	char* serializedCommand = _serializer->Serialize(command,&_commandLength);
	char* copyCommand = new char[_commandLength]; 
	memcpy(copyCommand,serializedCommand, _commandLength);

	EnterCriticalSection(&_inputBufferCS);
	{
		while(_commandQueue.size()>3)//TODO: Check what is a reasonable queue length
		{
			SleepConditionVariableCS(&_inputFullCV,&_inputBufferCS,INFINITE);
		}
		_commandQueue.push(copyCommand);
	}
	WakeConditionVariable(&_inputReadyCV);
	LeaveCriticalSection(&_inputBufferCS);

}

void* InputHandlerClient::GetEmptyCommand()
{
	return _serializer->GetEmptyCommand();
}

#endif