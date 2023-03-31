#include "stdafx.h"
#include "stdint.h"
#include <iostream>

#include "CXPathXML.h"

/*
关于XPath的基础知识，可以访问http://www.w3school.com.cn/xpath/index.asp
*/

using namespace std;
using namespace TinyXPath;

CXPathXML clsXmlSet;

CXPathXML::CXPathXML()
{
	doc = NULL;
	root_element = NULL;
	isChanged = FALSE;

	OpenXml();
}

CXPathXML::~CXPathXML()
{
	CloseXml();
}

void CXPathXML::CreateXml(void)
{
	doc = new TiXmlDocument; //xml文档指针
	//文档格式声明
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", "yes");
	doc->LinkEndChild(decl); //写入文档
	TiXmlElement* root_element = new TiXmlElement("wave data analyze settings");//根元素
	doc->LinkEndChild(root_element);
	isChanged = TRUE;
	root_element->LinkEndChild(new TiXmlElement("public"));
}

TiXmlElement* CXPathXML::GetGroupNode(char* GroupName)
{
	TiXmlElement* cur_node;
	for (cur_node = root_element->FirstChildElement(); cur_node; cur_node = cur_node->NextSiblingElement())
	{
		if (0 == strcmp(cur_node->Value(), GroupName))
		{
			return cur_node;
		}
	}
	GroupNode->LinkEndChild(cur_node = new TiXmlElement(GroupName));
	isChanged = TRUE;
	return cur_node;
}

void CXPathXML::GetElementVaue(char* GroupName, char* NodeName, void* NodeValue, NodeValueType type)
{
	TiXmlElement* cur_node;
	for (cur_node = GetGroupNode(GroupName)->FirstChildElement(); cur_node; cur_node = cur_node->NextSiblingElement())
	{
		if (0 == strcmp(cur_node->Value(), NodeName))
		{
			switch (type)
			{
			case NodeInt32Value:
				*(int32_t*)NodeValue = atoi(cur_node->GetText());
				break;
			case NodeInt64Value:
				*(int64_t*)NodeValue = atol(cur_node->GetText());
				break;
			case NodeDoubleValue:
				*(double*)NodeValue = atof(cur_node->GetText());
				break;
			case NodeStringValue:
				strcpy((char*)NodeValue, cur_node->GetText());
				break;
			}
			return;
		}
	}
	*(int64_t*)NodeValue = 0;
}


void CXPathXML::SetElementVaue(char* GroupName, char* NodeName, void* NodeValue, NodeValueType type)
{
	TiXmlElement *group_node, *cur_node;

	for (cur_node = (group_node = GetGroupNode(GroupName))->FirstChildElement(); cur_node; cur_node = cur_node->NextSiblingElement())
		if (0 == strcmp(cur_node->Value(), NodeName))break;
	if (!cur_node)
		group_node->LinkEndChild(cur_node = new TiXmlElement(NodeName));
	char s[1024];
	switch (type)
	{
	case NodeInt32Value:
		sprintf(s, "%d", *(int32_t*)NodeValue);
		break;
	case NodeInt64Value:
		sprintf(s, "%.20lld", *(int64_t*)NodeValue);
		break;
	case NodeDoubleValue:
		sprintf(s, "%.20lf", *(double*)NodeValue);
		break;
	case NodeStringValue:
		sprintf(s, "%s", (char*)NodeValue);
		break;
	}
	cur_node->SetValue(s);
	isChanged = TRUE;
}

void CXPathXML::OpenXml(void)
{
	doc = new TiXmlDocument(XMLFILENAME);
	if (!doc->LoadFile())	
	{
		CreateXml();
	}
	root_element = doc->RootElement();	
}

void CXPathXML::CloseXml(void)
{
	if (isChanged)
		doc->SaveFile();

	//delete root_element;
	//delete doc;
}