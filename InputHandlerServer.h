#pragma once
#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "InputSerializer.h"
#include <queue>
#include "Measurement.h"
using namespace std;

 //The max size in bytes of each command representation
#define KAHAWAI_INPUT_COMMAND_BUFFER 1000


class InputHandlerServer
{
public:
	InputHandlerServer(int port, char* gameName);
	~InputHandlerServer(void);

	static DWORD WINAPI		AsyncInputHandler(void* Param);

	bool					Connect();
	bool					IsConnected();
	void*					ReceiveCommand();
	void*					GetEmptyCommand();		
	size_t					GetCommandLength() {return _serializer->GetCommandSize();}
	bool					Finalize();

	//Instrumentation
	void					SetMeasurement(Measurement* measurement);

private:
	void					ReceiveCommandsAsync();
	//Communication
	SOCKET					_inputSocket;
	int						_port;
	bool					_offloading;

	//Network Input queue synchronization
	CONDITION_VARIABLE		_inputFullCV;
	CONDITION_VARIABLE		_inputReadyCV;
	CRITICAL_SECTION		_inputBufferCS;
	queue<void*>			_commandQueue;
	int						_queueSize;

	//Serialization
	InputSerializer*		_serializer;

	//Instrumentation
	Measurement*			_measurement;
	int						_numReceivedInput;
};

#endif