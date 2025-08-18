#include "Game/TileDefinition.hpp"

std::vector<TileDefinition*> TileDefinition::s_definitions;

STATIC void TileDefinition::InitializeDefinitions(const char* path /*= "Data/Definitions/TileDefinitions.xml"*/)
{
	ClearDefinitions();

	XmlDocument document;
	XmlResult result = document.LoadFile(path);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open xml file: \"%s\"", path));

	XmlElement* rootElement = document.RootElement();
	GUARANTEE_OR_DIE(rootElement, Stringf("No elements in xml file: \"%s\"", path));

	XmlElement* tileDefElement = rootElement->FirstChildElement();
	while (tileDefElement != nullptr)
	{
		std::string elementName = tileDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "TileDefinition", Stringf("Root child element in %s was <%s>, must be <TileDefinition>!", path, elementName.c_str()));
		TileDefinition* newTileDef = new TileDefinition();
		newTileDef->LoadFromXmlElement(*tileDefElement);
		s_definitions.push_back(newTileDef);

		tileDefElement = tileDefElement->NextSiblingElement();
	}
}

STATIC void TileDefinition::ClearDefinitions()
{
	for (int i = 0; i < (int)s_definitions.size(); ++i)
	{
		delete s_definitions[i];
	}
	s_definitions.clear();
}

STATIC TileDefinition const* TileDefinition::GetByName(std::string const& tileDefName)
{
	for (int i = 0; i < (int)s_definitions.size(); ++i)
	{
		if (s_definitions[i]->m_name == tileDefName)
		{
			return s_definitions[i];
		}
	}

	ERROR_AND_DIE(Stringf("Tile is not in TileDefinitions: \"%s\"", tileDefName.c_str()));
}


TileDefinition const* TileDefinition::GetByMapColor(Rgba8 mapColor)
{
	for (int i = 0; i < (int)s_definitions.size(); ++i)
	{
		if (s_definitions[i]->m_mapImagePixelColor == mapColor)
		{
			return s_definitions[i];
		}
	}
	ERROR_AND_DIE(Stringf("Map color is not in TileDefinitions!"));
}

bool TileDefinition::LoadFromXmlElement(XmlElement const& element)
{
	m_name = ParseXmlAttribute(element, "name", m_name);
	m_isSolid = ParseXmlAttribute(element, "isSolid", m_isSolid);
	m_mapImagePixelColor = ParseXmlAttribute(element, "mapImagePixelColor", m_mapImagePixelColor);
	m_floorSpriteCoords = ParseXmlAttribute(element, "floorSpriteCoords", m_floorSpriteCoords);
	m_ceilingSpriteCoords = ParseXmlAttribute(element, "ceilingSpriteCoords", m_ceilingSpriteCoords);
	m_wallSpriteCoords = ParseXmlAttribute(element, "wallSpriteCoords", m_wallSpriteCoords);

	return true;
}
