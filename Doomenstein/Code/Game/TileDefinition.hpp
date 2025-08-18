#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <vector>
#include <string>

struct TileDefinition
{
	static void InitializeDefinitions(const char* path = "Data/Definitions/TileDefinitions.xml");
	static void ClearDefinitions();
	static TileDefinition const* GetByName(std::string const& tileDefName);
	static TileDefinition const* GetByMapColor(Rgba8 mapColor);
	static std::vector<TileDefinition*> s_definitions;

	bool LoadFromXmlElement(XmlElement const& element);

	std::string m_name;

	bool m_isSolid = false;
	Rgba8 m_mapImagePixelColor;
	IntVec2 m_floorSpriteCoords = IntVec2(-1, -1);
	IntVec2 m_ceilingSpriteCoords = IntVec2(-1, -1);
	IntVec2 m_wallSpriteCoords = IntVec2(-1, -1);
};

