#ifdef KAHAWAI
#include "KahawaiTypes.h"

class CircularBuffer
{
public:
	CircularBuffer(int bufferSize, int numBuffers);
	~CircularBuffer(void);

	bool PutStart(void** buffer, int* buffId);
	void PutEnd(int buffId, int size);

	bool GetStart(void** buffer, int* size, int* buffId);
	void GetEnd(int buffId);

private:
	
	enum State {EMPTY, FULL, PUT_PENDING, GET_PENDING};

	int _maxBufferSize;
	int _numBuffers;

	byte**	_buffers;
	int*	_bufferSizes;
	State*	_bufferStates;

	int		_nextPutIndex;
	int		_nextGetIndex;
};
#endif //KAHAWAI