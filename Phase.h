#ifndef PHASE_H
#define PHASE_H

/**
 * This is an enum-style class used to designate the beginning and the end of the 
 * different phases in the lifecycle of a game running on Kahawai. This class is used
 * in conjunction with the measurement class, so that the different phases can be profiled
 * and the relationship between the various phases can be observed better.
 *
 * We have an enum class instead of just plain enums because we want type-safety, and we want
 * other useful things such as access to a string representation of the enum.
 */
class Phase
{

public:
	
	const int _ordinal; //Numerical representation of the enum
	const char* _str; //String representation of teh enum

	//Phases encountered within the game thread
	static const Phase* GAME_BEGIN;
	static const Phase* GAME_END;

	static const Phase* CAPTURE_BEGIN;
	static const Phase* CAPTURE_END;

	//Phases encountered everywhere else (i.e. outside the game thread)
	static const Phase* KAHAWAI_BEGIN;
	static const Phase* KAHAWAI_END;

	static const Phase* TRANSFORM_BEGIN;
	static const Phase* TRANSFORM_END;

	static const Phase* ENCODE_BEGIN;
	static const Phase* ENCODE_END;

	static const Phase* DECODE_BEGIN;
	static const Phase* DECODE_END;

	static const Phase* SEND_BEGIN;
	static const Phase* SEND_END;

	static const Phase* SHOW_BEGIN;
	static const Phase* SHOW_END;

	static const Phase* IFRAME_CLIENT_ENCODE_BEGIN;
	static const Phase* IFRAME_CLIENT_ENCODE_END;

	static const Phase* IFRAME_MULTIPLEX_IFRAME_BEGIN;
	static const Phase* IFRAME_MULTIPLEX_IFRAME_END;

	static const Phase* IFRAME_MULTIPLEX_PFRAME_BEGIN;
	static const Phase* IFRAME_MULTIPLEX_PFRAME_END;

	//From the client's perspective, receiving an input
	//means receiving it from the game and sending an input
	//means sending it to the cloud server
	static const Phase* INPUT_CLIENT_RECEIVE;
	static const Phase* INPUT_CLIENT_SEND_BEGIN;
	static const Phase* INPUT_CLIENT_SEND_END;

	//From the server's perspective, receiving an input
	//means receiving it from the client and sending an input
	//means sending it to the game
	static const Phase* INPUT_SERVER_RECEIVE_BEGIN;
	static const Phase* INPUT_SERVER_RECEIVE_END;
	static const Phase* INPUT_SERVER_SEND;

private:

	//No one else can create a new instance of this enum
	Phase(char* str);

	//Used to assign an ordinal to the phases
	static int _nextOrdinal;
};

#endif // PHASE