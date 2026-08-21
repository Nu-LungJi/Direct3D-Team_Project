#include "pch.h"
#include "AnimEdit_Manager.h"
#include  "GameObject.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "ResModel.h"
#include "ResModelBone.h"
#include "DbgLineRender.h"
#include "JsonSerializer.h"
#include "JsonDeSerializer.h"


#include <fstream>
NS_USING(Engine)


CAnimEdit_Manager::CAnimEdit_Manager()
{
}
CAnimEdit_Manager::~CAnimEdit_Manager()
{

}

HRESULT CAnimEdit_Manager::Initilize()
{
    m_SpeedKeys.clear();

    m_SpeedKeys.push_back({ 0.f, 1.f });

	return S_OK;
}

HRESULT CAnimEdit_Manager::SetupTestModel()
{
	// 반드시 TestModel이 먼저 생성된 후에 CAnimEdit_Manager를 생성해야 한다.

	if (auto layer = CGameInstance::Get().GetGameObjectLayer("TestModelLayer"))
	{
		if (!layer->empty())
		{
			m_hTestModel = layer->front();
		}
	}

	return S_OK;
}



void CAnimEdit_Manager::Update(_float fTimeDelta)
{

}


_string CAnimEdit_Manager::GetAnimName(uint32_t iIndex, CHandle Handle)
{
	if (iIndex < 0)
		return "";
	auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(Handle);
	if (pSampleObj == nullptr)
		return "";

	auto pComModelInstance =
		pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");

	auto pModel = pComModelInstance->GetModel();
	
	if (pModel)
	{
		auto& anim = pModel->GetAnimations();
		if (anim.size() <= iIndex)
			return "";
		return anim[iIndex]->GetAnimName();
	}
	return "";
}

int32_t CAnimEdit_Manager::GetAnimIndex(CHandle Handle)
{
    int m_iSelectedAnimIndex = -1;
    auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(Handle);
    if (pSampleObj == nullptr)
        return m_iSelectedAnimIndex;

    auto pComModelInstance =
        pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");


    if (nullptr == pComModelInstance)
        return m_iSelectedAnimIndex;

    if (ImGui::Begin("SelectAnimation", NULL,ImGuiWindowFlags_MenuBar))
    {
        auto pModel = pComModelInstance->GetModel();

        if (pModel)
        {
            auto& anim = pModel->GetAnimations();

            for (uint32_t i = 0; i < anim.size(); ++i)
            {
                const std::string& animName = anim[i]->GetAnimName();

                if (ImGui::Selectable(animName.c_str()))
                {
                    m_iSelectedAnimIndex = static_cast<int>(i);
                
                   // ImGui::CloseCurrentPopup();
                }
            }
        }

        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::End();
    }

    return m_iSelectedAnimIndex;
}

void CAnimEdit_Manager::IMGUI_Select_AnimType()
{
	auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(m_hTestModel);
	if (pSampleObj == nullptr)
		return;

	auto pComAnimator =
		pSampleObj->GetComponent<CComAnimator>("ComCModelAnimator");

	if (pComAnimator == nullptr)
		return;

	int iAnimType =
		static_cast<int>(pComAnimator->GetAnimationTYPE());

	ImGuiViewport* pViewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(
		ImVec2(pViewport->Pos.x, pViewport->Pos.y));

	ImGui::SetNextWindowSize(
		ImVec2(pViewport->Size.x, 40.f));

	ImGui::SetNextWindowBgAlpha(0.75f);

	ImGui::Begin(
		"##AnimationEditor",
		nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse);

	const char* pTypeName =
		(iAnimType == 0) ? "Animation" : "Action";

	if (ImGui::Button(
		(std::string("Type : ") + pTypeName + "  v").c_str(),
		ImVec2(180.f, 28.f)))
	{
		ImGui::OpenPopup("AnimTypePopup");
	}

	if (ImGui::BeginPopup("AnimTypePopup"))
	{
		if (ImGui::MenuItem("Animation"))
			iAnimType = 0;

		if (ImGui::MenuItem("Action"))
			iAnimType = 1;

		ImGui::EndPopup();
	}

	pComAnimator->SetAnimationTYPE(
		static_cast<CComAnimator::ANIMTYPE>(iAnimType)
	);

	ImGui::SameLine();

	if (iAnimType == 0)
	{
		IMGUI_TopBar_Animation(pSampleObj, pComAnimator);
	}
	else if (iAnimType == 1)
	{
		IMGUI_TopBar_Action(pSampleObj, pComAnimator);
	}

	ImGui::End();
}

