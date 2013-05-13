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

const Phase* Phase::CAPTURE_START = new Phase(num++, "CAPTURE_START");
const Phase* Phase::CAPTURE_END = new Phase(num++, "CAPTURE_END");

const Phase* Phase::KAHAWAI_START = new Phase(num++, "KAHAWAI_START");
const Phase* Phase::KAHAWAI_END = new Phase(num++, "KAHAWAI_END");

const Phase* Phase::TRANSFORM_START = new Phase(num++, "TRANSFORM_START");
const Phase* Phase::TRANSFORM_END = new Phase(num++, "TRANSFORM_END");

const Phase* Phase::DECODE_START = new Phase(num++, "DECODE_START");
const Phase* Phase::DECODE_END = new Phase(num++, "DECODE_END");

const Phase* Phase::SHOW_START = new Phase(num++, "SHOW_START");
const Phase* Phase::SHOW_END = new Phase(num++, "SHOW_END");