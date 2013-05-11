#include "Phase.h"

Phase::Phase(int num, char* str)
	: _num(num),
	_str(str)
{
	
}

// Initialize our phases
int num=0;
const Phase* Phase::GAME_START = new Phase(num++, "GAME_START");
const Phase* Phase::GAME_END = new Phase(num++, "GAME_END");

const Phase* Phase::KAHAWAI_START = new Phase(num++, "KAHAWAI_START");
const Phase* Phase::KAHAWAI_END = new Phase(num++, "KAHAWAI_END");