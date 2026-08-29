#include "pch.h"
#include "AnimatedWorldObject.h"
#include "ComAnimator.h"
#include "ComBeHavior.h"
#include "ComModelInstance.h"
#include "GameInstance.h"
#include "Resources.h"

NS_USING(Client)

void CAnimatedWorldObject::UpdateGUI()
{
	CAnimationObject::UpdateGUI();
	ImGui::Text("Instance submitted: %s", m_bSubmittedThisFrame ? "YES" : "NO");
	ImGui::Text("Instanced render reached: %s", m_bRenderedLastFrame ? "YES" : "NO");
	ImGui::Text("Last batch: %u", m_iLastBatchInstanceCount);
	ImGui::Text("Submit / Render: %llu / %llu",
				(unsigned long long)m_iInstanceSubmitCount,
				(unsigned long long)m_iInstancedRenderCount);
}

HRESULT CAnimatedWorldObject::InitializePrototype(void*)
{
	auto& game = CGameInstance::Get();
	m_pInstancedVertexShader = game.GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER,
																	   "VS_TestModelAnim_CPU_Skinning_Instanced");
	m_pPixelShader = game.GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	m_pSkinMeshCBuffer = game.GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_GPU_SKIN_MESH");
	return (!m_pInstancedVertexShader || FAILED(m_pInstancedVertexShader->Load()) || !m_pPixelShader ||
			FAILED(m_pPixelShader->Load()) || !m_pSkinMeshCBuffer)
			   ? E_FAIL
			   : S_OK;
}

HRESULT CAnimatedWorldObject::Initialize(void* pArg)
{
	if (!pArg || FAILED(CAnimationObject::Initialize(pArg)))
		return E_FAIL;
	const auto* desc = static_cast<DESC*>(pArg);
	CComModelInstance::DESC model{};
	model.sGroupTag = desc->sModelGroupTag;
	model.sResTag = desc->sModelResourceTag;
	if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ModelInstance", "Com_AnimatedObjectModel", &model, &m_pModelInstance)))
		return E_FAIL;
	CComAnimator::DESC animator{};
	animator.sComTag = "Com_AnimatedObjectModel";
	if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_Animator", "Com_AnimatedObjectAnimator", &animator, &m_pAnimator)))
		return E_FAIL;

	if (!desc->sBehaviorMajorTag.empty() || !desc->sBehaviorMinorTag.empty())
	{
		CComBeHavior::BEHAVIOR_DESC behaviorDesc{};
		behaviorDesc.OwnerName = "Com_AnimatedObjectBT";
		behaviorDesc.resBeHaviorMajor = desc->sBehaviorMajorTag;
		behaviorDesc.resBeHaviorMinor = desc->sBehaviorMinorTag;
		if (FAILED(AddComponentFromProto(
				"BEHAVIOR",
				"Prototype_Component_BeHavior",
				"Com_AnimatedObjectBT",
				&behaviorDesc,
				&m_pBehavior)))
			return E_FAIL;
	}
	m_ParentHandle = desc->ParentHandle;
	m_iParentBoneIndex = desc->iParentBoneIndex;
	m_bLockLocalRotation = desc->bLockLocalRotation;
	m_vLockedLocalRotation = desc->vRotation;
	m_fDissolveAppearDuration = std::max(desc->fDissolveAppearDuration, 0.f);
	m_fDissolveAppearElapsed = 0.f;
	m_fDissolveIntensity = m_fDissolveAppearDuration > 0.f ? 1.f : 0.f;

	m_pAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pAnimator->Build_BoneMatrices_CPU(0.f);
	GetTransform().SetPosition(XMLoadFloat3(&desc->vPosition));
	GetTransform().SetScale(desc->vScale);
	const _matrix rotation = XMMatrixRotationX(XMConvertToRadians(desc->vRotation.x)) *
							 XMMatrixRotationY(XMConvertToRadians(desc->vRotation.y)) *
							 XMMatrixRotationZ(XMConvertToRadians(desc->vRotation.z));
	GetTransform().SetQuaternion(XMQuaternionRotationMatrix(rotation));
	if (m_bLockLocalRotation)
		GetTransform().SetRotationEuler(m_vLockedLocalRotation);
	GetTransform().Update();
	if (m_bLockLocalRotation)
		GetTransform().SetRotationEuler(m_vLockedLocalRotation);
	if (!PlayAnimation(desc->sAnimationName, desc->bLoop, desc->fAnimationSpeed, desc->fStartRatio))
		return E_FAIL;
	if (!desc->bAutoPlay)
		SetAnimationPaused(true);
	return S_OK;
}

