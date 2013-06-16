#ifndef INPUT_DESCRIPTOR_H
#define INPUT_DESCRIPTOR_H

struct InputDescriptor
{
	const int _frameNum;
	const char* _command;

	InputDescriptor(int frameNum, char* command)
		: _frameNum(frameNum)
		, _command(command)
	{
		// Nothing else to do
	}
};

#endif // INPUT_DESCRIPTOR_H