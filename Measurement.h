#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <windows.h>
#include <queue>
#include <iostream>
#include <fstream>
#include "Phase.h"

using namespace std;

#define MAX_RECORDS_BEFORE_FLUSH 100
#define FRAME_NUM_NOT_APPLICABLE -1

class Measurement
{
public: 

	Measurement(char* filename, char* headerFmt="", ...);
	~Measurement();

	void AddPhase(const Phase* phase, int frameNum, char* extraFmt = "", ...);
	void AddPhase(char* message, int frameNum);
	
	void InputSampled(int inputNum);
	void InputProcessed(int inputNum, int frameNum);
	void InputClientSendBegin(int inputNum);
	void InputClientSendEnd(int inputNum);
	void InputServerReceiveBegin(int inputNum);
	void InputServerReceiveEnd(int inputNum);

	void Flush();

private:

	struct PhaseRecord
	{
		const Phase* phase;
		char* message;
		boolean newedMessage;
		int frameNum;
		DWORD time;
		char* extra;

		PhaseRecord()
		{
			extra = NULL;
			newedMessage = false;
		}

		~PhaseRecord()
		{
			if (extra != NULL)
			{
				delete[] extra;
				extra = NULL;
			}

			if (newedMessage && message != NULL)
			{
				delete[] message;
				message = NULL;
			}
		}
	};

	queue<PhaseRecord*> _phaseRecords;
	CRITICAL_SECTION _recordsCS;

	ofstream _measurementFile;

	void InitMeasurementFile(char* filename);
	void WriteMeasurementLine(char* line);


	DWORD	_kBegin; // Timestamp of KAHAWAI_BEGIN
	DWORD	_kElapsed; // KAHAWAI_END - KAHAWAI_BEGIN
	int		_numFrames;
	void	LogFPS();
};

#endif // MEASUREMENT_H