void CAnimatedWorldObject::PriorityUpdate(E::_float delta)
{
	m_bSubmittedThisFrame = false;
	if (m_pBehavior)
		m_pBehavior->Update(delta);
}
void CAnimatedWorldObject::Update(E::_float delta)
{
	CAnimationObject::Update(delta);
	if (m_fDissolveAppearDuration > 0.f && m_fDissolveIntensity > 0.f)
	{
		m_fDissolveAppearElapsed += std::max(delta, 0.f);
		m_fDissolveIntensity = 1.f - std::clamp(
			m_fDissolveAppearElapsed / m_fDissolveAppearDuration, 0.f, 1.f);
	}
	if (m_pAnimator && m_pModelInstance && !m_pModelInstance->GetModel()->GetAnimations().empty())
		m_pAnimator->Update(delta);
	if (m_pBehavior)
		m_pBehavior->AbortNode();
}
void CAnimatedWorldObject::LateUpdate(E::_float)
{
	if (m_ParentHandle.IsValid())
	{
		if (auto* parent = CGameInstance::Get().GetGameObjectByHandle(m_ParentHandle))
		{
			_matrix parentWorld = parent->GetTransform().GetLoadedWorldMatrix();
			if (m_iParentBoneIndex >= 0)
			{
				if (auto* parentModel = parent->GetComponent<CComModelInstance>("ComCModelIntance"))
				{
					const auto& bones = parentModel->Get_CombinedBoneMatrices();
					if (static_cast<size_t>(m_iParentBoneIndex) < bones.size())
					{
						_matrix socket = XMLoadFloat4x4(&bones[m_iParentBoneIndex]);
						for (uint32_t axis = 0; axis < 3; ++axis)
							socket.r[axis] = XMVector3Normalize(socket.r[axis]);
						parentWorld = socket * parentWorld;
					}
				}
			}
			_float4x4 storedParent{};
			XMStoreFloat4x4(&storedParent, parentWorld);
			GetTransform().SetParentWorldMatrix(storedParent);
		}
	}
	if (m_bLockLocalRotation)
		GetTransform().SetRotationEuler(m_vLockedLocalRotation);
	GetTransform().Update();
	if (m_bLockLocalRotation)
		GetTransform().SetRotationEuler(m_vLockedLocalRotation);
	if (!m_pModelInstance || !m_pAnimator || !m_pModelInstance->GetModel() ||
		m_pModelInstance->GetModel()->GetAnimations().empty())
		return;
	uint32_t dissolveBits{};
	static_assert(sizeof(dissolveBits) == sizeof(m_fDissolveIntensity));
	memcpy(&dissolveBits, &m_fDissolveIntensity, sizeof(dissolveBits));
	CGameInstance::Get().Add_Instance(
		m_pModelInstance, m_pAnimator,
		*GetTransform().GetCombinedWorldMatrix(), dissolveBits);
	m_bSubmittedThisFrame = true;
	++m_iInstanceSubmitCount;
}

