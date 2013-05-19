#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <windows.h>
#include <queue>
#include <iostream>
#include <fstream>
#include "Phase.h"

using namespace std;

#define MAX_RECORDS_BEFORE_FLUSH 100

class Measurement
{
public: 

	Measurement(char* filename);
	~Measurement();

	void AddPhase(const Phase* phase, int frameNum, char* extraFmt = "", ...);
	void Flush();

private:

	struct PhaseRecord
	{
		const Phase* phase;
		int frameNum;
		DWORD time;
		char* extra;

		PhaseRecord()
		{
			extra = NULL;
		}

		~PhaseRecord()
		{
			if (extra != NULL)
			{
				delete[] extra;
				extra = NULL;
			}
		}
	};

	queue<PhaseRecord*> _phaseRecords;
	CRITICAL_SECTION _recordsCS;

	ofstream _measurementFile;

	void InitMeasurementFile(char* filename);
	void WriteMeasurementLine(char* line);

};

#endif // MEASUREMENT_H