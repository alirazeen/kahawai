#include "Measurement.h"
#include "utils.h"

Measurement::Measurement(char* filename)
{
	doomFrameNum = 0;
	kahawaiFrameNum = 0;
	InitMeasurementFile(filename);
}

void Measurement::FrameStart() 
{
	FrameRecord* frameRecord = new FrameRecord();
	frameRecord->frameNum = ++doomFrameNum;
	frameRecord->timeFrameStart = timeGetTime();

	frameRecords[doomFrameNum] = frameRecord;

	int numAhead = doomFrameNum - kahawaiFrameNum;
}

void Measurement::FrameEnd()
{
	FrameRecord* frameRecord = frameRecords[doomFrameNum];
	if (frameRecord == NULL) 
	{
		KahawaiPrintF("Frame record not found in FrameEnd()\n");
		return;
	}

	frameRecord->timeFrameEnd = timeGetTime();

}

void Measurement::InputReceived(void* inputCmd, int frameNum)
{

	InputRecord* inputRecord = new InputRecord();
	inputRecord->inputCmd = inputCmd;
	inputRecord->timeInputReceived = timeGetTime();

	InputRecord* existingRecord = inputRecords[inputCmd];
	if (existingRecord != NULL) {
		KahawaiPrintF("Input record already found on InputReceived. New Record: %p, Existing Record: %p\n", inputCmd, existingRecord);
		return;
	}

	inputRecords[inputCmd] = inputRecord;
}

void Measurement::InputSent(void* inputCmd, int frameNum)
{
	InputRecord* inputRecord = inputRecords[inputCmd];
	if (inputRecord == NULL) 
	{
		KahawaiPrintF("No input record found on InputSent\n");
		return;
	}

	FrameRecord* frameRecord = frameRecords[frameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on InputSent\n");
		return;
	}

	frameRecord->timeInputReceived = inputRecord->timeInputReceived;
	frameRecord->timeInputSent = timeGetTime();

	inputRecords.erase(inputCmd);
	delete inputRecord;
	inputRecord = NULL;
}


void Measurement::CaptureStart(int frameNum)
{
	FrameRecord* frameRecord = frameRecords[frameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on CaptureStart\n");
		return;
	}

	frameRecord->timeCaptureStart = timeGetTime();
}

void Measurement::CaptureEnd(int frameNum)
{
	FrameRecord* frameRecord = frameRecords[frameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on CaptureEnd\n");
		return;
	}

	frameRecord->timeCaptureEnd = timeGetTime();
}


void Measurement::KahawaiStart()
{
	kahawaiFrameNum += 1;
	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on KahawaiStart\n");
		return;
	}

	frameRecord->timeKahawaiStart = timeGetTime();
}

void Measurement::KahawaiEnd()
{
	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on KahawaiEnd\n");
		return;
	}

	frameRecord->timeKahawaiEnd = timeGetTime();

	char line[8192];
	sprintf_s(line,"%d, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld\n", frameRecord->frameNum,
		frameRecord->timeFrameStart, frameRecord->timeFrameEnd,
		frameRecord->timeInputReceived, frameRecord->timeInputSent,
		frameRecord->timeCaptureStart, frameRecord->timeCaptureEnd,
		frameRecord->timeKahawaiStart, frameRecord->timeKahawaiEnd,
		frameRecord->timeTransformStart, frameRecord->timeTransformEnd,
		frameRecord->timeDecodeStart, frameRecord->timeDecodeEnd,
		frameRecord->timeShowStart, frameRecord->timeShowEnd,
		frameRecord->timeEncodeStart, frameRecord->timeEncodeEnd,
		frameRecord->timeSendStart, frameRecord->timeSendEnd);
	WriteMeasurementLine(line);
}

void Measurement::EncodeStart()
{

	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on EncodeStart\n");
		return;
	}

	frameRecord->timeEncodeStart = timeGetTime();
}

void Measurement::EncodeEnd()
{

	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on EncodeEnd\n");
		return;
	}

	frameRecord->timeEncodeEnd = timeGetTime();
}

void Measurement::SendStart()
{

	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on SendStart\n");
		return;
	}

	frameRecord->timeSendStart = timeGetTime();
}

void Measurement::SendEnd()
{

	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on SendEnd\n");
		return;
	}

	frameRecord->timeSendEnd = timeGetTime();
}


void Measurement::TransformStart()
{

	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on TransformStart\n");
		return;
	}

	frameRecord->timeTransformStart = timeGetTime();
}

void Measurement::TransformEnd()
{

	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on TransformEnd\n");
		return;
	}

	frameRecord->timeTransformEnd = timeGetTime();
}



void Measurement::DecodeStart()
{
	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on DecodeStart\n");
		return;
	}

	frameRecord->timeDecodeStart = timeGetTime();
}

void Measurement::DecodeEnd()
{
	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on DecodeEnd\n");
		return;
	}

	frameRecord->timeDecodeEnd = timeGetTime();
}

void Measurement::ShowStart()
{	
	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on Showed\n");
		return;
	}

	frameRecord->timeShowStart = timeGetTime();
}

void Measurement::ShowEnd()
{	
	FrameRecord* frameRecord = frameRecords[kahawaiFrameNum];
	if (frameRecord == NULL)
	{
		KahawaiPrintF("No frame record found on Showed\n");
		return;
	}

	frameRecord->timeShowEnd = timeGetTime();
}


void Measurement::InitMeasurementFile(char* filename)
{
	measurementFile.open(filename, ios::trunc);
	WriteMeasurementLine("frameNum, timeFrameStart, timeFrameEnd, timeInputReceived, timeInputSent, timeCaptureStart, timeCaptureEnd, timeKahawaiStart, timeKahawaiEnd, timeTransformStart, timeTransformEnd, timeDecodeStart, timeDecodeEnd, timeShowStart, timeShowEnd, timeEncodeStart, timeEncodeEnd, timeSendStart, timeSendEnd\n");
}

void Measurement::WriteMeasurementLine(char* line)
{
	measurementFile << line;
	measurementFile.flush();
	// XXX: This is very sloppy programming
	// XXX: We should be closing the measurement file somewhere instead of just
	//      relying on flush() and relying on the simple fact that things will 
	//      be fine when the Doom 3 exits.
}