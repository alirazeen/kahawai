#include "CircularBuffer.h"
#include <iostream>
#ifdef KAHAWAI

CircularBuffer::CircularBuffer(int maxBufferSize, int numBuffers)
	:_maxBufferSize(maxBufferSize),
	_numBuffers(numBuffers),
	_nextPutIndex(0),
	_nextGetIndex(0)
{
	_buffers = new byte*[numBuffers];
	_bufferSizes = new int[numBuffers];
	_bufferStates = new State[numBuffers];

	// Pre-allocate the actual storage space
	for (int i=0; i<numBuffers; i++)
	{
		_buffers[i] = new byte[maxBufferSize];
		_bufferSizes[i] = 0;
		_bufferStates[i] = EMPTY;
	}

};

CircularBuffer::~CircularBuffer(void)
{
	if (_buffers != NULL)
	{
		for (int i=0;i<_numBuffers;i++)
		{
			if (_buffers[i] != NULL)
			{
				delete _buffers[i];
				_buffers = NULL;
			}
		}

		delete _buffers;
		_buffers = NULL;
	}

	if (_bufferSizes != NULL)
	{
		delete _bufferSizes;
		_bufferSizes = NULL;
	}


	if (_bufferStates != NULL)
	{
		delete _bufferStates;
		_bufferStates = NULL;
	}
};

bool CircularBuffer::PutStart(void** buffer, int* buffId)
{
	if (_bufferStates[_nextPutIndex] != EMPTY)
		return false;

	(*buffer) = _buffers[_nextPutIndex];
	(*buffId) = _nextPutIndex;
	_bufferStates[_nextPutIndex] = PUT_PENDING;

	_nextPutIndex++;
	_nextPutIndex = _nextPutIndex >= _numBuffers ? 0 : _nextPutIndex;

	return true;
}

void CircularBuffer::PutEnd(int buffId, int size)
{
	_bufferSizes[buffId] = size;
	_bufferStates[buffId] = FULL;
}

bool CircularBuffer::GetStart(void** buffer, int* size, int* buffId)
{
	if (_bufferStates[_nextGetIndex] != FULL)
		return false;

	(*buffer) = _buffers[_nextGetIndex];
	(*size) = _bufferSizes[_nextGetIndex];
	(*buffId) = _nextGetIndex;
	_bufferStates[_nextGetIndex] = GET_PENDING;

	_nextGetIndex++;
	_nextGetIndex = _nextGetIndex >= _numBuffers ? 0 : _nextGetIndex;

	return true;
}

void CircularBuffer::GetEnd(int buffId)
{
	_bufferStates[buffId] = EMPTY;
}

#endif //KAHAWAI