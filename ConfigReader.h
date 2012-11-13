#pragma once

#include "config.h"


namespace pugi
{
	class xml_document;
	class xml_node;

}

class ConfigReader
{
private:
	ConfigReader(void);
	pugi::xml_document* _config;
public:
	ConfigReader(char* fileName);
	~ConfigReader(void);
	int ReadProperty(char* section, char* attribute, char* buffer);
	int ReadIntegerValue(char* section, char* attribute);
};

