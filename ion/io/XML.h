///////////////////////////////////////////////////
// File:		XML.h
// Date:		15th February 2014
// Authors:		Matt Phillips
// Description:	XML reader
///////////////////////////////////////////////////

#pragma once

#include "core/Types.h"

#include <string>
#include <map>
#include <list>

#include <TinyXML/tinyxml2.h>

namespace ion
{
	namespace io
	{
		class XML
		{
		public:
			XML();
			~XML();

			//Load XML doc from file
			bool Load(const std::string& filename);

			//Hierarchy
			XML* FindChild(const std::string& name) const;
			XML* AddChild(const std::string& name);

			//Attributes
			bool GetAttribute(const std::string& name, std::string& value) const;
			bool GetAttribute(const std::string& name, int& value) const;
			bool GetAttribute(const std::string& name, float& value) const;

			void SetAttribute(const std::string& name, const std::string& value);
			void SetAttribute(const std::string& name, int value);
			void SetAttribute(const std::string& name, float value);

		protected:
			void ParseTinyXmlElement(const tinyxml2::XMLElement& element);

			std::string mData;
			std::map<std::string, std::string> mAttributes;
			std::map<std::string, XML> mChildren;
		};
	}
}