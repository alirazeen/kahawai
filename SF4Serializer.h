#pragma once
#include "InputSerializer.h"
#include "SF4cmd.h"


class SF4Serializer:
	public InputSerializer
{

public:
	char*			Serialize(void* nativeInput, int* length);
	void*			Deserialize(char* serializedInput);
	void*			GetEmptyCommand();
	size_t			GetCommandSize();

private:
	sf4cmd_t	_empty;

public:
	SF4Serializer(void);
	~SF4Serializer(void);

};