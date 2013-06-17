#include "Measurement.h"
#include "utils.h"

Measurement::Measurement(char* filename, char* headerFmt, ...)
{
	InitMeasurementFile(filename);
	InitializeCriticalSection(&_recordsCS);

	int headerLen=2048;
	char* header = new char[headerLen];
	header[0] = '#';

	va_list headerArgs;
	va_start(headerArgs, headerFmt);
	vsprintf_s(header+1, headerLen-1, headerFmt, headerArgs); //headerLen-1 because the first character is reserved

	WriteMeasurementLine(header);

	delete[] header;
}

Measurement::~Measurement()
{
	_measurementFile.close();
}

void Measurement::AddPhase(const Phase* phase, int frameNum, char* extraFmt, ...)
{
	DWORD time = timeGetTime();
	
	PhaseRecord* record = new PhaseRecord();
	record->phase = phase;
	record->frameNum = frameNum;
	record->time = time;

	//Copy the extra phase tag information over
	int extraLen = 2048; //XXX: More sloppy programming? How long should this really be?
	record->extra = new char[extraLen]; 
	va_list extraArgs;
	va_start(extraArgs, extraFmt);
	vsprintf_s(record->extra, extraLen, extraFmt, extraArgs);
	
	EnterCriticalSection(&_recordsCS);
	_phaseRecords.push(record);

	if (_phaseRecords.size() > MAX_RECORDS_BEFORE_FLUSH)
		Flush();

	LeaveCriticalSection(&_recordsCS);
}

void Measurement::Flush()
{
	EnterCriticalSection(&_recordsCS);
	{
		//XXX: Sloppy programming? We just need a ``big enough''
		//char buffer
		char line[4096];

		while(!_phaseRecords.empty())
		{
			PhaseRecord* record = _phaseRecords.front();
			_phaseRecords.pop();

			strcpy(line, "");
			sprintf_s(line, "%s, %d, %s, %ld\n", record->phase->_str, record->frameNum, record->extra, record->time);
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