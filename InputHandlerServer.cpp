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
		_measurement->AddPhase(Phase::INPUT_SERVER_RECEIVE_BEGIN, FRAME_NUM_NOT_APPLICABLE, "InputNum=%d", _numReceivedInput);
#endif

		int frameNum = -1;
		char* command = new char[KAHAWAI_INPUT_COMMAND_BUFFER];
		
		//First, receive the frame number the command is intended for
		int receivedBytes = 0;
		int burst = 1;
		int size = sizeof(frameNum);

		while(receivedBytes < size && burst > 0)
		{
			burst = recv(_inputSocket, (char*)&frameNum+receivedBytes, size-receivedBytes, 0);
			receivedBytes += burst;
		}

		//Receive the command from the server
		receivedBytes = 0;
		burst = 1;
		size = _serializer->GetCommandSize();

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

			InputDescriptor* descriptor = new InputDescriptor(frameNum, command);
			_commandQueue.push(descriptor);			

#ifndef MEASUREMENT_OFF
			_measurement->AddPhase(Phase::INPUT_SERVER_RECEIVE_END, frameNum, "InputNum=%d", _numReceivedInput);
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

int InputHandlerServer::PeekCommandFrame()
{
	int frameNum = -1;

	EnterCriticalSection(&_inputBufferCS);
	{
		while(_commandQueue.empty())
		{
			SleepConditionVariableCS(&_inputReadyCV, &_inputBufferCS, INFINITE);
		}

		InputDescriptor* descriptor = _commandQueue.front();
		frameNum = descriptor->_frameNum;
	}
	LeaveCriticalSection(&_inputBufferCS);

	return frameNum;
}

void* InputHandlerServer::ReceiveCommand()
{
	int frameNum = -1;
	char* command = NULL;

	EnterCriticalSection(&_inputBufferCS);
	{
		while(_commandQueue.empty())
		{
			SleepConditionVariableCS(&_inputReadyCV,&_inputBufferCS,INFINITE);
		}

		InputDescriptor* descriptor = _commandQueue.front();
		frameNum = descriptor->_frameNum;

		command = (char*) descriptor->_command;			
		_commandQueue.pop();
		delete descriptor;
	}

	WakeConditionVariable(&_inputFullCV);
	LeaveCriticalSection(&_inputBufferCS);

	void* processedCommand = _serializer->Deserialize(command);
	
#ifndef MEASUREMENT_OFF
	_measurement->AddPhase(Phase::INPUT_SERVER_SEND, frameNum, "InputNum=%d", _numSentInput);
#endif
	_numSentInput++;

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
	_frameNum(0),
	_numReceivedInput(0),
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

void InputHandlerServer::SetMeasurement(Measurement* measurement)
{
	_measurement = measurement;
}

void InputHandlerServer::SetFrameNum(int frameNum)
{
	_frameNum = frameNum;
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