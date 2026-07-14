#pragma once
#include "Engine_Defines.h"
#include "Handle.h"

NS_BEGIN(Client)

class CEditorSelection final
{
public:
	explicit CEditorSelection(E::CHandle* primaryHandle);

	void SyncFromPrimary();
	void PruneInvalid();
	void SelectSingle(const E::CHandle& handle);
	void Toggle(const E::CHandle& handle);
	void SelectRange(const std::vector<E::CHandle>& handles, size_t firstIndex,
		size_t lastIndex, _bool additive);
	void Clear();

	_bool IsSelected(const E::CHandle& handle) const;
	const std::vector<E::CHandle>& GetHandles() const { return m_SelectedHandles; }
	size_t GetCount() const { return m_SelectedHandles.size(); }

private:
	void SetPrimary(const E::CHandle& handle);

private:
	E::CHandle* m_pPrimaryHandle = nullptr;
	E::CHandle m_LastPrimaryHandle{};
	std::vector<E::CHandle> m_SelectedHandles{};
};

NS_END
