#include "Measurement.h"
#include "utils.h"

Measurement::Measurement(char* filename)
{
	InitMeasurementFile(filename);
	InitializeCriticalSection(&_recordsCS);
}

Measurement::~Measurement()
{
	_measurementFile.close();
}

void Measurement::AddPhase(const Phase* phase, int frameNum)
{
	DWORD time = timeGetTime();
	
	PhaseRecord* record = new PhaseRecord();
	record->phase = phase;
	record->frameNum = frameNum;
	record->time = time;

	EnterCriticalSection(&_recordsCS);
	_phaseRecords.push(record);
	LeaveCriticalSection(&_recordsCS);
}

void Measurement::Flush()
{
	EnterCriticalSection(&_recordsCS);
	{
		//Sloppy programming? We just need a ``big enough''
		//char buffer
		char line[2048];
		while(!_phaseRecords.empty())
		{
			PhaseRecord* record = _phaseRecords.front();
			_phaseRecords.pop();

			strcpy(line, "");
			sprintf_s(line, "%s, %d, %ld\n", record->phase->_str, record->frameNum, record->time);
			WriteMeasurementLine(line);

			delete record;
		}

		_measurementFile.flush();
	}
	LeaveCriticalSection(&_recordsCS);
}

void Measurement::InitMeasurementFile(char* filename)
{
	_measurementFile.open(filename, ios::trunc);
}

void Measurement::WriteMeasurementLine(char* line)
{
	_measurementFile << line;
}