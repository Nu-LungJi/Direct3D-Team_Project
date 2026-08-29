#include "pch.h"
#include "Player_Weapon.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Resources.h"
#include "GameInstance.h"
#include "ComModelInstance.h"
#include "ResModelBone.h"
#include "Trail_CPU.h"
NS_USING(Client)

namespace
{
	constexpr const _char* WAND2_MODEL_PATH =
		"./Resources/SampleClient/Models/Skeleton/Wand2/SK_Wand2.bin";
	constexpr const _char* WAND2_RESOURCE_TAG =
		"PLAYER_WEAPON_SKELETON_RESOURCE_WAND2";
	constexpr _float WAND_SMOKE_SPAWN_INTERVAL = 0.2f;
}

CPlayer_Weapon::CPlayer_Weapon()
	: CGameObject{}
{
}

CPlayer_Weapon::~CPlayer_Weapon()
{
}

void CPlayer_Weapon::UpdateGUI()
{
	CGameObject::UpdateGUI();

}

HRESULT CPlayer_Weapon::InitializePrototype(void* pArg)
{

	m_pResVertexNonAnimShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim");
	if (FAILED(m_pResVertexNonAnimShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelNonAnimShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	if (FAILED(m_pResPixelNonAnimShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CPlayer_Weapon::Initialize(void* pArg)
{

	auto pDesc = static_cast<WEAPON_DESC*>(pArg);
	m_iBoneSocketIndex = pDesc->iBoneIndex;
	m_iSpawnBoneIndex = pDesc->iSpawnBoneIndex;
	m_ParentHandle = pDesc->ParentHandle;

	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	if (!CGameInstance::Get().GetResourceFirst<CResModel>(
		pDesc->LevelTag, pDesc->WeaponName))
	{
		auto socketModel = CGameInstance::Get().AddResourceT<CResModel>(
			pDesc->LevelTag,
			pDesc->WeaponName,
			CResModel::Create("./Resources/SampleClient/Models/Skeleton/Wand/SK_Wand.bin"));
		if (!socketModel)
			return E_FAIL;
		CResModel::DESC modelDesc{};
		modelDesc.PreTransformMatrix = XMMatrixRotationX(XMConvertToRadians(-90.f));
		if (FAILED(socketModel->Load(modelDesc)))
			return E_FAIL;
	}

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}

	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = pDesc->LevelTag;
		Desc.sResTag = pDesc->WeaponName;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	if (FAILED(RefreshModelBones()))
		return E_FAIL;

	XMStoreFloat4x4(&m_ParentMatrix, XMMatrixIdentity());
	GetTransform().SetScale(_float3{ 4.f,4.f,4.f });
	GetTransform().SetRotationEuler({ -90.f, 0.f, 0.f });

	//test = CGameInstance::Get().Parse_Command("FireSparkQueue.json");

	return S_OK;
}

HRESULT CPlayer_Weapon::EquipWand2()
{
	return EquipWandModel(WAND2_MODEL_PATH, WAND2_RESOURCE_TAG);
}

HRESULT CPlayer_Weapon::EquipWandModel(
	const _string& strModelPath,
	const _string& strResourceTag)
{
	if (!m_pComModelInstance || strModelPath.empty() || strResourceTag.empty())
		return E_INVALIDARG;

	const E::StringID groupTag = m_pComModelInstance->Get_GroupTag();
	const E::StringID resourceTag{ strResourceTag };

	auto model =
		CGameInstance::Get().GetResourceFirst<CResModel>(groupTag, resourceTag);
	if (!model)
	{
		model = CGameInstance::Get().AddResourceT<CResModel>(
			groupTag,
			resourceTag,
			CResModel::Create(strModelPath));
		if (!model)
			return E_FAIL;
	}

	// A failed first load leaves the resource registered under its tag. Always
	// call Load so restoring a missing file can recover on the next attempt.
	CResModel::DESC modelDesc{};
	modelDesc.PreTransformMatrix =
		XMMatrixRotationX(XMConvertToRadians(-90.f));
	if (FAILED(model->Load(modelDesc)))
		return E_FAIL;

	const E::StringID previousGroupTag = m_pComModelInstance->Get_GroupTag();
	const E::StringID previousResourceTag = m_pComModelInstance->Get_ResTag();

	if (FAILED(m_pComModelInstance->ChangeModel(groupTag, resourceTag)))
		return E_FAIL;

	if (FAILED(RefreshModelBones()))
	{
		m_pComModelInstance->ChangeModel(previousGroupTag, previousResourceTag);
		RefreshModelBones();
		return E_FAIL;
	}

	m_bWand2Equipped = strResourceTag == WAND2_RESOURCE_TAG;
	m_fWandSmokeSpawnTime = WAND_SMOKE_SPAWN_INTERVAL;

	return S_OK;
}

HRESULT CPlayer_Weapon::RefreshModelBones()
{
	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return E_FAIL;

	const auto& bones = m_pComModelInstance->GetModel()->GetBones();
	auto& combinedBones = m_pComModelInstance->Get_CombinedBoneMatrices();
	combinedBones.clear();
	combinedBones.resize(bones.size());

	for (size_t i = 0; i < bones.size(); ++i)
	{
		if (!bones[i])
			return E_FAIL;

		bones[i]->Update_CombinedTransformationMatrix(
			bones, XMMatrixIdentity());
		combinedBones[i] =
			*bones[i]->Get_CombinedTransformationMatrixPtr();
	}

	m_iMuzzleSocketBoneIndex =
		m_pComModelInstance->GetModel()->Get_BoneIndex("MuzzleSocket");
	return m_iMuzzleSocketBoneIndex >= 0 ? S_OK : E_FAIL;
}
void CPlayer_Weapon::PriorityUpdate(E::_float fTimeDelta)
{
}

void CPlayer_Weapon::Update(E::_float fTimeDelta)
{
#ifdef _DEBUG
	if (CGameInstance::Get().KeyDown(DIK_I))
		EquipWand2();
#endif
}

void CPlayer_Weapon::LateUpdate(E::_float fTimeDelta)
{
	_bool bHandWorldUpdated = false;
	if (auto iter = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle))
	{
		if (!m_bThrow)
		{
			if (auto pModel = GetParentModelInstance())
			{
				if (m_iBoneSocketIndex >= 0 &&
					static_cast<size_t>(m_iBoneSocketIndex) <
					pModel->Get_CombinedBoneMatrices().size())
				{
					_matrix Par = XMLoadFloat4x4(&pModel->Get_CombinedBoneMatrices()[m_iBoneSocketIndex]);
					for (uint32_t i = 0; i < 3; ++i)
					{
						Par.r[i] = XMVector3Normalize(Par.r[i]);
					}
					XMStoreFloat4x4(&m_ParentMatrix, Par * XMLoadFloat4x4(pModel->GetGameObject()->GetTransform().GetWorldMatrix()));
					bHandWorldUpdated = true;
				}
			}
		}

	}

	GetTransform().SetParentWorldMatrix(m_ParentMatrix);
	GetTransform().Update();
	if (m_bWand2Equipped && bHandWorldUpdated)
	{
		m_fWandSmokeSpawnTime += std::max(0.f, fTimeDelta);
		if (m_fWandSmokeSpawnTime >= WAND_SMOKE_SPAWN_INTERVAL)
		{
			m_fWandSmokeSpawnTime = 0.f;
			CGameInstance::Get().Spawn(
				"WandSmokeLoop.json", m_ParentMatrix);
		}
	}
	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);

	/*----------- 광윤 추가 -----------*/
	CGameInstance::Get().AddShadowRenderGroup(ACTORTYPE::DYNAMIC, this);
	/*---------------------------------*/
}

CComModelInstance* CPlayer_Weapon::GetParentModelInstance() const
{
	auto* pParent = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle);
	if (!pParent)
		return nullptr;

	// [LSY] 기존 Player는 과거 오타가 포함된 태그를 사용하고, 신규 Pawn은
	// 정상 태그를 사용한다. 양쪽을 허용해 기존 장비 부착을 깨지 않는다.
	if (auto* pModel =
		pParent->GetComponent<CComModelInstance>("ComCModelIntance"))
	{
		return pModel;
	}

	return pParent->GetComponent<CComModelInstance>("ComCModelInstance");
}

HRESULT CPlayer_Weapon::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	{
		E::CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}

	const auto& vs = m_pResVertexNonAnimShader;
	const auto& ps = m_pResPixelNonAnimShader;

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	auto pModel = m_pComModelInstance->GetModel();

	uint32_t	iNumMeshes = pModel->Get_NumMeshes();
	for (uint32_t i = 0; i < iNumMeshes; ++i) {
		const auto& viBuffer = pModel->GetMeshes()[i];
		if (FAILED(m_pComModelInstance->Bind_BoneMatrices(pContext, i)))
			return E_FAIL;


		ID3D11Buffer* vertexBuffers[] = {
				viBuffer->GetVertexBuffer().Get()
		};
		uint32_t strides[] = {
			viBuffer->GetVertexStride()
		};
		uint32_t offsets[] = {
			0
		};
		pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		{
			m_pComModelInstance->Bind_Textures(pContext, i);
			m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
		}

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}


	return S_OK;
}

