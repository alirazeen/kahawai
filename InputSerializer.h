#pragma once
class InputSerializer
{
public:
	InputSerializer(void) {};
	~InputSerializer(void) {};

	virtual char*	Serialize(void* nativeInput, int* length) = 0;
	virtual void*	Deserialize(char* serializedInput)= 0;
	virtual void*	GetEmptyCommand()=0;


protected:

	template <class T>
	void Write(T theValue)
	{
		memcpy(_buffer+_position,&theValue,sizeof(T)); _position+=sizeof(T);
	}

	template <class T>
	T Read()
	{
		T result;
		memcpy(&result,_buffer+_position,sizeof(T)); _position+=sizeof(T);	
		return result;
	}


	char*			_buffer;
	int				_position;

};

