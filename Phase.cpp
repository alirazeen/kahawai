#include "Phase.h"

Phase::Phase(char* str)
	: _ordinal(_nextOrdinal++),
	_str(str)
{
	
}

// Initialize our phases

int Phase::_nextOrdinal = 0;

const Phase* Phase::GAME_START = new Phase("GAME_START");
const Phase* Phase::GAME_END = new Phase("GAME_END");

const Phase* Phase::CAPTURE_START = new Phase("CAPTURE_START");
const Phase* Phase::CAPTURE_END = new Phase("CAPTURE_END");

const Phase* Phase::KAHAWAI_START = new Phase("KAHAWAI_START");
const Phase* Phase::KAHAWAI_END = new Phase("KAHAWAI_END");

const Phase* Phase::TRANSFORM_START = new Phase("TRANSFORM_START");
const Phase* Phase::TRANSFORM_END = new Phase("TRANSFORM_END");

const Phase* Phase::ENCODE_START = new Phase("ENCODE_START");
const Phase* Phase::ENCODE_END = new Phase("ENCODE_END");

const Phase* Phase::DECODE_START = new Phase("DECODE_START");
const Phase* Phase::DECODE_END = new Phase("DECODE_END");

const Phase* Phase::SEND_START = new Phase("SEND_START");
const Phase* Phase::SEND_END = new Phase("SEND_END");

const Phase* Phase::SHOW_START = new Phase("SHOW_START");
const Phase* Phase::SHOW_END = new Phase("SHOW_END");
