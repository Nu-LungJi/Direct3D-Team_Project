#include "pch.h"
#include "GameInstance.h"
#include "ComSocket.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
#include "ComTransform.h"
NS_USING(Engine)



void CComSocket::UpdateGUI()
{

}

CComSocket::CComSocket()
{


}

CComSocket::~CComSocket()
{
}


HRESULT CComSocket::Initialize(void* pArg)
{
	if (FAILED(CComponent::Initialize(pArg)))
	{
		return E_FAIL;
	}
	CComSocket::DESC* pDesc = reinterpret_cast<CComSocket::DESC*>(pArg);
	// 오브젝트가 가지고 있는 핸들은 무조건 있어야됨
	m_pOwner = pDesc->m_pOwner;
	// Bone Socket 위치 알기 위한 이름
	m_iBoneIndex	=	pDesc->iBoneIndex;
	m_fOffset		=	pDesc->m_fOffset;
	m_sModelInstanceName = pDesc->sModelInstanceName;
	m_sAnimatorName = pDesc->sAnimationName;
	return S_OK;
}

_float4x4& CComSocket::Get_Socket_Matrix()
{
	return m_SocketMatrix;
}

_bool CComSocket::Get_Socket_MatrixAtPose(int32_t iAnimIndex, _float fTrackPosition, _float4x4& OutSocketMatrix) const
{
	ZoneScopedN("Update Socket_Matrix");

	auto* pObj = CGameInstance::Get().GetGameObjectByHandle(m_pOwner);
	if (!pObj)
		return false;

	auto* pModelInstance = pObj->GetComponent<CComModelInstance>(m_sModelInstanceName);
	auto* pAnimator = pObj->GetComponent<CComAnimator>(m_sAnimatorName);
	if (pModelInstance == nullptr || pAnimator == nullptr)
		return false;

	if (m_BoneChain.size() == 0) {
		const auto pModel = pModelInstance->GetModel();

		if (!pModel)
			return false;

		BuildBoneChain(*pModel);
	}

	_float4x4 combinedBoneMatrices;
	if (!pAnimator->Sample_CombinedBoneMatrices(iAnimIndex, fTrackPosition, m_BoneChain, combinedBoneMatrices))
	{
		return false;
	}

	const _matrix matOffset = XMMatrixTranslation(m_fOffset.x, m_fOffset.y, m_fOffset.z);
	const _matrix matBone = XMLoadFloat4x4(&combinedBoneMatrices);

	XMStoreFloat4x4(&OutSocketMatrix, matOffset * matBone);
	return true;
}

void CComSocket::BuildBoneChain(const CResModel& model) const
{
	m_BoneChain.clear();

	const auto& bones = model.GetBones();
	if (m_iBoneIndex >= bones.size())
		return;

	for (int32_t bone = static_cast<int32_t>(m_iBoneIndex);
		bone >= 0;
		bone = bones[bone]->GetParendBoneIndex())
	{
		m_BoneChain.push_back(static_cast<uint32_t>(bone));
	}

	std::reverse(m_BoneChain.begin(), m_BoneChain.end());
}

UPtr<CComSocket> CComSocket::Create()
{
	auto pInstance = ToUPtr(new CComSocket{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComSocket");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CComSocket::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComSocket{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: CComSocket");
		return nullptr;
	}
	return pInstance;
}


