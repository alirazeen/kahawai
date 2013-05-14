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
	
	const int _num; //Numerical representation of the enum
	const char* _str; //String representation of teh enum

	//Phases encountered within the game thread
	static const Phase* GAME_START;
	static const Phase* GAME_END;

	static const Phase* CAPTURE_START;
	static const Phase* CAPTURE_END;

	//Phases encountered everywhere else (i.e. outside the game thread)
	static const Phase* KAHAWAI_START;
	static const Phase* KAHAWAI_END;

	static const Phase* TRANSFORM_START;
	static const Phase* TRANSFORM_END;

	static const Phase* DECODE_START;
	static const Phase* DECODE_END;

	static const Phase* SHOW_START;
	static const Phase* SHOW_END;

private:

	//No one else can create a new instance of this enum
	Phase(int num, char* str);
};

#endif // PHASE