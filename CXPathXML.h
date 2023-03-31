#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../tinyxpath_1_3_1/tinyxml.h"
#include "../tinyxpath_1_3_1/xpath_static.h"

#define XMLFILENAME	"settings.xml"

class CXPathXML
{

public:
	CXPathXML();
	~CXPathXML();

	typedef enum TagNodeValueType
	{
		NodeInt32Value = 0,
		NodeInt64Value,
		NodeDoubleValue,
		NodeStringValue
	} NodeValueType;

	void CreateXml(void);
	void OpenXml(void);
	void CloseXml(void);
	void SetElementVaue(char* GroupName, char* NodeName, void* NodeValue, NodeValueType type);
	void GetElementVaue(char* GroupName, char* NodeName, void* NodeValue, NodeValueType type);
	TiXmlElement* GetGroupNode(char* GroupName);

public:
	TiXmlDocument* doc;
	TiXmlElement* root_element;
	TiXmlElement* GroupNode;
	bool isChanged;
};

extern CXPathXML clsXmlSet;