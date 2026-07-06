
#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CGameObject;

class CAnimEdit_Manager final : public CEngineBase
{
public:
	struct AnimSpeedKey
	{
		float fTime = 0.f;
		float fSpeed = 1.f;
	};

private:
	CAnimEdit_Manager();
	~CAnimEdit_Manager();

public:
	HRESULT Initilize();


	HRESULT SetupTestModel();
public:

	void Update(_float fTimeDelta);

	void SetTestModelHandle(const CHandle& handle) { m_hTestModel = handle; }


public:
	uint32_t GetAnimIndex();
public:
	void IMGUI_Select_AnimType();
	void IMGUI_Slider_Animation();
	void IMGUI_Select_Animation();
	void IMGUI_AnimationSpeedCurve();
	void IMGUI_File_Rename(const std::string& Path, const std::string& fileName, const std::string& newfileName);


public:
	// helper ÇÔ¼öµé
	_bool RenameAnimFile_Overwrite(const std::string& oldFullPath, const std::string& newAnimName, std::string& outNewFullPath);
	_bool IsSamePath(const std::filesystem::path& a, const std::filesystem::path& b);
	_bool IsAlreadyLoadedAnim(const std::vector<SPtr<CResModelAnim>>& animations, const std::filesystem::path& loadPath);
	_bool WriteSaveBinary(const std::string _path, const std::string _Name);



public:
	void UpdateGUI();



public:
	float GetSpeedAtTime(float fTrackPos);
	
	


public:
	
private:
	CHandle m_hTestModel{};
	_float	m_fTimeDelta{ 0.f };
	uint32_t m_iCurrentAnimIndex{};

private:
	bool bPopup_File_Open = false;

	std::string oldPath;
	std::string newPath;

	std::vector<AnimSpeedKey> m_SpeedKeys;

public:
	static UPtr<CAnimEdit_Manager> Create();
};

NS_END