_bool CAnimatedWorldObject::PlayAnimation(const _string& name, _bool loop, _float speed, _float startRatio)
{
	if (!m_pAnimator || !m_pModelInstance)
		return false;
	const auto& animations = m_pModelInstance->GetModel()->GetAnimations();
	if (animations.empty())
		return false;
	const auto found =
		std::ranges::find_if(animations, [&](const auto& anim) { return anim && anim->GetAnimName() == name; });
	const int32_t index =
		name.empty() ? 0 : (found == animations.end() ? -1 : (int32_t)std::distance(animations.begin(), found));
	if (index < 0)
		return false;
	m_pAnimator->Play_Anim(index, loop, 0.f);
	auto& state = m_pAnimator->GetCurAnimState();
	state.fSpeed = std::max(0.f, speed);
	m_pAnimator->SetTrackPosition(state.fDuration * std::clamp(startRatio, 0.f, 1.f));
	m_pAnimator->SetPlay(true);
	return true;
}
void CAnimatedWorldObject::SetAnimationPaused(_bool paused)
{
	if (m_pAnimator)
		m_pAnimator->SetPlay(!paused);
}

void CAnimatedWorldObject::ApplyTransform(
	const _float3& position,
	const _float3& rotation,
	const _float3& scale)
{
	GetTransform().SetPosition(XMLoadFloat3(&position));
	GetTransform().SetScale(scale);

	const _matrix rotationMatrix =
		XMMatrixRotationX(XMConvertToRadians(rotation.x)) *
		XMMatrixRotationY(XMConvertToRadians(rotation.y)) *
		XMMatrixRotationZ(XMConvertToRadians(rotation.z));

	GetTransform().SetQuaternion(XMQuaternionRotationMatrix(rotationMatrix));
	GetTransform().Update();
}
void CAnimatedWorldObject::StopAnimation()
{
	if (m_pAnimator)
	{
		m_pAnimator->SetPlay(false);
		m_pAnimator->SetTrackPosition(0.f);
		m_pAnimator->Build_BoneMatrices_CPU(0.f);
	}
}

HRESULT CAnimatedWorldObject::UpdateInstanceBuffer(ID3D11DeviceContext* context,
												   const std::vector<GPU_ANIM_INSTANCE_DATA>& instances)
{
	m_iLastBatchInstanceCount = (uint32_t)instances.size();
	if (instances.empty())
		return S_OK;
	if (instances.size() > 512)
		return E_FAIL;
	auto buffer =
		CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");
	if (!buffer || !buffer->GetBuffer())
		return E_FAIL;
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->CSSetShaderResources(6, 1, &nullSRV);
	context->VSSetShaderResources(6, 1, &nullSRV);
	D3D11_BOX box{};
	box.right = (UINT)(sizeof(GPU_ANIM_INSTANCE_DATA) * instances.size());
	box.bottom = box.back = 1;
	context->UpdateSubresource(buffer->GetBuffer().Get(), 0, &box, instances.data(), 0, 0);
	return S_OK;
}
HRESULT CAnimatedWorldObject::BindInstanceBuffer(ID3D11DeviceContext* context)
{
	auto buffer =
		CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");
	if (!buffer || !buffer->GetSRV())
		return E_FAIL;
	ID3D11ShaderResourceView* srv = buffer->GetSRV().Get();
	context->VSSetShaderResources(6, 1, &srv);
	return S_OK;
}

