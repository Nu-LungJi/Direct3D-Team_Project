#pragma once
#include "AnimationObject.h"

NS_BEGIN(Engine)
class CComAnimator;
class CComBeHavior;
class CComModelInstance;
class CResCBuffer;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)
class CAnimatedWorldObject final : public CAnimationObject
{
public:
	DECLARE_DERIVED_TYPE(CAnimatedWorldObject, CAnimationObject)
	struct DESC : public CAnimationObject::DESC
	{
		_string sModelGroupTag{};
		_string sModelResourceTag{};
		_float3 vPosition{};
		_float3 vRotation{};
		_float3 vScale{ 1.f, 1.f, 1.f };
		_string sAnimationName{};
		_bool bAutoPlay{ true };
		_bool bLoop{ true };
		_float fAnimationSpeed{ 1.f };
		_float fStartRatio{};
		_string sBehaviorMajorTag{};
		_string sBehaviorMinorTag{};
	};

private:
	CAnimatedWorldObject() = default;
	~CAnimatedWorldObject() override = default;

public:
	void UpdateGUI() override;
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render_Instanced(
		ID3D11DeviceContext* pContext,
		const E::RENDER_CTX& RenderContext,
		const E::MODEL_INSTANCE_BATCH& Batch) override;
	_bool PlayAnimation(const _string& sAnimationName, _bool bLoop, _float fSpeed, _float fStartRatio = 0.f);
	void ApplyTransform(const _float3& vPosition, const _float3& vRotation, const _float3& vScale);
	void SetAnimationPaused(_bool bPaused);
	void StopAnimation();

private:
	HRESULT UpdateInstanceBuffer(
		ID3D11DeviceContext* pContext,
		const std::vector<E::GPU_ANIM_INSTANCE_DATA>& Instances);
	HRESULT BindInstanceBuffer(ID3D11DeviceContext* pContext);
	E::CComModelInstance* m_pModelInstance{};
	E::CComAnimator* m_pAnimator{};
	E::CComBeHavior* m_pBehavior{};
	E::SPtr<E::CResPixelShader> m_pPixelShader{};
	E::SPtr<E::CResVertexShader> m_pInstancedVertexShader{};
	E::SPtr<E::CResCBuffer> m_pSkinMeshCBuffer{};
	uint32_t m_iLastBatchInstanceCount{};
	uint64_t m_iInstanceSubmitCount{};
	uint64_t m_iInstancedRenderCount{};
	_bool m_bSubmittedThisFrame{};
	_bool m_bRenderedLastFrame{};

public:
	static E::UPtr<CAnimatedWorldObject> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};
NS_END
