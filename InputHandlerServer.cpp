#include "kahawaiBase.h"
#ifdef KAHAWAI

#include "InputHandlerServer.h"
#include "Doom3Serializer.h"
#include "SF4Serializer.h"
#include "config.h"


void InputHandlerServer::ReceiveCommandsAsync()
{
	_offloading = true;

	while(_offloading)
	{

#ifndef MEASUREMENT_OFF
		_measurement->InputServerReceiveBegin(_numReceivedInput);
#endif
		char* command = new char[KAHAWAI_INPUT_COMMAND_BUFFER];
		
		//Receive the command from the server
		int receivedBytes = 0;
		int burst = 1;
		int size = _serializer->GetCommandSize();

		while(receivedBytes < size && burst > 0)
		{
			burst = recv(_inputSocket, command+receivedBytes, size-receivedBytes, 0);
			receivedBytes+=burst;
		}

		if(receivedBytes < size)
		{
			KahawaiLog("Didn't receive the complete command.", KahawaiError);
			continue;
		}

		EnterCriticalSection(&_inputBufferCS);
		{
			while(_commandQueue.size()>MAX_INPUT_QUEUE_LENGTH_SERVER)
			{
				SleepConditionVariableCS(&_inputFullCV,&_inputBufferCS,INFINITE);
			}

			_commandQueue.push(command);			

#ifndef MEASUREMENT_OFF
			_measurement->InputServerReceiveEnd(_numReceivedInput);
#endif
			_numReceivedInput++;
		}
		WakeConditionVariable(&_inputReadyCV);
		LeaveCriticalSection(&_inputBufferCS);

	}

}

bool InputHandlerServer::Finalize()
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

void* InputHandlerServer::ReceiveCommand()
{
	char* command = NULL;

	EnterCriticalSection(&_inputBufferCS);
	{
		while(_commandQueue.empty())
		{
			SleepConditionVariableCS(&_inputReadyCV,&_inputBufferCS,INFINITE);
		}

		command = (char*) _commandQueue.front();
		_commandQueue.pop();
	}

	WakeConditionVariable(&_inputFullCV);
	LeaveCriticalSection(&_inputBufferCS);

	void* processedCommand = _serializer->Deserialize(command);

	delete[] command;
	command = NULL;

	return processedCommand;

}

bool InputHandlerServer::Connect()
{

	//Connect to offloading client
	_inputSocket = CreateSocketToClient(_port);

	//Spawn listening thread
	bool threadCreated = CreateKahawaiThread(AsyncInputHandler,this);

	return _inputSocket != INVALID_SOCKET;
}

bool InputHandlerServer::IsConnected()
{
	return _inputSocket != INVALID_SOCKET;
}

void* InputHandlerServer::GetEmptyCommand()
{
	return _serializer->GetEmptyCommand();
}


InputHandlerServer::InputHandlerServer(int port, char* gameName)
	:_port(port),
	_numReceivedInput(0)
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

void InputHandlerServer::SetMeasurement(Measurement* measurement)
{
	_measurement = measurement;
}

DWORD WINAPI InputHandlerServer::AsyncInputHandler(void* Param)
{
	InputHandlerServer* This = (InputHandlerServer*) Param;	
	This->ReceiveCommandsAsync();

	return 0;

}


InputHandlerServer::~InputHandlerServer(void)
{
	if(_serializer!=NULL)
		delete _serializer;
	_serializer = NULL;

}

#endif