#include "Game/GameCommon.hpp"
#include "Game/Actor.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RaycastUtils.hpp"


void AddVertsForWalls(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	unsigned int startIndex = static_cast<unsigned int>(verts.size());

	Vec2 uv0 = UVs.m_mins;
	Vec2 uv1 = Vec2(UVs.m_maxs.x, UVs.m_mins.y);
	Vec2 uv2 = UVs.m_maxs;
	Vec2 uv3 = Vec2(UVs.m_mins.x, UVs.m_maxs.y);

	Vec3 tangent = Vec3::ZERO;
	Vec3 bitangent = Vec3::ZERO;
	Vec3 normal = CrossProduct3D(bottomRight - bottomLeft, topLeft - bottomLeft).GetNormalized();

	verts.push_back(Vertex_PCUTBN(bottomLeft, color, uv0, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(bottomRight, color, uv1, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(topRight, color, uv2, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(topLeft, color, uv3, tangent, bitangent, normal));

	indexes.push_back(startIndex);
	indexes.push_back(startIndex + 1);
	indexes.push_back(startIndex + 2);

	indexes.push_back(startIndex);
	indexes.push_back(startIndex + 2);
	indexes.push_back(startIndex + 3);
}

void DebugDrawRaycastResult3D(RaycastResultWithActor result)
{
	float constexpr RAY_RADIUS = 0.01f;
	float constexpr DURATION = 1.f;
	float constexpr IMPACT_POINT_RADIUS = 0.06f;
	float constexpr ARROW_LENGTH = 0.3f;
	float constexpr ARROW_RADIUS = 0.03f;

	Vec3 startPos = result.m_rayStartPos;
	Vec3 endPos = result.m_rayStartPos + result.m_rayFwdNormal * result.m_rayLength;
	DebugAddWorldCylinder(startPos, endPos, RAY_RADIUS, DURATION, Rgba8::OPAQUE_WHITE, Rgba8::OPAQUE_WHITE, DebugRenderMode::X_RAY);

	if (result.m_didImpact)
	{
		Vec3 impactPos = result.m_rayStartPos + result.m_rayFwdNormal * result.m_impactDist;
		DebugAddWorldSphere(impactPos, IMPACT_POINT_RADIUS, DURATION, Rgba8::OPAQUE_WHITE, Rgba8::OPAQUE_WHITE);
		DebugAddWorldArrow(impactPos, impactPos + result.m_impactNormal * ARROW_LENGTH, ARROW_RADIUS, DURATION, Rgba8::BLUE, Rgba8::BLUE);

		if (result.m_hitActor)
		{
			DebugAddMessage(Stringf("Raycast Hit %s[%d]", result.m_hitActor->m_definition->m_name.c_str(), result.m_hitActor->m_handle.GetIndex()), 3.f);
		}
	}
}
