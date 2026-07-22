#pragma once
#include "Engine_Base.h"
#include "Engine_PhysxDefines.h"

NS_BEGIN(Engine)

class CPhysXCookingEditor final : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CPhysXCookingEditor, CEngineBase)

private:
	enum class SOURCE_TYPE : uint8_t
	{
		PROCEDURAL_PRIMITIVE,
		MODEL_RESOURCE
	};

	enum class PRIMITIVE_TYPE : uint8_t
	{
		CYLINDER,
		WEDGE
	};

	enum class COOK_TYPE : uint8_t
	{
		CONVEX_MESH,
		TRIANGLE_MESH
	};

	struct SOURCE_GEOMETRY
	{
		std::vector<_float3> vertices{};
		std::vector<uint32_t> indices{};
	};

	struct MODEL_MESH_SOURCE
	{
		std::string name{};
		SOURCE_GEOMETRY geometry{};
	};

private:
	CPhysXCookingEditor();
	~CPhysXCookingEditor() override = default;

public:
	void Open() { m_bOpen = true; }
	void UpdateGUI();

private:
	void DrawWindow();
	HRESULT Cook();
	HRESULT InspectModelBin();
	HRESULT BuildSourceGeometry(SOURCE_GEOMETRY& outGeometry) const;
	HRESULT BuildUnitCylinder(SOURCE_GEOMETRY& outGeometry) const;
	HRESULT BuildUnitWedge(SOURCE_GEOMETRY& outGeometry) const;
	HRESULT BuildModelGeometry(SOURCE_GEOMETRY& outGeometry) const;
	HRESULT CookGeometry(const SOURCE_GEOMETRY& geometry, const _string& outputPath) const;
	HRESULT ValidateCookedFile(const _string& outputPath) const;
	void QueueResult(std::string message, _bool success);

private:
	SOURCE_TYPE m_eSourceType{ SOURCE_TYPE::PROCEDURAL_PRIMITIVE };
	PRIMITIVE_TYPE m_ePrimitiveType{ PRIMITIVE_TYPE::CYLINDER };
	COOK_TYPE m_eCookType{ COOK_TYPE::CONVEX_MESH };
	int32_t m_iCylinderSegments{ PX_UNIT_CYLINDER_SEGMENTS };
	_bool m_bOverwrite{};
	_bool m_bOpen{};
	_bool m_bOpenResultPopup{};
	_bool m_bResultSuccess{};
	uint32_t m_iLastVertexCount{};
	uint32_t m_iLastIndexCount{};
	int32_t m_iSelectedModelMesh{ -1 };
	_float3 m_vModelScale{ 1.f, 1.f, 1.f };
	_float3 m_vModelRotation{};
	_float3 m_vModelTranslation{};
	std::vector<MODEL_MESH_SOURCE> m_ModelMeshes{};
	std::string m_Status{ "Ready." };
	std::string m_ResultMessage{};
	char m_ModelInputPath[512]{};
	char m_OutputPath[512]{};

public:
	static UPtr<CPhysXCookingEditor> Create();
};

NS_END
