#pragma once
#include "InputSerializer.h"
#ifndef BIT
#define BIT( num )				( 1 << ( num ) )
#endif
#include "..\framework\UsercmdGen.h"

class Doom3Serializer :
	public InputSerializer
{

public:
	char*			Serialize(void* nativeInput, int* length);
	void*			Deserialize(char* serializedInput);
	void*			GetEmptyCommand();
	size_t			GetCommandSize();

private:
	usercmd_t	_empty;
	//Persistent command state (Doom3)
	int				_flags;
	int				_impulse;

public:
	Doom3Serializer(void);
	~Doom3Serializer(void);

};

