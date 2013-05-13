#ifndef PHASE_H
#define PHASE_H

class Phase
{

public:
	
	const int _num;
	const char* _str;

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

	Phase(int num, char* str);
};

#endif // PHASE