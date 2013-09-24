#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "InputHandlerClient.h"
#include "config.h"
#include "Doom3Serializer.h"
#include "SF4Serializer.h"

//TODO: Abstract input handling to allow game specific and game agnostic input handling
//Through inheritance is possible to transform this class into "GameSpecificInputHandlerClient" and 
//code a generic DirectInput intercepting input handler


bool InputHandlerClient::Connect()
{
	//Connect to offloading server
	int attempts = 0;
	while(_inputSocket == INVALID_SOCKET && attempts < MAX_ATTEMPTS)
	{
		_inputSocket = CreateSocketToServer(_serverIP,_port);
#ifdef WIN32
		if (_inputSocket == INVALID_SOCKET)
			Sleep(RECONNECTION_WAIT);
#endif
	}


	//Spawn listening thread
	bool threadCreated = CreateKahawaiThread(AsyncInputHandler,this);

	_connected = threadCreated && (_inputSocket!=INVALID_SOCKET);

	if(_connected)
	{
		WakeConditionVariable(&_inputReadyCV);
	}

	return _connected;
}

bool InputHandlerClient::IsConnected()
{
	return _connected;
}

InputHandlerClient::InputHandlerClient(char* serverIP, int port, char* gameName)
	:_port(port),
	_serverIP(serverIP),
	_inputSocket(INVALID_SOCKET),
	_connected(false),
	_numSentInput(0)
{

	//Obtain the game specific serializer
	if(strcmp(gameName,CONFIG_DOOM3)==0)
	{
		_serializer = new Doom3Serializer();
	}
	else
	{
		_serializer = new SF4Serializer();
	}

	//Initialize Synchronization
	InitializeCriticalSection(&_inputBufferCS);
	InitializeConditionVariable(&_inputFullCV);
	InitializeConditionVariable(&_inputReadyCV);


}

void InputHandlerClient::SetMeasurement(Measurement* measurement)
{
#ifndef MEASUREMENT_OFF
	_measurement = measurement;
#endif
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
	int frameNum = -1;
	_offloading = true;

	while(_offloading)
	{
		EnterCriticalSection(&_inputBufferCS);
		{
			while(_commandQueue.empty() || !_connected)
			{
				SleepConditionVariableCS(&_inputReadyCV,&_inputBufferCS,INFINITE);
			}

			InputDescriptor* descriptor = _commandQueue.front();
			command = (char*)descriptor->_command;
			frameNum = descriptor->_frameNum;

			_commandQueue.pop();
			delete descriptor;
		}
		//Wake the game thread if it is waiting to send more frames.
		WakeConditionVariable(&_inputFullCV);
		LeaveCriticalSection(&_inputBufferCS);

#ifndef MEASUREMENT_OFF
		_measurement->InputClientSendBegin(_numSentInput);
#endif
		int result = send(_inputSocket, (char*)&frameNum, sizeof(frameNum), 0);
		if (result != SOCKET_ERROR)
		{
			result = send(_inputSocket, command, _commandLength, 0); 
			//simulate network delay
			//Sleep(200);
		}
		
		if (result == SOCKET_ERROR)
		{
			KahawaiLog("Unable to send frame number of input or the input itself to the server", KahawaiError);
		}
#ifndef MEASUREMENT_OFF
		_measurement->InputClientSendEnd(_numSentInput);
#endif
		_numSentInput++;

		delete[] command;
		command = NULL;
		frameNum = -1;
	}

}


bool InputHandlerClient::Finalize()
{
	_offloading =false;
	WakeConditionVariable(&_inputFullCV);
	WakeConditionVariable(&_inputReadyCV);

	if(_inputSocket!=INVALID_SOCKET)
	{
		closesocket(_inputSocket);
		_inputSocket = INVALID_SOCKET;
	}

	return true;

}

void InputHandlerClient::SendCommand(void* command)
{
	char* serializedCommand = _serializer->Serialize(command,&_commandLength);
	char* copyCommand = new char[_commandLength]; 
	memcpy(copyCommand,serializedCommand, _commandLength);

	EnterCriticalSection(&_inputBufferCS);
	{
		while(_commandQueue.size()>MAX_INPUT_QUEUE_LENGTH_CLIENT)//TODO: Check what is a reasonable queue length
		{
			SleepConditionVariableCS(&_inputFullCV,&_inputBufferCS,INFINITE);
		}

		InputDescriptor* descriptor = new InputDescriptor(_frameNum, copyCommand);
		_commandQueue.push(descriptor);
	}
	WakeConditionVariable(&_inputReadyCV);
	LeaveCriticalSection(&_inputBufferCS);

}

void* InputHandlerClient::GetEmptyCommand()
{
	return _serializer->GetEmptyCommand();
}

#endif