/*----------- 광윤 추가 -----------*/
HRESULT CPlayer_Weapon::Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject)
		return E_FAIL;

	E::CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))	return E_FAIL;

	pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());

	const auto model = m_pComModelInstance->GetModel();
	if (!model)	return E_FAIL;

	for (uint32_t i = 0; i < model->Get_NumMeshes(); ++i)
	{
		const auto& viBuffer = model->GetMeshes()[i];
		ID3D11Buffer* vertexBuffer = viBuffer->GetVertexBuffer().Get();
		const uint32_t stride = viBuffer->GetVertexStride();
		const uint32_t offset = 0;

		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());
		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	ID3D11ShaderResourceView* pSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
	pContext->PSSetShaderResources(0, 4, pSRVs);

	return S_OK;
}
bool CPlayer_Weapon::GetShadowBounds(BoundingBox& OutBounds) const {
	return false;
}
/*---------------------------------*/

_float4x4 CPlayer_Weapon::GetSpawnWorldMatrix() const
{
	if (m_pComModelInstance && m_iMuzzleSocketBoneIndex >= 0)
	{
		const auto& combinedBones = m_pComModelInstance->Get_CombinedBoneMatrices();
		if (static_cast<size_t>(m_iMuzzleSocketBoneIndex) < combinedBones.size())
		{
			_matrix weaponWorld = GetTransform().GetLoadedCombinedWorldMatrix();

			// Player and weapon LateUpdate ordering is not guaranteed. Build the
			// attachment matrix from the player's current-frame hand bone here so
			// wand-tip effects never consume the weapon's previous-frame transform.
			if (auto* pParent = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle))
			{
				if (auto* pParentModel = GetParentModelInstance())
				{
					const auto& parentBones = pParentModel->Get_CombinedBoneMatrices();
					if (m_iBoneSocketIndex >= 0 &&
						static_cast<size_t>(m_iBoneSocketIndex) < parentBones.size())
					{
						_matrix handSocket = XMLoadFloat4x4(&parentBones[m_iBoneSocketIndex]);
						for (uint32_t i = 0; i < 3; ++i)
							handSocket.r[i] = XMVector3Normalize(handSocket.r[i]);

						weaponWorld = GetTransform().GetLoadedWorldMatrix() *
							handSocket * pParent->GetTransform().GetLoadedWorldMatrix();
					}
				}
			}

			_float4x4 result{};
			XMStoreFloat4x4(&result,
				XMLoadFloat4x4(&combinedBones[m_iMuzzleSocketBoneIndex]) *
				weaponWorld);
			return result;
		}
	}

	return *GetTransform().GetCombinedWorldMatrix();
}

E::UPtr<CPlayer_Weapon> CPlayer_Weapon::Create()
{
	auto pInstance = E::ToUPtr(new CPlayer_Weapon{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CPlayer_Weapon");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CPlayer_Weapon::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CPlayer_Weapon{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CPlayer_Weapon");
		return nullptr;
	}

	return pInstance;
}
