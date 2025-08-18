#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include <vector>
#include <string>


struct SpawnInfo
{
	std::string m_actor;
	Vec3 m_position;
	EulerAngles m_orientation;
	Vec3 m_velocity; // for projectile actors

	bool LoadFromXmlElement(XmlElement const& element);
};


struct MapDefinition
{
	static void InitializeDefinitions(const char* path = "Data/Definitions/MapDefinitions.xml");
	static void ClearDefinitions();
	static MapDefinition const* GetByName(std::string const& mapDefName);
	static std::vector<MapDefinition*> s_definitions;

	bool LoadFromXmlElement(XmlElement const& element);

	std::string m_name	= "TestMap";
	Image m_image;
	Shader* m_shader	= nullptr;
	Texture* m_spriteSheetTexture	= nullptr;
	IntVec2 m_spriteSheetCellCount	= IntVec2(8,8); // Sprite Sheet Dimensions
	std::vector<SpawnInfo> m_spawnInfos;
};

