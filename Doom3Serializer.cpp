#include "kahawaiBase.h"
#ifdef KAHAWAI

#include "Doom3Serializer.h"


Doom3Serializer::Doom3Serializer(void)
	:_flags(0),
	_impulse(0)
{
	_buffer = new char[sizeof(usercmd_t)];

	//Generating default empty command
	memset( &_empty, 0, sizeof( _empty ) );
	_empty.flags = _flags;
	_empty.impulse = _impulse;

}


Doom3Serializer::~Doom3Serializer(void)
{
}

void* Doom3Serializer::GetEmptyCommand()
{
	return (void*)&_empty;
}

char* Doom3Serializer::Serialize(void* nativeInput, int* length)
{
	usercmd_t* cmd = (usercmd_t*) nativeInput;
	_position=0;
	*length = sizeof(usercmd_t) + sizeof(int);


	Write( sizeof(usercmd_t) ); //the size of the command itself
	Write( cmd->gameFrame );
	Write( cmd->gameTime );
	Write( cmd->duplicateCount );
	Write( cmd->buttons );
	Write( cmd->forwardmove );
	Write( cmd->rightmove );
	Write( cmd->upmove );
	Write( cmd->angles[0] );
	Write( cmd->angles[1] );
	Write( cmd->angles[2] );
	Write( cmd->mx );
	Write( cmd->my );
	Write( cmd->impulse );
	Write( cmd->flags );
	Write( cmd->sequence );

	//save persistent attributes
	_flags = cmd->flags;
	_impulse = cmd->impulse;

	return _buffer;

}

void* Doom3Serializer::Deserialize(char* serializedInput)
{
	char* oldBuffer = _buffer;
	_buffer = serializedInput;
	_position = 0;

	KahawaiLog ("Receiving command",KahawaiDebug);
	usercmd_t* cmd = new usercmd_t();

	cmd->gameFrame = Read<int>();
	cmd->gameTime = Read<int>();
	cmd->duplicateCount = Read<int>();
	cmd->buttons =  Read<byte>();
	cmd->forwardmove = Read<signed char>();
	cmd->rightmove = Read<signed char>();
	cmd->upmove = Read<signed char>();
	cmd->angles[0] = Read<short>();
	cmd->angles[1] = Read<short>();
	cmd->angles[2] = Read<short>();
	cmd->mx = Read<short>();
	cmd-> my = Read<short>();
	cmd->impulse = Read<signed char>();
	cmd->flags = Read<byte>();
	cmd->sequence = Read<int>();

	//save persistent attributes
	_flags = cmd->flags;
	_impulse = cmd->impulse;

	_buffer = oldBuffer;

	return (void*) cmd;
}


#endif