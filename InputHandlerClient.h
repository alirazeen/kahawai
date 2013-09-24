#pragma once
#include "kahawaiBase.h"
#ifdef KAHAWAI
#include "InputSerializer.h"
#include "InputDescriptor.h"
#include <queue>
#include "Measurement.h"
using namespace std;

class InputHandlerClient
{
public:
	InputHandlerClient(char* serverIP, int port, char* gameName);
	~InputHandlerClient(void);

	static DWORD WINAPI		AsyncInputHandler(void* Param);
	void					SendCommand(void* command);
	void*					GetEmptyCommand();	
	size_t					GetCommandLength() {return _serializer->GetCommandSize();}

	bool					Connect();
	bool					Finalize();
	bool					IsConnected();

	void					SetMeasurement(Measurement* measurement);

private:
	void					SendCommandsAsync();

	//Communication
	SOCKET					_inputSocket;
	char*					_serverIP;
	int						_port;
	bool					_connected;
	bool					_offloading;

	//Network Input queue synchronization
	CONDITION_VARIABLE		_inputFullCV;
	CONDITION_VARIABLE		_inputReadyCV;
	CRITICAL_SECTION		_inputBufferCS;
	queue<InputDescriptor*>	_commandQueue;

	//Serialization
	InputSerializer*		_serializer;

	int						_commandLength;

	//Instrumentation
	Measurement*			_measurement;
	int						_frameNum;
	int						_numSentInput;
};
#endif
