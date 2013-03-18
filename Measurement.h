#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <windows.h>
#include <map>
#include <iostream>
#include <fstream>
using namespace std;

class Measurement
{
public: 
	Measurement(char* filename);

	// ==== Doom 3 level profiling ====
	void FrameStart();
	void FrameEnd();

	void InputReceived(void* inputCmd, int frameNum);
	void InputSent(void* inputCmd, int frameNum);

	void CaptureStart(int frameNum);
	void CaptureEnd(int frameNum);

	// ==== Kahawai level profiling =====
	void KahawaiStart();
	void KahawaiEnd();

	void TransformStart();
	void TransformEnd();

	void EncodeStart();
	void EncodeEnd();

	void SendStart();
	void SendEnd();

	void DecodeStart();
	void DecodeEnd();

	void ShowStart();
	void ShowEnd();

private:

	struct InputRecord {
		void*	inputCmd;				// The actual user command
		DWORD	timeInputReceived;		// The time this input was received from the Doom 3 engine
	};

	struct FrameRecord {
		int frameNum;					// The current frame number

		DWORD timeFrameStart;
		DWORD timeFrameEnd;

		DWORD timeInputReceived;		// The time the inputs for this frame was received by Kahawai
		DWORD timeInputSent;			// The time the inputs for this frame was sent to the engine

		DWORD timeCaptureStart;
		DWORD timeCaptureEnd;

		DWORD timeKahawaiStart;
		DWORD timeKahawaiEnd;

		DWORD timeTransformStart;
		DWORD timeTransformEnd;

		DWORD timeEncodeStart;
		DWORD timeEncodeEnd;

		DWORD timeSendStart;
		DWORD timeSendEnd;

		DWORD timeDecodeStart;
		DWORD timeDecodeEnd;

		DWORD timeShowStart;
		DWORD timeShowEnd;
	};

	int doomFrameNum;					// Number of frame worked on by the doom 3 engine
	int kahawaiFrameNum;				// Number of frame worked on by Kahawai

	map<void*, InputRecord*> inputRecords;
	map<int, FrameRecord*> frameRecords;

	ofstream measurementFile;

	void InitMeasurementFile(char* filename);
	void WriteMeasurementLine(char* line);

};

#endif // MEASUREMENT_H