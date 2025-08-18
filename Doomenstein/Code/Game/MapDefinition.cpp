#include "Game/MapDefinition.hpp"
#include "Engine/Renderer/Renderer.hpp"

std::vector<MapDefinition*> MapDefinition::s_definitions;

STATIC MapDefinition const* MapDefinition::GetByName(std::string const& mapDefName)
{
	for (int i = 0; i < (int)s_definitions.size(); ++i)
	{
		if (s_definitions[i]->m_name == mapDefName)
		{
			return s_definitions[i];
		}
	}

	ERROR_AND_DIE(Stringf("Map is not in MapDefs: \"%s\"", mapDefName.c_str()));
}

STATIC void MapDefinition::InitializeDefinitions(const char* path /*= "Data/Definitions/MapDefinitions.xml"*/)
{
	ClearDefinitions();

	XmlDocument document;
	XmlResult result = document.LoadFile(path);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("Failed to open xml file: \"%s\"", path));

	XmlElement* rootElement = document.RootElement();
	GUARANTEE_OR_DIE(rootElement, Stringf("No elements in xml file: \"%s\"", path));

	XmlElement* mapDefElement = rootElement->FirstChildElement();
	while (mapDefElement != nullptr)
	{
		std::string elementName = mapDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "MapDefinition", Stringf("Root child element in %s was <%s>, must be <MapDefinition>!", path, elementName.c_str()));
		MapDefinition* newMapDef = new MapDefinition();
		newMapDef->LoadFromXmlElement(*mapDefElement);
		// GUARANTEE true?
		s_definitions.push_back(newMapDef);
		mapDefElement = mapDefElement->NextSiblingElement();
	}

}

STATIC void MapDefinition::ClearDefinitions()
{
	for (int i = 0; i < (int)s_definitions.size(); ++i)
	{
		delete s_definitions[i];
	}
	s_definitions.clear();
}

bool MapDefinition::LoadFromXmlElement(XmlElement const& element)
{
	m_name = ParseXmlAttribute(element, "name", m_name);

	std::string imagePath = ParseXmlAttribute(element, "image", "Data/Maps/TestMap.png");
	m_image = Image(imagePath.c_str());

	std::string shaderName = ParseXmlAttribute(element, "shader", "Data/Shaders/Diffuse");
	m_shader = g_theRenderer->CreateOrGetShader(shaderName.c_str(), VertexType::VERTEX_PCUTBN);

	std::string texturePath = ParseXmlAttribute(element, "spriteSheetTexture", "Data/Images/Terrain_8x8.png");
	m_spriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFile(texturePath.c_str());

	m_spriteSheetCellCount = ParseXmlAttribute(element, "spriteSheetCellCount", m_spriteSheetCellCount);

	XmlElement const* spawnInfosElement = element.FirstChildElement("SpawnInfos");
	if (spawnInfosElement)
	{
		XmlElement const* spawnInfoElement = spawnInfosElement->FirstChildElement();
		while (spawnInfoElement)
		{
			std::string elementName = spawnInfoElement->Name();
			GUARANTEE_OR_DIE(elementName == "SpawnInfo", Stringf("child element in %s was <%s>, must be <SpawnInfo>!", "SpawnInfos", elementName.c_str()));
			
			SpawnInfo spawnInfo;
			spawnInfo.LoadFromXmlElement(*spawnInfoElement);
			m_spawnInfos.push_back(spawnInfo);

			spawnInfoElement = spawnInfoElement->NextSiblingElement();
		}
	}
	return true;
}

bool SpawnInfo::LoadFromXmlElement(XmlElement const& element)
{
	m_actor = ParseXmlAttribute(element, "actor", m_actor);
	m_position = ParseXmlAttribute(element, "position", m_position);
	m_orientation = ParseXmlAttribute(element, "orientation", m_orientation);
	m_velocity = ParseXmlAttribute(element, "velocity", m_velocity);

	return true;
}
