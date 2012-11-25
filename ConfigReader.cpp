#include "kahawaiBase.h"
#ifdef KAHAWAI

#include "ConfigReader.h"
#include "utils.h"
#include "pugixml.hpp"

ConfigReader::ConfigReader(void)
{
}

ConfigReader::ConfigReader(char* fileName)
{
	_config = new pugi::xml_document();
	pugi::xml_parse_result result = _config->load_file(fileName);
	if(!result)
	{
		KahawaiLog("FATAL: Unable to read config file: kahawai.xml",KahawaiError);
		exit(0);
	}
}


ConfigReader::~ConfigReader(void)
{
}

int ConfigReader::ReadProperty(char* section, char* attribute, char* buffer)
{
	pugi::xml_node _root = _config->child(CONFIG_KAHAWAI);
	pugi::xml_node sectionNode = _root.child(section);
	strcpy(buffer,sectionNode.attribute(attribute).value());
	return 0;

}

int ConfigReader::ReadIntegerValue(char* section, char* attribute)
{
	pugi::xml_node _root = _config->child(CONFIG_KAHAWAI);
	pugi::xml_node sectionNode = _root.child(section);
	return atoi(sectionNode.attribute(attribute).value());
}

bool ConfigReader::ReadBooleanValue(char* section, char* attribute)
{
	pugi::xml_node _root = _config->child(CONFIG_KAHAWAI);
	pugi::xml_node sectionNode = _root.child(section);
	return strncmp(sectionNode.attribute(attribute).value(),"true",sizeof("true"))==0;
}

#endif