void CAnimEdit_Manager::IMGUI_TopBar_Animation(CGameObject* pSampleObj,CComAnimator* pComAnimator)
{
	if (!pSampleObj || !pComAnimator)
		return;

	// 여기 안에 기존 Save 관련 static 상태들 둔다.
	static bool s_bOpenRenamePopup = false;
	static bool s_bOpenSaveFilePopup = false;
	static bool s_bOpenSaveConfirmPopup = false;

	static char s_szRenameBuffer[256] = "";
	static char s_szSaveNameBuffer[256] = "";

	static std::filesystem::path s_SaveTargetPath;

	// ------------------------------------------------------------
	// Animation Save
	// ------------------------------------------------------------
	if (ImGui::Button("Save", ImVec2(100.f, 28.f)))
	{
		ImGui::OpenPopup("SaveAnimAction");
	}

	if (ImGui::BeginPopupModal("SaveAnimAction", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Select Animation Save Action");
		ImGui::Separator();

		if (ImGui::Button("Rename Animation", ImVec2(180.f, 28.f)))
		{
			s_bOpenRenamePopup = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::Spacing();

		if (ImGui::Button("Save New Animation File", ImVec2(180.f, 28.f)))
		{
			s_bOpenSaveFilePopup = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::Separator();

		if (ImGui::Button("Cancel", ImVec2(180.f, 28.f)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (s_bOpenRenamePopup)
	{
		s_bOpenRenamePopup = false;
		ImGui::OpenPopup("RenameAnim");
	}

	if (s_bOpenSaveFilePopup)
	{
		s_bOpenSaveFilePopup = false;
		ImGui::OpenPopup("SaveAnimFile");
	}

	if (s_bOpenSaveConfirmPopup)
	{
		s_bOpenSaveConfirmPopup = false;
		ImGui::OpenPopup("ConfirmSaveAnimFile");
	}

	// ------------------------------------------------------------
	// Rename Animation
	// ------------------------------------------------------------
	if (ImGui::BeginPopupModal("RenameAnim", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Rename Animation File");
		ImGui::Separator();

		ImGui::InputText("Anim Name", s_szRenameBuffer, sizeof(s_szRenameBuffer));

		if (ImGui::Button("OK", ImVec2(100.f, 28.f)))
		{
			uint32_t iSelected = pComAnimator->GetPlayAnimIndex();

			auto pComModelInstance =
				pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");

			if (pComModelInstance && pComModelInstance->GetModel())
			{
				auto animations =
					pComModelInstance->GetModel()->GetAnimations();

				if (!animations.empty() && iSelected < animations.size())
				{
					auto pAnim = animations[iSelected];

					if (pAnim && s_szRenameBuffer[0] != '\0')
					{
						std::string oldPath = pAnim->GetAnimPath();
						std::string newPath;

						if (RenameAnimFile_Overwrite(
							oldPath,
							s_szRenameBuffer,
							newPath))
						{
							// [LSY] 공유 Clip의 파일명 변경과 ResourceManager 경로 캐시를 함께 갱신한다.
							CGameInstance::Get().MoveResourcePathLookup(
								oldPath, newPath, pAnim);
							pAnim->SetAnimName(s_szRenameBuffer);
							pAnim->SetAnimPath(newPath);
						}
					}
				}
			}

			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(100.f, 28.f)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// ------------------------------------------------------------
	// Save Animation As New File
	// ------------------------------------------------------------
	if (ImGui::BeginPopupModal("SaveAnimFile", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Save Current Animation As New File");
		ImGui::Separator();

		ImGui::InputText("Save Name", s_szSaveNameBuffer, sizeof(s_szSaveNameBuffer));

		if (ImGui::Button("Save", ImVec2(100.f, 28.f)))
		{
			uint32_t iSelected = pComAnimator->GetPlayAnimIndex();

			auto pComModelInstance =
				pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");

			if (pComModelInstance && pComModelInstance->GetModel())
			{
				auto animations =
					pComModelInstance->GetModel()->GetAnimations();

				if (!animations.empty() && iSelected < animations.size())
				{
					auto pAnim = animations[iSelected];

					if (pAnim && s_szSaveNameBuffer[0] != '\0')
					{
						std::filesystem::path oldPath = pAnim->GetAnimPath();
						std::filesystem::path saveName = s_szSaveNameBuffer;

						if (saveName.extension().empty())
							saveName += oldPath.extension().string();

						s_SaveTargetPath = oldPath.parent_path() / saveName;

						s_bOpenSaveConfirmPopup = true;
						ImGui::CloseCurrentPopup();
					}
				}
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(100.f, 28.f)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// ------------------------------------------------------------
	// Confirm Save Animation
	// ------------------------------------------------------------
	if (ImGui::BeginPopupModal("ConfirmSaveAnimFile", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Really save this animation?");
		ImGui::Separator();

		ImGui::Text("Target File:");
		ImGui::TextWrapped("%s", s_SaveTargetPath.string().c_str());

		if (std::filesystem::exists(s_SaveTargetPath))
		{
			ImGui::Spacing();
			ImGui::TextColored(
				ImVec4(1.f, 0.3f, 0.3f, 1.f),
				"File already exists. It will be overwritten."
			);
		}

		ImGui::Separator();

		if (ImGui::Button("Yes, Save", ImVec2(120.f, 28.f)))
		{
			WriteSaveBakedBinary(
				s_SaveTargetPath.parent_path().string(),
				s_SaveTargetPath.stem().string()
			);

			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("No", ImVec2(100.f, 28.f)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::SameLine();

	// ------------------------------------------------------------
	// Animation Load
	// ------------------------------------------------------------
	if (ImGui::Button("Load", ImVec2(100.f, 28.f)))
	{
		ImGui::OpenPopup("LoadAnim");
	}



	ImGui::SameLine();

	// ------------------------------------------------------------
	// Current Animation 표시
	// ------------------------------------------------------------
	auto pComModelInstance =
		pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");

	if (pComModelInstance && pComModelInstance->GetModel())
	{
		auto animations = pComModelInstance->GetModel()->GetAnimations();

		uint32_t iSelected = pComAnimator->GetPlayAnimIndex();

		if (!animations.empty() && iSelected < animations.size() && animations[iSelected])
		{
			ImGui::Text("Current Anim : %s",
				animations[iSelected]->GetAnimName().c_str());
		}
		else
		{
			ImGui::Text("Current Anim : None");
		}
	}
}

void CAnimEdit_Manager::IMGUI_Slider_Animation()
{
    auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(m_hTestModel);
	if (pSampleObj == nullptr)
		return;


    auto pComModelInstance =
        pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");

    auto pComAnimator =
        pSampleObj->GetComponent<CComAnimator>("ComCModelAnimator");

    if (nullptr == pComAnimator)
        return;


    if (pComModelInstance->GetModel()->GetAnimations().size() == 0)
        return;

	if (pComAnimator->GetPlayAnimIndex() < 0)
		return;

	if (pComAnimator->GetPlayAnimIndex() == -1)
		return;

    auto pAnim = pComModelInstance->GetModel()->GetAnimations()[pComAnimator->GetPlayAnimIndex()];

    if (nullptr == pAnim)
        return;

    //----------------------------------------
    // 창 위치 (화면 중앙 아래)
    //----------------------------------------

    ImGuiViewport* pViewport = ImGui::GetMainViewport();

    constexpr float WINDOW_WIDTH = 700.f;
    constexpr float WINDOW_HEIGHT = 100.f;

    ImGui::SetNextWindowPos(ImVec2(pViewport->Pos.x + (pViewport->Size.x - WINDOW_WIDTH) * 0.5f, pViewport->Pos.y + pViewport->Size.y - WINDOW_HEIGHT - 20.f));

    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));

    ImGui::SetNextWindowBgAlpha(0.9f);

    ImGui::Begin("##AnimationTimeline",nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse);

    //----------------------------------------
    // Timeline
    //----------------------------------------

    float fDuration = pAnim->GetDuration();
    float fCurrentPos = pComAnimator->GetCurAnimState().fTrackPosition;
    float fTPS = pAnim->GetTickPerSecond();

    ImGui::Text("Animation Timeline");

    ImGui::SameLine();

	if (ImGui::Button(pComAnimator->GetPlay() ? "Pause" : "Play", ImVec2(70.f, 0.f)))
	{
		pComAnimator->SetPlay(!pComAnimator->GetPlay());
	}


    ImGui::PushItemWidth(-1.f);


    if (ImGui::SliderFloat("##AnimTimeline",&fCurrentPos,0.f,fDuration, "%.3f") )
    {
		// 에디터에서 스크럽할 때는 멈추는 게 자연스러움
		pComAnimator->SetPlay(false);

		// Resource Anim 말고 Animator의 TrackPosition을 바꿔야 함
		pComAnimator->SetTrackPosition(fCurrentPos);
        
    }
   



    ImGui::PopItemWidth();

    ImGui::Text("Time : %.3f / %.3f", fCurrentPos,fDuration);

    ImGui::SameLine();

    ImGui::Text("TPS : %.1f",fTPS);

    ImGui::End();
}

void CAnimEdit_Manager::IMGUI_Select_Animation()
{
    auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(m_hTestModel);
    if (!pSampleObj)
        return;

    auto pComModelInstance = pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");

    auto pComAnimator = pSampleObj->GetComponent<CComAnimator>("ComCModelAnimator");

    if (!pComModelInstance || !pComAnimator)
        return;

    auto& animations = pComModelInstance->GetModel()->GetAnimations();

    ImGui::Begin("Animation List");

	ImGui::SetNextItemWidth(-70.f);
	ImGui::InputTextWithHint(
		"##AnimationSearch",
		"Search animation name...",
		m_szAnimationSearch,
		IM_ARRAYSIZE(m_szAnimationSearch));
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
		m_szAnimationSearch[0] = '\0';

	std::string searchText = m_szAnimationSearch;
	std::ranges::transform(searchText, searchText.begin(),
		[](_char character)
		{
			return static_cast<_char>(
				std::tolower(static_cast<unsigned char>(character)));
		});

	uint32_t iMatchCount{};
	for (const auto& animation : animations)
	{
		if (!animation)
			continue;

		std::string animationName = animation->GetAnimName();
		std::ranges::transform(animationName, animationName.begin(),
			[](_char character)
			{
				return static_cast<_char>(
					std::tolower(static_cast<unsigned char>(character)));
			});
		if (searchText.empty() || animationName.find(searchText) != std::string::npos)
			++iMatchCount;
	}
	ImGui::TextDisabled(
		"Showing %u of %u animations",
		iMatchCount,
		static_cast<uint32_t>(animations.size()));

    if (ImGui::TreeNode("Animation"))
    {
        for (uint32_t i = 0; i < animations.size(); ++i)
        {
            auto pAnim = animations[i];

            if (!pAnim)
                continue;

			std::string animationNameLower = pAnim->GetAnimName();
			std::ranges::transform(animationNameLower, animationNameLower.begin(),
				[](_char character)
				{
					return static_cast<_char>(
						std::tolower(static_cast<unsigned char>(character)));
				});
			if (!searchText.empty() &&
				animationNameLower.find(searchText) == std::string::npos)
				continue;

            bool bSelected = (pComAnimator->GetPlayAnimIndex() == i);

 
            if (ImGui::Selectable(pAnim->GetAnimName().c_str(), bSelected))
            {
                pComAnimator->Play_Anim(i,true, 0.1f);

            }
        }

        ImGui::TreePop();
    }


    ImGui::End();
}

void CAnimEdit_Manager::IMGUI_Speed_Animation()
{
    auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(m_hTestModel);
    if (!pSampleObj)
        return;

    auto pComModelInstance =
        pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");

    auto pComAnimator =
        pSampleObj->GetComponent<CComAnimator>("ComCModelAnimator");

    if (!pComModelInstance || !pComAnimator)
        return;

    if (!pComModelInstance->GetModel())
        return;

    auto& animations = pComModelInstance->GetModel()->GetAnimations();

    if (animations.empty())
        return;

    uint32_t iAnimIndex = pComAnimator->GetPlayAnimIndex();

    if (iAnimIndex >= animations.size())
        return;

    auto pAnim = animations[iAnimIndex];

    if (!pAnim)
        return;

    float fDuration = pAnim->GetDuration();
	float fCurrentPos = pComAnimator->GetCurAnimState().fTrackPosition;

    if (m_SpeedKeys.empty())
    {
        m_SpeedKeys.push_back({ 0.f, 1.f });
    }

    //----------------------------------------
    // 창 위치
    //----------------------------------------
    ImGuiViewport* pViewport = ImGui::GetMainViewport();

    constexpr float WINDOW_WIDTH = 380.f;
    constexpr float WINDOW_HEIGHT = 360.f;

    ImGui::SetNextWindowPos(
        ImVec2(
            pViewport->Pos.x + pViewport->Size.x - WINDOW_WIDTH - 20.f,
            pViewport->Pos.y + 80.f
        ),
        ImGuiCond_FirstUseEver
    );

    ImGui::SetNextWindowSize(
        ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT),
        ImGuiCond_FirstUseEver
    );

    ImGui::Begin("Animation Speed Editor");

    //----------------------------------------
    // 현재 정보
    //----------------------------------------
    float fCurrentSpeed = GetSpeedAtTime(fCurrentPos);

    ImGui::Text("Current Anim : %s", pAnim->GetAnimName().c_str());
    ImGui::Text("Current Tick : %.3f / %.3f", fCurrentPos, fDuration);
    ImGui::Text("Current Speed : %.2fx", fCurrentSpeed);

    ImGui::Separator();

    //----------------------------------------
    // 현재 위치에 Speed Key 추가
    //----------------------------------------
    static float s_fNewSpeed = 1.f;

    ImGui::DragFloat("New Speed", &s_fNewSpeed, 0.01f, 0.05f, 50.f, "%.2fx");

    if (ImGui::Button("Add Key At Current Time", ImVec2(-1.f, 28.f)))
    {
        CAnimEdit_Manager::SPEED_KEY key{};
        key.fTime = fCurrentPos;
        key.fSpeed = s_fNewSpeed;

        m_SpeedKeys.push_back(key);

        std::sort(
            m_SpeedKeys.begin(),
            m_SpeedKeys.end(),
            [](const SPEED_KEY& A, const SPEED_KEY& B)
            {
                return A.fTime < B.fTime;
            });
    }

    if (ImGui::Button("Reset Speed Keys", ImVec2(-1.f, 28.f)))
    {
        m_SpeedKeys.clear();
        m_SpeedKeys.push_back({ 0.f, 1.f });
    }

    ImGui::Separator();

    //----------------------------------------
    // Speed Key 목록
    //----------------------------------------
    ImGui::Text("Speed Keys");

    static int s_iSelectedSpeedKey = -1;

    if (s_iSelectedSpeedKey >= static_cast<int>(m_SpeedKeys.size()))
        s_iSelectedSpeedKey = -1;

    ImGui::BeginChild("##SpeedKeyList", ImVec2(0.f, 120.f), true);

    for (int i = 0; i < static_cast<int>(m_SpeedKeys.size()); ++i)
    {
        char szLabel[128] = {};
        sprintf_s(
            szLabel,
            "Key %d | Time %.3f | Speed %.2fx",
            i,
            m_SpeedKeys[i].fTime,
            m_SpeedKeys[i].fSpeed
        );

        bool bSelected = (s_iSelectedSpeedKey == i);

        if (ImGui::Selectable(szLabel, bSelected))
        {
            s_iSelectedSpeedKey = i;
        }
    }

    ImGui::EndChild();

    //----------------------------------------
    // 선택된 Speed Key 편집
    //----------------------------------------
    if (s_iSelectedSpeedKey >= 0 &&
        s_iSelectedSpeedKey < static_cast<int>(m_SpeedKeys.size()))
    {
        ImGui::Separator();
        ImGui::Text("Edit Selected Key");

        SPEED_KEY& key = m_SpeedKeys[s_iSelectedSpeedKey];

        bool bNeedSort = false;

        if (s_iSelectedSpeedKey == 0)
        {
            ImGui::Text("First key time is fixed to 0.");
            key.fTime = 0.f;
        }
        else
        {
            if (ImGui::SliderFloat("Time", &key.fTime, 0.f, fDuration, "%.3f"))
            {
                bNeedSort = true;
            }
        }

        ImGui::DragFloat("Speed", &key.fSpeed, 0.01f, 0.05f, 5.f, "%.2fx");

        if (key.fSpeed < 0.05f)
            key.fSpeed = 0.05f;

        if (ImGui::Button("Move Selected Key To Current Time", ImVec2(-1.f, 28.f)))
        {
            if (s_iSelectedSpeedKey != 0)
            {
                key.fTime = fCurrentPos;
                bNeedSort = true;
            }
        }

        if (s_iSelectedSpeedKey != 0)
        {
            if (ImGui::Button("Delete Selected Key", ImVec2(-1.f, 28.f)))
            {
                m_SpeedKeys.erase(m_SpeedKeys.begin() + s_iSelectedSpeedKey);
                s_iSelectedSpeedKey = -1;
            }
        }

        if (bNeedSort)
        {
            std::sort(
                m_SpeedKeys.begin(),
                m_SpeedKeys.end(),
                [](const SPEED_KEY& A, const SPEED_KEY& B)
                {
                    return A.fTime < B.fTime;
                });

            s_iSelectedSpeedKey = -1;
        }
    }

    ImGui::Separator();

    //----------------------------------------
    // 디버그 출력
    //----------------------------------------
    ImGui::Text("Bake Preview");

    for (int i = 0; i < static_cast<int>(m_SpeedKeys.size()); ++i)
    {
        ImGui::Text(
            "[%d] Time %.3f  Speed %.2fx",
            i,
            m_SpeedKeys[i].fTime,
            m_SpeedKeys[i].fSpeed
        );
    }

    ImGui::End();
}

float CAnimEdit_Manager::GetSpeedAtTime(float fTrackPos)
{
    if (m_SpeedKeys.empty())
        return 1.f;

    std::sort(
        m_SpeedKeys.begin(),
        m_SpeedKeys.end(),
        [](const auto& A, const auto& B)
        {
            return A.fTime < B.fTime;
        });

    float fSpeed = m_SpeedKeys.front().fSpeed;

    for (auto& Key : m_SpeedKeys)
    {
        if (fTrackPos >= Key.fTime)
            fSpeed = Key.fSpeed;
        else
            break;
    }

    return fSpeed;
}

void CAnimEdit_Manager::UpdateGUI()
{
	auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(m_hTestModel);
	if (pSampleObj == nullptr)
		return;

	auto pComAnimator =
		pSampleObj->GetComponent<CComAnimator>("ComCModelAnimator");
	if (pComAnimator == nullptr)
		return;

	ImGui::Begin("Animation Editor Target");
	ImGui::TextUnformatted("Editor control is active.");
	ImGui::TextDisabled("Gameplay animation updates are paused for this target.");
	if (ImGui::Button("Release Target"))
	{
		// Timeline scrubbing pauses the animator. Returning control to gameplay
		// must not leave the target frozen.
		pComAnimator->SetPlay(true);
		ClearTarget();
		ImGui::End();
		return;
	}
	ImGui::End();

	auto pModelInstance =
		pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");
	if (pModelInstance && pModelInstance->GetModel())
	{
		const auto& bones = pModelInstance->GetModel()->GetBones();
		const auto& combinedMatrices =
			pModelInstance->Get_CombinedBoneMatrices();

		ImGui::Begin("Animation Bone Viewer");
		ImGui::Checkbox("Draw Skeleton", &m_bDrawSkeleton);
		ImGui::DragFloat(
			"Max Link Length", &m_fMaxBoneLinkLength,
			0.01f, 0.05f, 5.f, "%.2f m");
		ImGui::InputTextWithHint(
			"##BoneSearch", "Search bone name...",
			m_szBoneSearch, IM_ARRAYSIZE(m_szBoneSearch));

		std::string boneSearch = m_szBoneSearch;
		std::ranges::transform(boneSearch, boneSearch.begin(),
			[](_char character)
			{
				return static_cast<_char>(
					std::tolower(static_cast<unsigned char>(character)));
			});

		ImGui::BeginChild("BoneList", ImVec2(0.f, 240.f), true);
		for (int32_t i = 0; i < static_cast<int32_t>(bones.size()); ++i)
		{
			if (!bones[i])
				continue;
			const std::string boneName = bones[i]->GetBoneName();
			std::string lowerName = boneName;
			std::ranges::transform(lowerName, lowerName.begin(),
				[](_char character)
				{
					return static_cast<_char>(
						std::tolower(static_cast<unsigned char>(character)));
				});
			if (!boneSearch.empty() &&
				lowerName.find(boneSearch) == std::string::npos)
				continue;

			const std::string label =
				std::to_string(i) + " : " + boneName;
			if (ImGui::Selectable(
				label.c_str(), m_iSelectedBoneIndex == i))
				m_iSelectedBoneIndex = i;
		}
		ImGui::EndChild();

		if (m_iSelectedBoneIndex >= 0 &&
			m_iSelectedBoneIndex < static_cast<int32_t>(bones.size()) &&
			bones[m_iSelectedBoneIndex])
		{
			const int32_t parentIndex =
				bones[m_iSelectedBoneIndex]->GetParendBoneIndex();
			ImGui::Text("Selected: %s",
				bones[m_iSelectedBoneIndex]->GetBoneName().c_str());
			ImGui::Text("Index: %d  Parent: %d",
				m_iSelectedBoneIndex, parentIndex);
		}
		ImGui::End();

		if (m_bDrawSkeleton &&
			combinedMatrices.size() >= bones.size())
		{
			auto* debugDraw = CGameInstance::Get().GetDbgLineRender();
			if (debugDraw)
			{
				const auto previousColor = debugDraw->GetColor();
				const auto previousDepth = debugDraw->GetDepthMode();
				debugDraw->SetDepthTest(false);
				// Match the exact transform used by the model renderer. The plain
				// world matrix omits inherited/model scale for attached objects.
				const _matrix world =
					pSampleObj->GetTransform().GetLoadedCombinedWorldMatrix();
				for (int32_t i = 0;
					i < static_cast<int32_t>(bones.size()); ++i)
				{
					if (!bones[i])
						continue;
					const _matrix boneWorld =
						XMLoadFloat4x4(&combinedMatrices[i]) * world;
					_float3 bonePosition{};
					XMStoreFloat3(&bonePosition, boneWorld.r[3]);
					const int32_t parent = bones[i]->GetParendBoneIndex();
					if (parent >= 0 &&
						parent < static_cast<int32_t>(combinedMatrices.size()))
					{
						const _matrix parentWorld =
							XMLoadFloat4x4(&combinedMatrices[parent]) * world;
						_float3 parentPosition{};
						XMStoreFloat3(&parentPosition, parentWorld.r[3]);
						const _float linkLength = XMVectorGetX(
							XMVector3Length(
								XMLoadFloat3(&bonePosition) -
								XMLoadFloat3(&parentPosition)));
						if (linkLength <= m_fMaxBoneLinkLength)
						{
							debugDraw->AddLine(
								parentPosition, bonePosition,
								{ 0.2f, 0.7f, 1.f, 1.f });
						}
					}
					if (m_iSelectedBoneIndex == i)
					{
						debugDraw->SetColor({ 1.f, 0.2f, 0.1f, 1.f });
						const _matrix markerWorld = XMMatrixTranslation(
							bonePosition.x, bonePosition.y, bonePosition.z);
						debugDraw->AddSphere(0.025f, markerWorld);
						debugDraw->AddAxis(0.08f, markerWorld);
					}
				}
				debugDraw->SetColor(previousColor);
				debugDraw->SetDepthMode(previousDepth);
			}
		}
	}

	IMGUI_Select_AnimType();

	if (pComAnimator->GetAnimationTYPE() == CComAnimator::ANIM) {

		IMGUI_Slider_Animation();
		IMGUI_Select_Animation();
		IMGUI_Speed_Animation();
	}
	else if (pComAnimator->GetAnimationTYPE() == CComAnimator::ACTION) {
		IMGUI_ActionEditor();
	}


	
}

_bool CAnimEdit_Manager::RenameAnimFile_Overwrite(const std::string& oldFullPath,const std::string& newAnimName,std::string& outNewFullPath)
{
    if (oldFullPath.empty())
        return false;

    if (newAnimName.empty())
        return false;

    std::filesystem::path oldPath = oldFullPath;

    if (!std::filesystem::exists(oldPath))
        return false;

    // 사용자가 "Walk"만 입력하면 기존 확장자 .bin 붙이기
    std::filesystem::path inputName = newAnimName;
    std::string newFileName = newAnimName;

    if (inputName.extension().empty())
        newFileName += oldPath.extension().string();

    std::filesystem::path newPath = oldPath.parent_path() / newFileName;

    std::error_code ec;

    // 완전히 같은 경로면 파일 rename 할 필요 없음
    if (std::filesystem::absolute(oldPath).lexically_normal() ==
        std::filesystem::absolute(newPath).lexically_normal())
    {
        outNewFullPath = oldPath.string();
        return true;
    }

    // 새 이름의 파일이 이미 있으면 삭제해서 덮어쓰기 효과
    if (std::filesystem::exists(newPath))
    {
        std::filesystem::remove(newPath, ec);

        if (ec)
            return false;
    }

    // 실제 파일 이름 변경
    std::filesystem::rename(oldPath, newPath, ec);

    if (ec)
        return false;

    outNewFullPath = newPath.string();
    return true;
}
_bool  CAnimEdit_Manager::IsSamePath(const std::filesystem::path& a, const std::filesystem::path& b)
{
    std::error_code ec;

    std::filesystem::path absA = std::filesystem::absolute(a, ec).lexically_normal();
    std::filesystem::path absB = std::filesystem::absolute(b, ec).lexically_normal();

    return absA == absB;
}
_bool CAnimEdit_Manager::IsAlreadyLoadedAnim( const std::vector<SPtr<CResModelAnim>>& animations,const std::filesystem::path& loadPath)
{
    for (auto& anim : animations)
    {
        if (!anim)
            continue;

        std::filesystem::path animPath = anim->GetAnimPath();

        if (IsSamePath(animPath, loadPath))
            return true;
    }

    return false;
}

_bool CAnimEdit_Manager::WriteSaveBakedBinary(const std::string& _path,const std::string& _Name)
{
    std::vector<char> animBuffer;

    auto pushAnim = [&](const void* data, size_t size)
        {
            size_t old = animBuffer.size();
            animBuffer.resize(old + size);
            memcpy(animBuffer.data() + old, data, size);
        };

    auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(m_hTestModel);
    if (pSampleObj == nullptr)
        return false;

    auto pComAnimator =
        pSampleObj->GetComponent<CComAnimator>("ComCModelAnimator");

    auto pComModelInstance =
        pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");

    if (!pComAnimator || !pComModelInstance || !pComModelInstance->GetModel())
        return false;

    auto& Animations = pComModelInstance->GetModel()->GetAnimations();

    uint32_t iAnimIndex = pComAnimator->GetPlayAnimIndex();

    if (Animations.empty() || iAnimIndex >= Animations.size())
        return false;

    auto pAnim = Animations[iAnimIndex];

    if (!pAnim)
        return false;

    float fSourceDuration = pAnim->GetDuration();
    float fTickPerSecond = pAnim->GetTickPerSecond();

    // 30fps 기준으로 굽기
    // 더 부드럽게 하고 싶으면 60.f
    constexpr float fBakeSampleFPS = 30.f;

    std::vector<BAKE_SAMPLE> samples = BuildBakeSamples(fSourceDuration, fTickPerSecond, fBakeSampleFPS);

    if (samples.empty())
        return false;

    float fBakedDuration = samples.back().fBakedTrackPosition;

    std::filesystem::path animPath = std::filesystem::path(_path) / ("AN_" + _Name + ".bin");

    std::ofstream file(animPath, std::ios::binary);

    if (!file.is_open())
        return false;

    MODEL_FILE_HEADER MFH{};
    MFH.bHasBone = false;
    MFH.bHasAnimation = true;
    MFH.MeshCount = 0;
    MFH.AnimationCount = 1;
    MFH.MaterialCount = 0;
    MFH.BoneCount = 0;

    file.write((char*)&MFH, sizeof(MFH));

    // 굽힌 Duration 저장
    pushAnim(&fBakedDuration, sizeof(float));

    // TPS는 기존 TPS 유지
    pushAnim(&fTickPerSecond, sizeof(float));

    uint32_t ChannelCount = pAnim->GetNumChannel();
    pushAnim(&ChannelCount, sizeof(uint32_t));

    auto& Channels = pAnim->GetChannels();

    for (uint32_t i = 0; i < ChannelCount; ++i)
    {
        auto pChannel = Channels[i];

        if (!pChannel)
            continue;

        int32_t BoneIndex = pChannel->Get_BoneIndex();

        // 이제 키프레임 개수는 원본 키 개수가 아니라 굽힌 샘플 개수
        uint32_t KeyFrameCount = static_cast<uint32_t>(samples.size());

        uint32_t ChannelSize =
            sizeof(int32_t) +
            sizeof(uint32_t) +
            KeyFrameCount *
            (
                sizeof(XMFLOAT3) +
                sizeof(XMFLOAT4) +
                sizeof(XMFLOAT3) +
                sizeof(float)
                );

        pushAnim(&ChannelSize, sizeof(uint32_t));
        pushAnim(&BoneIndex, sizeof(int32_t));
        pushAnim(&KeyFrameCount, sizeof(uint32_t));

        for (uint32_t j = 0; j < KeyFrameCount; ++j)
        {
            const BAKE_SAMPLE& sample = samples[j];

            KEYFRAME bakedKey =
                SampleChannelKeyFrame(
                    pChannel.get(),
                    sample.fSourceTrackPosition);


            bakedKey.fTrackPosition = sample.fBakedTrackPosition;

            pushAnim(&bakedKey.vScale, sizeof(XMFLOAT3));
            pushAnim(&bakedKey.vRotation, sizeof(XMFLOAT4));
            pushAnim(&bakedKey.vTranslation, sizeof(XMFLOAT3));
            pushAnim(&bakedKey.fTrackPosition, sizeof(float));
        }
    }

    ChunkHeader chAnim{};
    chAnim.type = CHUNCK_TYPE::CHUNK_ANIM;
    chAnim.size = static_cast<uint32_t>(animBuffer.size());

    file.write((char*)&chAnim, sizeof(chAnim));
    file.write(animBuffer.data(), animBuffer.size());

    file.close();

    return true;
}
std::vector<CAnimEdit_Manager::BAKE_SAMPLE> CAnimEdit_Manager::BuildBakeSamples(float fSourceDuration,float fTickPerSecond,float fSampleFPS)
{
    std::vector<BAKE_SAMPLE> samples;

    if (fSourceDuration <= 0.f || fTickPerSecond <= 0.f || fSampleFPS <= 0.f)
        return samples;

    float fSourceTrack = 0.f;
    float fBakedTrack = 0.f;

    // 예: TPS 60, SampleFPS 30이면 2 tick마다 키프레임 하나 생성
    const float fDeltaTrack = fTickPerSecond / fSampleFPS;

    while (fSourceTrack < fSourceDuration)
    {
        BAKE_SAMPLE sample{};
        sample.fSourceTrackPosition = fSourceTrack;
        sample.fBakedTrackPosition = fBakedTrack;
        samples.push_back(sample);

        float fSpeed = GetSpeedAtTime(fSourceTrack);

        // 0 이하 속도면 무한루프 방지
        if (fSpeed <= 0.001f)
            fSpeed = 0.001f;

        fSourceTrack += fDeltaTrack * fSpeed;
        fBakedTrack += fDeltaTrack;
    }

    // 마지막 프레임 보장
    BAKE_SAMPLE last{};
    last.fSourceTrackPosition = fSourceDuration;
    last.fBakedTrackPosition = fBakedTrack;
    samples.push_back(last);

    return samples;
}

KEYFRAME CAnimEdit_Manager::SampleChannelKeyFrame(CResModelChanel* pChannel,float fTrackPosition)
{
    KEYFRAME result{};

    auto& keyFrames = pChannel->Get_KeyFrames();

    if (keyFrames.empty())
        return result;

    if (keyFrames.size() == 1)
    {
        result = keyFrames[0];
        result.fTrackPosition = fTrackPosition;
        return result;
    }

    if (fTrackPosition <= keyFrames.front().fTrackPosition)
    {
        result = keyFrames.front();
        result.fTrackPosition = fTrackPosition;
        return result;
    }

    if (fTrackPosition >= keyFrames.back().fTrackPosition)
    {
        result = keyFrames.back();
        result.fTrackPosition = fTrackPosition;
        return result;
    }

    uint32_t iIndex = 0;

    for (uint32_t i = 0; i < keyFrames.size() - 1; ++i)
    {
        if (fTrackPosition < keyFrames[i + 1].fTrackPosition)
        {
            iIndex = i;
            break;
        }
    }

    const auto& A = keyFrames[iIndex];
    const auto& B = keyFrames[iIndex + 1];

    float fRange = B.fTrackPosition - A.fTrackPosition;
    float fRatio = 0.f;

    if (fRange > 0.f)
        fRatio = (fTrackPosition - A.fTrackPosition) / fRange;

    fRatio = std::clamp(fRatio, 0.f, 1.f);

    XMVECTOR vScaleA = XMLoadFloat3(&A.vScale);
    XMVECTOR vScaleB = XMLoadFloat3(&B.vScale);

    XMVECTOR vRotA = XMLoadFloat4(&A.vRotation);
    XMVECTOR vRotB = XMLoadFloat4(&B.vRotation);

    XMVECTOR vTransA = XMLoadFloat3(&A.vTranslation);
    XMVECTOR vTransB = XMLoadFloat3(&B.vTranslation);

    XMVECTOR vScale = XMVectorLerp(vScaleA, vScaleB, fRatio);
    XMVECTOR vRot = XMQuaternionSlerp(vRotA, vRotB, fRatio);
    XMVECTOR vTrans = XMVectorLerp(vTransA, vTransB, fRatio);

    XMStoreFloat3(&result.vScale, vScale);
    XMStoreFloat4(&result.vRotation, XMQuaternionNormalize(vRot));
    XMStoreFloat3(&result.vTranslation, vTrans);

    result.fTrackPosition = fTrackPosition;

    return result;
}

void CAnimEdit_Manager::IMGUI_File_Rename(const std::string& Path, const std::string& fileName,const std::string& newfileName )
{

    std::filesystem::path oldPath(Path);

    std::string ext = oldPath.extension().string();

    std::filesystem::path newPath =
        oldPath.parent_path() / (newfileName + ext);

    if (std::filesystem::exists(newPath))
    {
      

        return ;
    }

    std::filesystem::rename(oldPath, newPath);

    return ;

}

void CAnimEdit_Manager::IMGUI_TopBar_Action(CGameObject* pSampleObj, CComAnimator* pComAnimator)
{
	if (!pSampleObj || !pComAnimator)
		return;

	if (ImGui::Button("New Action", ImVec2(110.f, 28.f)))
	{
		CComAnimator::ACTIONSTRUCT action{};
		action.ActionName = "NewAction";
		pComAnimator->GetActions().push_back(std::move(action));
		m_iSelectedActionIndex = static_cast<int32_t>(pComAnimator->GetActions().size()) - 1;
	}

	ImGui::SameLine();
	if (ImGui::Button("Save Actions", ImVec2(110.f, 28.f)))
	{
		m_ActionStatus = SaveActions(*pComAnimator, m_ActionFilePath)
			? "Actions saved."
			: "Failed to save Actions.";
	}

	ImGui::SameLine();
	if (ImGui::Button("Load Actions", ImVec2(110.f, 28.f)))
	{
		m_ActionStatus = LoadActions(*pComAnimator, m_ActionFilePath)
			? "Actions loaded."
			: "Failed to load Actions.";
	}

	ImGui::SameLine();
	if (m_iSelectedActionIndex >= 0 &&
		m_iSelectedActionIndex < static_cast<int32_t>(pComAnimator->GetActions().size()) &&
		ImGui::Button("Preview", ImVec2(90.f, 28.f)))
	{
		pComAnimator->Play_Action(m_iSelectedActionIndex, 0.1f);
	}
}

void CAnimEdit_Manager::IMGUI_ActionEditor()
{
	auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(m_hTestModel);
	if (!pSampleObj)
		return;

	auto pAnimator = pSampleObj->GetComponent<CComAnimator>("ComCModelAnimator");
	auto pModelInstance = pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");
	if (!pAnimator || !pModelInstance || !pModelInstance->GetModel())
		return;

	auto& actions = pAnimator->GetActions();
	if (m_iSelectedActionIndex >= static_cast<int32_t>(actions.size()))
		m_iSelectedActionIndex = actions.empty() ? -1 : 0;

	ImGui::SetNextWindowPos(ImVec2(10.f, 55.f), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(900.f, 540.f), ImGuiCond_Once);
	if (!ImGui::Begin("Action Editor"))
	{
		ImGui::End();
		return;
	}

	ImGui::TextUnformatted("Action asset");
	ImGui::SameLine();
	char pathBuffer[512]{};
	strncpy_s(pathBuffer, m_ActionFilePath.string().c_str(), _TRUNCATE);
	if (ImGui::InputText("##ActionFile", pathBuffer, sizeof(pathBuffer)))
		m_ActionFilePath = pathBuffer;
	if (!m_ActionStatus.empty())
		ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.45f, 1.f), "%s", m_ActionStatus.c_str());

	ImGui::BeginChild("ActionList", ImVec2(220.f, 0.f), true);
	for (int32_t i = 0; i < static_cast<int32_t>(actions.size()); ++i)
	{
		const char* name = actions[i].ActionName.empty() ? "Unnamed Action" : actions[i].ActionName.c_str();
		if (ImGui::Selectable(name, m_iSelectedActionIndex == i))
			m_iSelectedActionIndex = i;
	}
	ImGui::EndChild();
	ImGui::SameLine();

	ImGui::BeginChild("ActionDetails", ImVec2(0.f, 0.f), true);
	if (m_iSelectedActionIndex < 0 || m_iSelectedActionIndex >= static_cast<int32_t>(actions.size()))
	{
		ImGui::TextUnformatted("Create an Action from the top bar to begin.");
	}
	else
	{
		auto& action = actions[m_iSelectedActionIndex];
		char nameBuffer[128]{};
		strncpy_s(nameBuffer, action.ActionName.c_str(), _TRUNCATE);
		if (ImGui::InputText("Action Name", nameBuffer, sizeof(nameBuffer)))
			action.ActionName = nameBuffer;

		ImGui::SameLine();
		if (ImGui::Button("Add Current Animation"))
		{
			const int32_t currentIndex = static_cast<int32_t>(pAnimator->GetPlayAnimIndex());
			if (currentIndex >= 0)
			{
				CComAnimator::ANIMSTRUCT segment{};
				segment.iAnimIndex = currentIndex;
				segment.bLoop = false;
				action.Anims.push_back(segment);
				action.StartTime.push_back(action.LastTime);
				RefreshActionLastTime(action, pSampleObj);
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Delete Action"))
		{
			actions.erase(actions.begin() + m_iSelectedActionIndex);
			m_iSelectedActionIndex = actions.empty() ? -1 : std::min(m_iSelectedActionIndex, static_cast<int32_t>(actions.size()) - 1);
			ImGui::EndChild();
			ImGui::End();
			return;
		}

		ImGui::Separator();
		ImGui::Text("Timeline length: %.2f ticks", action.LastTime);
		if (ImGui::BeginTable("ActionSegments", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Order");
			ImGui::TableSetupColumn("Animation");
			ImGui::TableSetupColumn("Start (ticks)");
			ImGui::TableSetupColumn("Speed");
			ImGui::TableSetupColumn("Remove");
			ImGui::TableHeadersRow();
			auto& resourceAnims = pModelInstance->GetModel()->GetAnimations();
			for (int32_t i = 0; i < static_cast<int32_t>(action.Anims.size()); ++i)
			{
				ImGui::PushID(i);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);
				ImGui::TableSetColumnIndex(1);
				const char* preview = (action.Anims[i].iAnimIndex >= 0 && action.Anims[i].iAnimIndex < static_cast<int32_t>(resourceAnims.size())) ? resourceAnims[action.Anims[i].iAnimIndex]->GetAnimName().c_str() : "Invalid";
				if (ImGui::BeginCombo("##Animation", preview))
				{
					for (int32_t animIndex = 0; animIndex < static_cast<int32_t>(resourceAnims.size()); ++animIndex)
						if (ImGui::Selectable(resourceAnims[animIndex]->GetAnimName().c_str(), action.Anims[i].iAnimIndex == animIndex)) action.Anims[i].iAnimIndex = animIndex;
					ImGui::EndCombo();
				}
				ImGui::TableSetColumnIndex(2); ImGui::DragFloat("##Start", &action.StartTime[i], 1.f, 0.f);
				ImGui::TableSetColumnIndex(3); ImGui::DragFloat("##Speed", &action.Anims[i].fSpeed, 0.05f, 0.01f, 10.f);
				ImGui::TableSetColumnIndex(4);
				if (ImGui::SmallButton("Remove")) { action.Anims.erase(action.Anims.begin() + i); action.StartTime.erase(action.StartTime.begin() + i); --i; }
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		// Animation 기준 LastTime 먼저 계산
		RefreshActionLastTime(action, pSampleObj);

		// std::string 편집용
		auto InputString =
			[](
				const char* pLabel,
				std::string& strValue)
			{
				char szBuffer[256]{};

				strncpy_s(
					szBuffer,
					sizeof(szBuffer),
					strValue.c_str(),
					_TRUNCATE
				);

				if (ImGui::InputText(
					pLabel,
					szBuffer,
					sizeof(szBuffer)))
				{
					strValue = szBuffer;
					return true;
				}

				return false;
			};


		// ============================================================
		// Collider Events
		// ============================================================

		ImGui::Spacing();
		ImGui::Separator();

		if (ImGui::CollapsingHeader(
			"Collider Events",
			ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Button(
				"Add Collider",
				ImVec2(140.f, 28.f)))
			{
				CComAnimator::COLLIDER_EVENT_DESC collider{};

				collider.sColliderName =
					"Collider_" +
					std::to_string(action.Colliders.size());

				collider.sBoneName = "Reference";
				collider.fActionTrackPosition = 0.f;

				collider.vLocalPosition =
					_float3{ 0.f, 0.f, 0.f };

				collider.vLocalRotation =
					_float3{ 0.f, 0.f, 0.f };

				collider.vLocalScale =
					_float3{ 1.f, 1.f, 1.f };

				action.Colliders.push_back(
					std::move(collider)
				);
			}

			ImGui::SameLine();

			ImGui::Text(
				"Collider Count : %d",
				static_cast<int32_t>(
					action.Colliders.size()
					)
			);

			int32_t iRemoveCollider = -1;

			for (int32_t i = 0;
				i < static_cast<int32_t>(
					action.Colliders.size());
				++i)
			{
				auto& collider =
					action.Colliders[i];

				ImGui::PushID(10000 + i);

				const char* pColliderName =
					collider.sColliderName.empty()
					? "Unnamed Collider"
					: collider.sColliderName.c_str();

				const bool bOpen =
					ImGui::TreeNodeEx(
						"##ColliderEvent",
						ImGuiTreeNodeFlags_Framed |
						ImGuiTreeNodeFlags_DefaultOpen,
						"Collider %d : %s",
						i,
						pColliderName
					);

				if (bOpen)
				{
					InputString(
						"Collider Name",
						collider.sColliderName
					);

					InputString(
						"Bone Name",
						collider.sBoneName
					);

					ImGui::DragFloat(
						"Action Position",
						&collider.fActionTrackPosition,
						0.01f,
						0.f,
						0.f,
						"%.3f ticks"
					);

					collider.fActionTrackPosition =
						std::max(
							0.f,
							collider.fActionTrackPosition
						);

					ImGui::DragFloat3(
						"Local Position",
						reinterpret_cast<float*>(
							&collider.vLocalPosition
							),
						0.01f
					);

					ImGui::DragFloat3(
						"Local Rotation",
						reinterpret_cast<float*>(
							&collider.vLocalRotation
							),
						0.1f
					);

					ImGui::DragFloat3(
						"Local Scale",
						reinterpret_cast<float*>(
							&collider.vLocalScale
							),
						0.01f,
						0.001f,
						100.f
					);

					collider.vLocalScale.x =
						std::max(
							0.001f,
							collider.vLocalScale.x
						);

					collider.vLocalScale.y =
						std::max(
							0.001f,
							collider.vLocalScale.y
						);

					collider.vLocalScale.z =
						std::max(
							0.001f,
							collider.vLocalScale.z
						);

					if (ImGui::Button(
						"Move To Action End"))
					{
						collider.fActionTrackPosition =
							action.LastTime;
					}

					ImGui::SameLine();

					if (ImGui::Button(
						"Remove Collider"))
					{
						iRemoveCollider = i;
					}

					ImGui::TreePop();
				}

				ImGui::PopID();
			}

			if (iRemoveCollider >= 0)
			{
				action.Colliders.erase(
					action.Colliders.begin() +
					iRemoveCollider
				);
			}
		}


		// ============================================================
		// Sound Events
		// ============================================================

		ImGui::Spacing();
		ImGui::Separator();

		if (ImGui::CollapsingHeader(
			"Sound Events",
			ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Button(
				"Add Sound",
				ImVec2(140.f, 28.f)))
			{
				CComAnimator::SOUND_EVENT_DESC sound{};

				sound.sSoundName =
					"Sound_" +
					std::to_string(action.Sounds.size());

				sound.fActionTrackPosition = 0.f;

				sound.eSoundType =
					CComAnimator::SOUND_3D;

				sound.fVolume = 1.f;
				sound.fPitch = 1.f;

				sound.bLoop = false;
				sound.bFollowOwner = true;

				sound.sBoneName = "Reference";

				sound.vLocalPosition =
					_float3{ 0.f, 0.f, 0.f };

				action.Sounds.push_back(
					std::move(sound)
				);
			}

			ImGui::SameLine();

			ImGui::Text(
				"Sound Count : %d",
				static_cast<int32_t>(
					action.Sounds.size()
					)
			);

			int32_t iRemoveSound = -1;

			for (int32_t i = 0;
				i < static_cast<int32_t>(
					action.Sounds.size());
				++i)
			{
				auto& sound =
					action.Sounds[i];

				ImGui::PushID(20000 + i);

				const char* pSoundName =
					sound.sSoundName.empty()
					? "Unnamed Sound"
					: sound.sSoundName.c_str();

				const bool bOpen =
					ImGui::TreeNodeEx(
						"##SoundEvent",
						ImGuiTreeNodeFlags_Framed |
						ImGuiTreeNodeFlags_DefaultOpen,
						"Sound %d : %s",
						i,
						pSoundName
					);

				if (bOpen)
				{
					InputString(
						"Sound Name",
						sound.sSoundName
					);

					ImGui::DragFloat(
						"Action Position",
						&sound.fActionTrackPosition,
						0.01f,
						0.f,
						0.f,
						"%.3f ticks"
					);

					sound.fActionTrackPosition =
						std::max(
							0.f,
							sound.fActionTrackPosition
						);

					int32_t iSoundType =
						static_cast<int32_t>(
							sound.eSoundType
							);

					const char* pSoundTypeNames[] =
					{
						"2D",
						"3D"
					};

					if (ImGui::Combo(
						"Sound Type",
						&iSoundType,
						pSoundTypeNames,
						IM_ARRAYSIZE(pSoundTypeNames)))
					{
						sound.eSoundType =
							static_cast<
							CComAnimator::SOUND_TYPE>(
								iSoundType
								);
					}

					ImGui::DragFloat(
						"Volume",
						&sound.fVolume,
						0.01f,
						0.f,
						10.f,
						"%.2f"
					);

					ImGui::DragFloat(
						"Pitch",
						&sound.fPitch,
						0.01f,
						0.01f,
						10.f,
						"%.2f"
					);

					ImGui::Checkbox(
						"Loop",
						&sound.bLoop
					);

					if (sound.eSoundType ==
						CComAnimator::SOUND_3D)
					{
						ImGui::Checkbox(
							"Follow Owner",
							&sound.bFollowOwner
						);

						InputString(
							"Bone Name",
							sound.sBoneName
						);

						ImGui::DragFloat3(
							"Local Position",
							reinterpret_cast<float*>(
								&sound.vLocalPosition
								),
							0.01f
						);
					}
					else
					{
						ImGui::TextDisabled(
							"2D Sound does not use Bone or Local Position."
						);
					}

					if (ImGui::Button(
						"Move To Action End"))
					{
						sound.fActionTrackPosition =
							action.LastTime;
					}

					ImGui::SameLine();

					if (ImGui::Button(
						"Remove Sound"))
					{
						iRemoveSound = i;
					}

					ImGui::TreePop();
				}

				ImGui::PopID();
			}

			if (iRemoveSound >= 0)
			{
				action.Sounds.erase(
					action.Sounds.begin() +
					iRemoveSound
				);
			}
		}


		// Collider와 Sound가 Animation보다 뒤에 있으면
		// Action LastTime을 이벤트 위치까지 늘림
		for (const auto& collider : action.Colliders)
		{
			action.LastTime =
				std::max(
					action.LastTime,
					collider.fActionTrackPosition
				);
		}

		for (const auto& sound : action.Sounds)
		{
			action.LastTime =
				std::max(
					action.LastTime,
					sound.fActionTrackPosition
				);
		}



	}
	ImGui::EndChild();
	ImGui::End();
}

_bool CAnimEdit_Manager::SaveActions(const CComAnimator& animator, const std::filesystem::path& path) const
{
	std::error_code ec;
	std::filesystem::create_directories(path.parent_path(), ec);
	if (ec)
		return false;
	auto serializer = CJsonSerializer::Create();
	if (!serializer)
		return false;
	static_cast<ISerializer&>(*serializer).Write("Actions", animator.GetActions());
	return SUCCEEDED(serializer->SaveToFile(path.string()));
}

_bool CAnimEdit_Manager::LoadActions(CComAnimator& animator, const std::filesystem::path& path)
{
	auto deserializer = CJsonDeSerializer::Create(path.string());
	if (!deserializer)
		return false;
	std::vector<CComAnimator::ACTIONSTRUCT> loadedActions;
	static_cast<IDeserializer&>(*deserializer).Read("Actions", loadedActions);
	animator.GetActions() = std::move(loadedActions);
	m_iSelectedActionIndex = animator.GetActions().empty() ? -1 : 0;
	return true;
}

void CAnimEdit_Manager::RefreshActionLastTime(CComAnimator::ACTIONSTRUCT& action, CGameObject* pSampleObj) const
{
	action.LastTime = 0.f;
	auto pModelInstance = pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");
	if (!pModelInstance || !pModelInstance->GetModel())
		return;
	auto& animations = pModelInstance->GetModel()->GetAnimations();
	for (size_t i = 0; i < action.Anims.size() && i < action.StartTime.size(); ++i)
	{
		const int32_t animIndex = action.Anims[i].iAnimIndex;
		if (animIndex >= 0 && animIndex < static_cast<int32_t>(animations.size()))
			action.LastTime = std::max(action.LastTime, action.StartTime[i] + animations[animIndex]->GetDuration());
	}
}



UPtr<CAnimEdit_Manager> CAnimEdit_Manager::Create()
{
	auto pInstance = UPtr<CAnimEdit_Manager>(new CAnimEdit_Manager{});
	if (FAILED(pInstance->Initilize()))
	{
		MSG_BOX("CAnimEdit_Manager Create Failed");
		return nullptr;
	}
	return pInstance;
}