HRESULT CAnimatedWorldObject::Render_Instanced(ID3D11DeviceContext* context,
											   const E::RENDER_CTX&,
											   const E::MODEL_INSTANCE_BATCH& batch)
{
	if (!context || !m_pInstancedVertexShader || !m_pPixelShader || batch.Instances.empty() ||
		batch.Instances.size() > 512 || batch.CombinedBoneMatrices.size() != batch.Instances.size())
		return E_FAIL;
	context->IASetInputLayout(m_pInstancedVertexShader->GetInputLayout().Get());
	context->VSSetShader(m_pInstancedVertexShader->GetVertexShader().Get(), nullptr, 0);
	context->PSSetShader(m_pPixelShader->GetPixelShader().Get(), nullptr, 0);
	if (FAILED(UpdateInstanceBuffer(context, batch.Instances)))
		return E_FAIL;
	auto model = CGameInstance::Get().GetResourceFirst<CResModel>(batch.Key.modelGroup, batch.Key.modelTag);
	auto palette = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER,
																			   "SBUFFER_CPU_BONEMATRIX");
	if (!model || !palette)
		return E_FAIL;
	_float4x4 identity{};
	XMStoreFloat4x4(&identity, XMMatrixIdentity());
	std::vector<_float4x4> matrices(batch.Instances.size() * 512, identity);
	for (size_t i = 0; i < batch.CombinedBoneMatrices.size(); ++i)
	{
		const auto& bones = batch.CombinedBoneMatrices[i];
		if (bones.empty() || bones.size() > 512)
			return E_FAIL;
		for (size_t b = 0; b < bones.size(); ++b)
			XMStoreFloat4x4(&matrices[i * 512 + b], XMMatrixTranspose(XMLoadFloat4x4(&bones[b])));
	}
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->VSSetShaderResources(7, 1, &nullSRV);
	if (FAILED(palette->UpdateData(matrices.data(), (uint32_t)(matrices.size() * sizeof(_float4x4)))) ||
		FAILED(BindInstanceBuffer(context)))
		return E_FAIL;
	ID3D11ShaderResourceView* paletteSRV = palette->GetSRV().Get();
	ID3D11ShaderResourceView* skinSRV = model->Get_GPUSkinBoneSRV();
	if (!paletteSRV || !skinSRV)
		return E_FAIL;
	context->VSSetShaderResources(7, 1, &paletteSRV);
	context->VSSetShaderResources(8, 1, &skinSRV);
	const auto& meshes = model->GetMeshes();
	const auto& materials = model->GetMaterials();
	for (uint32_t i = 0; i < std::min<uint32_t>(model->Get_NumMeshes(), (uint32_t)meshes.size()); ++i)
	{
		const auto& mesh = meshes[i];
		if (!mesh || mesh->Get_MaterialIndex() >= materials.size() || !materials[mesh->Get_MaterialIndex()])
			continue;
		const auto& skin = model->Get_GPUMeshSkinRange(i);
		if (!skin.iSkinBoneCount)
			return E_FAIL;
		GPU_SKIN_MESH_CONSTANTS constants{skin.iSkinBoneOffset, mesh->GetNumVertices(), skin.iSkinBoneCount};
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(context->Map(m_pSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			return E_FAIL;
		memcpy(mapped.pData, &constants, sizeof(constants));
		context->Unmap(m_pSkinMeshCBuffer->GetCBuffer().Get(), 0);
		ID3D11Buffer* cb = m_pSkinMeshCBuffer->GetCBuffer().Get();
		context->VSSetConstantBuffers(5, 1, &cb);
		ID3D11Buffer* vb = mesh->GetVertexBuffer().Get();
		const UINT stride = mesh->GetVertexStride(), offset = 0;
		context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
		context->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		context->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pModelInstance->Bind_Textures(context, i);
		m_pModelInstance->Bind_Materials(context, {1.f, 1.f, 1.f}, 0, {1.f, 1.f, 1.f}, 0.f, 1.f);
		context->DrawIndexedInstanced(mesh->GetNumIndices(), (UINT)batch.Instances.size(), 0, 0, 0);
	}
	ID3D11ShaderResourceView* nulls[3]{};
	context->VSSetShaderResources(6, 3, nulls);
	m_bRenderedLastFrame = true;
	++m_iInstancedRenderCount;
	return S_OK;
}

E::UPtr<CAnimatedWorldObject> CAnimatedWorldObject::Create()
{
	auto p = E::ToUPtr(new CAnimatedWorldObject{});
	return FAILED(p->InitializePrototype()) ? nullptr : std::move(p);
}
E::UPtr<E::CPrototype> CAnimatedWorldObject::Clone(void* arg)
{
	auto p = E::ToUPtr(new CAnimatedWorldObject{*this});
	return FAILED(p->Initialize(arg)) ? nullptr : std::move(p);
}
