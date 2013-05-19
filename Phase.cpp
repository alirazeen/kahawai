#include "Phase.h"

Phase::Phase(char* str)
	: _ordinal(_nextOrdinal++),
	_str(str)
{
	
}

// Initialize our phases

int Phase::_nextOrdinal = 0;

const Phase* Phase::GAME_BEGIN = new Phase("GAME_BEGIN");
const Phase* Phase::GAME_END = new Phase("GAME_END");

const Phase* Phase::CAPTURE_BEGIN = new Phase("CAPTURE_BEGIN");
const Phase* Phase::CAPTURE_END = new Phase("CAPTURE_END");

const Phase* Phase::KAHAWAI_BEGIN = new Phase("KAHAWAI_BEGIN");
const Phase* Phase::KAHAWAI_END = new Phase("KAHAWAI_END");

const Phase* Phase::TRANSFORM_BEGIN = new Phase("TRANSFORM_BEGIN");
const Phase* Phase::TRANSFORM_END = new Phase("TRANSFORM_END");

const Phase* Phase::ENCODE_BEGIN = new Phase("ENCODE_BEGIN");
const Phase* Phase::ENCODE_END = new Phase("ENCODE_END");

const Phase* Phase::DECODE_BEGIN = new Phase("DECODE_BEGIN");
const Phase* Phase::DECODE_END = new Phase("DECODE_END");

const Phase* Phase::SEND_BEGIN = new Phase("SEND_BEGIN");
const Phase* Phase::SEND_END = new Phase("SEND_END");

const Phase* Phase::SHOW_BEGIN = new Phase("SHOW_BEGIN");
const Phase* Phase::SHOW_END = new Phase("SHOW_END");

const Phase* Phase::IFRAME_CLIENT_ENCODE_BEGIN = new Phase("IFRAME_CLIENT_ENCODE_BEGIN");
const Phase* Phase::IFRAME_CLIENT_ENCODE_END = new Phase("IFRAME_CLIENT_ENCODE_END");

const Phase* Phase::IFRAME_CLIENT_MULTIPLEX_BEGIN = new Phase("IFRAME_CLIENT_MULTIPLEX_BEGIN");
const Phase* Phase::IFRAME_CLIENT_MULTIPLEX_END = new Phase("IFRAME_CLIENT_MULTIPLEX_END");

const Phase* Phase::INPUT_CLIENT_RECEIVE = new Phase("INPUT_CLIENT_RECEIVE");
const Phase* Phase::INPUT_CLIENT_SEND_BEGIN = new Phase("INPUT_CLIENT_SEND_BEGIN");
const Phase* Phase::INPUT_CLIENT_SEND_END = new Phase("INPUT_CLIENT_SEND_END");

const Phase* Phase::INPUT_SERVER_RECEIVE_BEGIN = new Phase("INPUT_SERVER_RECEIVE_BEGIN");
const Phase* Phase::INPUT_SERVER_RECEIVE_END = new Phase("INPUT_SERVER_RECEIVE_END");
const Phase* Phase::INPUT_SERVER_SEND = new Phase("INPUT_SERVER_SEND");