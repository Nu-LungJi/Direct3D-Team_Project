#pragma once

#include "IEditorCommand.h"

NS_BEGIN(Engine)
class CTerrain;
NS_END

NS_BEGIN(Client)

class CTerrainEditCommand final : public IEditorCommand
{
public:
	explicit CTerrainEditCommand(E::CTerrain* terrain);
	void CaptureBefore(const E::_float3& worldCenter, float radius);
	bool Finalize();
	_bool Execute() override;
	_bool Undo() override;

private:
	struct HEIGHT_CHANGE { uint32_t x, z; float before, after; };
	struct MASK_CHANGE { int64_t x, z; std::vector<uint8_t> before, after; };
	_bool Apply(bool after);

private:
	E::CTerrain* m_pTerrain = nullptr;
	std::unordered_map<uint64_t, float> m_HeightBefore{};
	std::unordered_map<uint64_t, std::vector<uint8_t>> m_MaskBefore{};
	std::vector<HEIGHT_CHANGE> m_HeightChanges{};
	std::vector<MASK_CHANGE> m_MaskChanges{};
};

NS_END
