#include "pch.h"
#include "AnimEdit_Manager.h"
#include  "GameObject.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "ResModel.h"
NS_USING(Engine)


CAnimEdit_Manager::CAnimEdit_Manager()
{
}
CAnimEdit_Manager::~CAnimEdit_Manager()
{

}

HRESULT CAnimEdit_Manager::Initilize()
{

	

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


uint32_t CAnimEdit_Manager::GetAnimIndex()
{
    int m_iSelectedAnimIndex = -1;
    auto pSampleObj = CGameInstance::Get().GetGameObjectByHandle(m_hTestModel);
    if (pSampleObj == nullptr)
        return m_iSelectedAnimIndex;

    auto pComModelInstance =
        pSampleObj->GetComponent<CComModelInstance>("ComCModelIntance");

    auto pComAnimator =
        pSampleObj->GetComponent<CComAnimator>("ComCModelAnimator");

    if (nullptr == pComAnimator)
        return m_iSelectedAnimIndex;

    if (ImGui::BeginPopupModal("SelectAnimation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
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
                
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
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

    // 앞으로 추가될 버튼들
    if (ImGui::Button("Save", ImVec2(100.f, 28.f)))
    {
    }

    ImGui::SameLine();

    if (ImGui::Button("Load", ImVec2(100.f, 28.f)))
    {
    }

    ImGui::SameLine();

    if (ImGui::Button("Create", ImVec2(100.f, 28.f)))
    {
    }

	pComAnimator->SetAnimationTYPE(static_cast<CComAnimator::ANIMTYPE>(iAnimType));
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

    ImGui::PushItemWidth(-1.f);


    if (ImGui::SliderFloat("##AnimTimeline",&fCurrentPos,0.f,fDuration, "%.3f"))
    {
        pAnim->SetCurrentTrackPosition(fCurrentPos);
        
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

            if (ImGui::Selectable( pAnim->GetAnimName().c_str(), bSelected))
            {
                pComAnimator->SetPlayAnimIndex(i);
            }
        }

        ImGui::TreePop();
    }

    ImGui::End();
}

void CAnimEdit_Manager::IMGUI_TestGetAnimIndex()
{
    ImGui::Begin("Anim Index Test");

    if (ImGui::Button("Select Animation"))
    {
        ImGui::OpenPopup("SelectAnimation");
    }

    uint32_t iIndex = GetAnimIndex();

    if (iIndex != static_cast<uint32_t>(-1))
    {
        ImGui::Text("Selected Index : %u", iIndex);
    }

    ImGui::End();

}
void CAnimEdit_Manager::UpdateGUI()
{
    if (&m_hTestModel == nullptr)
        return;

	IMGUI_Select_AnimType();
    IMGUI_Slider_Animation();
    IMGUI_Select_Animation();
    IMGUI_TestGetAnimIndex();
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
