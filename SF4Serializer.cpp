#include "kahawaiBase.h"
#ifdef KAHAWAI

#include "SF4Serializer.h"

SF4Serializer::SF4Serializer(void)
{
	_buffer = new char[256];

	//default empty command
	memset(&_empty, 0, sizeof(_empty));
}

SF4Serializer::~SF4Serializer(void)
{
}

void* SF4Serializer::GetEmptyCommand()
{
	return (void*)&_empty;
}

char* SF4Serializer::Serialize(void* nativeInput, int* length)
{
	sf4cmd_t* cmd = (sf4cmd_t*) nativeInput;
	_position=0;
	*length = sizeof(sf4cmd_t);
	for(int i=0; i < 256; i++) {
		Write(cmd->keys[i]);
	}
	return _buffer;
}

void* SF4Serializer::Deserialize(char* serializedInput)
{
	char* oldBuffer = _buffer;
	_buffer = serializedInput;
	_position = 0;

	sf4cmd_t* cmd = new sf4cmd_t();
	
	for(int i=0; i < 256; i++) {
		cmd->keys[i] = Read<byte>(); 
	}

	_buffer = oldBuffer;
	return (void*) cmd;
}

size_t SF4Serializer::GetCommandSize()
{
	return 256;
}


#endif