#include "pch.h"
#include "GameInstance.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
#include "ResModelAnim.h"
#include "ResModel.h"
NS_USING(Engine)



CComAnimator::CComAnimator()
{


}

CComAnimator::~CComAnimator()
{
}


HRESULT CComAnimator::Initialize(void* pArg)
{
    if (FAILED(CComponent::Initialize(pArg)))
    {
        return E_FAIL;
    }

    if (pArg != nullptr) {
        CComAnimator::DESC* pDesc = reinterpret_cast<CComAnimator::DESC*>(pArg);
           
        m_Comtag = pDesc->sComTag;
        m_pModelInstance = GetGameObject()->GetComponent<CComModelInstance>(m_Comtag);
    }


    return S_OK;
}

HRESULT CComAnimator::Update(_float fTimeDelta)
{
    if (m_bPlay) {
        switch (m_iPlayAnimationType) {
        case ANIMTYPE::MONTAGE: {
            Play_AnimationMontage(fTimeDelta, "");
        }
                              break;
        case ANIMTYPE::ANIM: {
            AnimEditor_Play_AnimResource(fTimeDelta, m_iPlayAnimIndex);

            m_fRatio = m_pModelInstance->GetModel()->GetAnimations()[m_iPlayAnimIndex]->GetCurrentTrackPosition()
                / m_pModelInstance->GetModel()->GetAnimations()[m_iPlayAnimIndex]->GetDuration();

        }
         break;
        }
    }


    return S_OK;
}

HRESULT CComAnimator::Play_AnimationMontage(_float fTimeDelta, const std::string& strAnimMontageName)
{
    return S_OK;
}


HRESULT CComAnimator::AnimEditor_Play_AnimResource(_float fTimeDelta, uint32_t iModelAnimNum)
{
	auto pModel = GetGameObject()->GetComponent<CComModelInstance>(m_Comtag)->GetModel();
    
    if(pModel == nullptr)
		return E_FAIL;
    


    auto& pAnim = pModel->GetAnimations();
    auto& m_PreTransformMatrix = pModel->Get_PreTransformMatrix();

    _bool           isFinished = { false };

    /* 뼈들의 m_TransformationMatrix를 갱신해준다. */
    isFinished = pAnim[iModelAnimNum]->Update_TransformationMatrices(fTimeDelta, pModel->GetBones(), true);

    for (auto& pBone : pModel->GetBones())
    {
        pBone->Update_CombinedTransformationMatrix(pModel->GetBones(), XMLoadFloat4x4(&m_PreTransformMatrix));
    }


    return isFinished;

}

HRESULT CComAnimator::AnimEditor_Play_AnimMontage(_float fTimeDelta, const std::string& strAnimMontageName)
{
    return S_OK;
}

UPtr<CComAnimator> CComAnimator::Create()
{
    auto pInstance = ToUPtr(new CComAnimator{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComAnimator");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComAnimator::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComAnimator{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComAnimator");
        return nullptr;
    }
    return pInstance;
}
