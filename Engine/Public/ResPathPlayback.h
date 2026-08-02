#pragma once

#include "Resource.h"
#include "PathPlaybackDefines.h"

NS_BEGIN(Engine)

class ENGINE_DLL CResPathPlayback final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResPathPlayback, CResource)

private:
	explicit CResPathPlayback(const _string& sPath);
	~CResPathPlayback() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

	HRESULT SetData(PATH_PLAYBACK_DATA Data);
	const PATH_PLAYBACK_DATA& GetData() const { return m_Data; }
	const PATH_PLAYBACK_CLIP* FindClip(const StringID& sClipID) const;
	std::optional<size_t> FindClipIndex(const StringID& sClipID) const;
	_float GetClipDuration(const StringID& sClipID) const;
	static _bool ValidateAndNormalizeData(
		PATH_PLAYBACK_DATA& Data,
		std::vector<std::string>* pOutErrors = nullptr);

private:
	_bool BuildLookup(const PATH_PLAYBACK_DATA& Data);

private:
	PATH_PLAYBACK_DATA m_Data{};
	std::unordered_map<StringID, size_t> m_ClipLookup{};

public:
	static SPtr<CResPathPlayback> Create(const _string& sPath);
	static SPtr<CResPathPlayback> CreateAndLoad(const _string& sPath);
	static SPtr<CResPathPlayback> CreateFromData(PATH_PLAYBACK_DATA Data);
};

NS_END
