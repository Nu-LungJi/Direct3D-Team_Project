#include "pch.h"
#include "AnimEdit_Manager.h"
#include  "GameObject.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "ResModel.h"

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
    if(pSampleObj == nullptr)
		return; 

    auto pComAnimator = pSampleObj->GetComponent<CComAnimator>("ComCModelAnimator");


    if (nullptr == pComAnimator)
        return;

    int iAnimType = static_cast<int>(pComAnimator->GetAnimationTYPE());

    //----------------------------------------
    // 화면 상단 전체 너비
    //----------------------------------------

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
        (iAnimType == 0) ? "Animation" : "Montage";

    // 타입 선택
    if (ImGui::Button((std::string("Type : ") + pTypeName + "  v").c_str(),ImVec2(180.f, 28.f)))
    {
        ImGui::OpenPopup("AnimTypePopup");
    }

    if (ImGui::BeginPopup("AnimTypePopup"))
    {
        if (ImGui::MenuItem("Animation"))
            iAnimType = 0;

        if (ImGui::MenuItem("Montage"))
            iAnimType = 1;

        ImGui::EndPopup();
    }

    ImGui::SameLine();


    // Save 관련 static 상태들
    static bool s_bOpenRenamePopup = false;
    static bool s_bOpenSaveFilePopup = false;
    static bool s_bOpenSaveConfirmPopup = false;

    static char s_szRenameBuffer[256] = "";
    static char s_szSaveNameBuffer[256] = "";

    static std::filesystem::path s_SaveTargetPath;

    // ------------------------------------------------------------
    // Save 버튼
    // ------------------------------------------------------------
    if (ImGui::Button("Save", ImVec2(100.f, 28.f)))
    {
        ImGui::OpenPopup("SaveAnimAction");
    }

    // ------------------------------------------------------------
    // 1단계: Rename 할 건지, 실제 Save 할 건지 선택
    // ------------------------------------------------------------
    if (ImGui::BeginPopupModal("SaveAnimAction", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Select Save Action");
        ImGui::Separator();

        if (ImGui::Button("Rename Only", ImVec2(180.f, 28.f)))
        {
            s_bOpenRenamePopup = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Spacing();

        if (ImGui::Button("Save New File", ImVec2(180.f, 28.f)))
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

    // CloseCurrentPopup() 직후 바로 OpenPopup()이 안 먹을 수 있어서 플래그로 다음 프레임에 열기
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
    // Rename Only 팝업
    // 파일 이름만 변경
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
                auto animations = pComModelInstance->GetModel()->GetAnimations();

                if (!animations.empty() && iSelected < animations.size())
                {
                    auto pAnim = animations[iSelected];

                    if (pAnim && s_szRenameBuffer[0] != '\0')
                    {
                        std::string oldPath = pAnim->GetAnimPath();
                        std::string newPath;

                        // 실제 파일 이름 변경
                        if (RenameAnimFile_Overwrite(oldPath, s_szRenameBuffer, newPath))
                        {
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
    // Save New File 팝업
    // 저장할 새 파일 이름 입력
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
                auto animations = pComModelInstance->GetModel()->GetAnimations();

                if (!animations.empty() && iSelected < animations.size())
                {
                    auto pAnim = animations[iSelected];

                    if (pAnim && s_szSaveNameBuffer[0] != '\0')
                    {
                        std::filesystem::path oldPath = pAnim->GetAnimPath();
                        std::filesystem::path saveName = s_szSaveNameBuffer;

                        // 확장자 안 적으면 기존 확장자 붙이기
                        if (saveName.extension().empty())
                        {
                            saveName += oldPath.extension().string();
                        }

                        s_SaveTargetPath = oldPath.parent_path() / saveName;

                        // 바로 저장하지 않고 확인 팝업으로 넘김
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
    // 최종 저장 확인 팝업
    // 여기서 Yes 눌러야 실제 저장됨
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
            uint32_t iSelected = pComAnimator->GetPlayAnimIndex();

            auto pComModelInstance = pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");

            if (pComModelInstance && pComModelInstance->GetModel())
            {
                auto animations = pComModelInstance->GetModel()->GetAnimations();

                if (!animations.empty() && iSelected < animations.size())
                {
                    auto pAnim = animations[iSelected];

                    if (pAnim)
                    {
                        if (WriteSaveBakedBinary(
                            s_SaveTargetPath.parent_path().string(),
                            s_SaveTargetPath.stem().string()))
                        {
                        }
                    }
                }
            }

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


    


     
    struct LOAD_ANIM_FILE_DESC
        {
            std::filesystem::path filePath;
            std::string fileName;
        };

    static std::vector<LOAD_ANIM_FILE_DESC> s_LoadAnimFiles;
    static std::filesystem::path s_LoadAnimFolder;

    auto RefreshLoadAnimFileList =
        [&](const std::filesystem::path& animFolder,
            const std::vector<SPtr<CResModelAnim>>& animations)
            {
                s_LoadAnimFiles.clear();
                s_LoadAnimFolder = animFolder;

                if (s_LoadAnimFolder.empty())
                    return;

                std::error_code ec;

                if (!std::filesystem::exists(s_LoadAnimFolder, ec))
                    return;

                for (auto& entry : std::filesystem::directory_iterator(s_LoadAnimFolder, ec))
                {
                    if (ec)
                        break;

                    if (!entry.is_regular_file())
                        continue;

                    std::filesystem::path filePath = entry.path();

                    if (filePath.extension() != ".bin")
                        continue;

                    if (IsAlreadyLoadedAnim(animations, filePath))
                        continue;

                    LOAD_ANIM_FILE_DESC desc{};
                    desc.filePath = filePath;
                    desc.fileName = filePath.filename().string();

                    s_LoadAnimFiles.push_back(desc);
                }
            };
    
    if (ImGui::Button("Load", ImVec2(100.f, 28.f)))
    {
        auto pSampleObj =
            CGameInstance::Get().GetGameObjectByHandle(m_hTestModel);

        auto pComModelInstance =
            pSampleObj ? pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance") : nullptr;

        auto pComAnimator =
            pSampleObj ? pSampleObj->GetComponent<CComAnimator>("ComCModelAnimator") : nullptr;

        if (pComModelInstance && pComModelInstance->GetModel() && pComAnimator)
        {
            auto pModel = pComModelInstance->GetModel();
            auto animations = pModel->GetAnimations();

            std::filesystem::path animFolder;

            if (!animations.empty())
            {
                uint32_t iSelected = pComAnimator->GetPlayAnimIndex();

                if (iSelected < animations.size() && animations[iSelected])
                {
                    animFolder =
                        std::filesystem::path(animations[iSelected]->GetAnimPath()).parent_path();
                }
            }

            s_LoadAnimFolder = animFolder;

   
        }

        ImGui::OpenPopup("LoadAnim");
    }

    if (ImGui::BeginPopupModal("LoadAnim", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Anim Folder:");
        ImGui::Text("%s", s_LoadAnimFolder.string().c_str());
        ImGui::Separator();

        if (s_LoadAnimFiles.empty())
        {
            ImGui::Text("No anim files found.");
        }
        else
        {
            for (auto& desc : s_LoadAnimFiles)
            {
                if (ImGui::Selectable(desc.fileName.c_str()))
                {
                    auto pSampleObj =
                        CGameInstance::Get().GetGameObjectByHandle(m_hTestModel);

                    auto pComModelInstance =
                        pSampleObj ? pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance") : nullptr;

                    if (pComModelInstance && pComModelInstance->GetModel())
                    {
                        auto pModel = pComModelInstance->GetModel();

                        //// 실제 애니메이션 로드
                        //pModel->LoadAnim(desc.filePath.string());

                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Refresh", ImVec2(100.f, 28.f)))
        {
            auto pSampleObj =
                CGameInstance::Get().GetGameObjectByHandle(m_hTestModel);

            auto pComModelInstance =
                pSampleObj ? pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance") : nullptr;

            if (pComModelInstance && pComModelInstance->GetModel())
            {
                auto animations = pComModelInstance->GetModel()->GetAnimations();

                RefreshLoadAnimFileList(s_LoadAnimFolder, animations);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(100.f, 28.f)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
    ImGui::SameLine();

    if (ImGui::Button("Create", ImVec2(100.f, 28.f)))
    {
    }

	pComAnimator->SetAnimationTYPE(static_cast<CComAnimator::ANIMTYPE>(iAnimType));

    ImGui::SameLine();
    if (iAnimType == 0) {
        auto pComModelInstance =
            pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");

        if (pComModelInstance->GetModel()->GetAnimations().size() == 0)
            return;

        auto animations = pComModelInstance->GetModel()->GetAnimations();
        ImGui::Text("Current : %s", animations[pComAnimator->GetPlayAnimIndex()]->GetAnimName().c_str());
    }


    ImGui::End();

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
    float fCurrentPos = pAnim->GetCurrentTrackPosition();
    float fTPS = pAnim->GetTickPerSecond();

    ImGui::Text("Animation Timeline");

    ImGui::SameLine();

    if (ImGui::Button(pComAnimator->GetPlay() ? "Play" : "Pause"))
    {
		//민수한태 알릴거
        pComAnimator->SetPlay(true);
    }

    ImGui::PushItemWidth(-1.f);


    if (ImGui::SliderFloat("##AnimTimeline",&fCurrentPos,0.f,fDuration, "%.3f") )
    {
        if (pComAnimator->GetPlay() == true) {

            pAnim->SetCurrentTrackPosition(fCurrentPos);
        }
        else if (pComAnimator->GetPlay() == false) {
            pAnim->SetCurrentTrackPosition(fCurrentPos);
            pComAnimator->AnimEditor_Play_AnimResource(m_fTimeDelta, pComAnimator->GetPlayAnimIndex());
        }
        
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

    if (ImGui::TreeNode("Animation"))
    {
        for (uint32_t i = 0; i < animations.size(); ++i)
        {
            auto pAnim = animations[i];

            if (!pAnim)
                continue;

            bool bSelected = (pComAnimator->GetPlayAnimIndex() == i);

 
            if (ImGui::Selectable(pAnim->GetAnimName().c_str(), bSelected))
            {
                pComAnimator->SetPlayAnimIndex(i);

          

            }
        }

        ImGui::TreePop();
    }


    ImGui::End();
}

void CAnimEdit_Manager::IMGUI_Select_Detail_Data()
{
    

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
    float fCurrentPos = pAnim->GetCurrentTrackPosition();

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
    if (&m_hTestModel == nullptr)
        return;

	IMGUI_Select_AnimType();
    IMGUI_Slider_Animation();
    IMGUI_Select_Animation();
    IMGUI_Select_Detail_Data();
    IMGUI_Speed_Animation();
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
