#include<iostream>

#ifdef KAHAWAI
#include "Kahawai.h"

#include<fstream>
#include<string>


DWORD WINAPI Kahawai::DecodeThreadStart(void* Param)
{
	Kahawai* This = (Kahawai*) Param;	
	while(This->DecodeAndShow(This->_hiVideoWidth,This->_hiVideoHeight,NULL));
	return 0;
}



DWORD WINAPI Kahawai::CrossStreamsThreadStart(void* Param)
{
	Kahawai* This = (Kahawai*) Param;
	return This->CrossStreams();
}


#endif