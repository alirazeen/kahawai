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

	//Phases encountered everywhere else (i.e. outside the game thread)
	static const Phase* KAHAWAI_START;
	static const Phase* KAHAWAI_END;

private:

	Phase(int num, char* str);
};

#endif // PHASE