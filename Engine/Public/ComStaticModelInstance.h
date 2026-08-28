#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CResStaticModelMesh;
class CResModelMaterial;
class CResModelBone;
class CResModelMaterial;
class CResStaticModel;
class CResTexture2D;

class ENGINE_DLL CComStaticModelInstance : public CComponent
{
public:
	typedef struct tagDesc : CComponent::DESC
	{
		StringID sGroupTag;
		StringID sResTag;
	}DESC;
public:
	DECLARE_DERIVED_TYPE(CComStaticModelInstance, CComponent)


public:
	virtual void UpdateGUI() override;


private:
	explicit CComStaticModelInstance();
	~CComStaticModelInstance() override;


private:
	HRESULT Initialize(void* pArg) override;


public:
	/*----------- 광윤 추가 -----------*/
	VOID Bind_Textures(ID3D11DeviceContext* pContext, uint32_t _MeshIndex);
	VOID Bind_Materials(ID3D11DeviceContext* pContext, _float3 _EmissiveColor, _float _EmissiveIntensity, _float3 _DissolveColor, _float _DissolveIntensity, _float _ObjectAlpha,
		_float _NormalIntensity = 1.f, _float _MetallicIntensity = 1.f, _float _RoughnessIntensity = 1.f, _float _AmbientIntensity = 1.f);
	/*---------------------------------*/
	// 움직이는 비애니메이션 오브젝트의 프레임 배치를 공통 셰이더로 렌더링한다.
	// 정적 MapMesh 상주 인스턴싱과는 별도의 Model Instance Manager 경로다.
	HRESULT RenderDynamicInstances(
		ID3D11DeviceContext* context,
		const MODEL_INSTANCE_BATCH& batch);
	// 비스키닝 모델의 객체별 그림자 렌더링과 월드 바운드 계산을 공통 처리한다.
	HRESULT RenderShadow(
		ID3D11DeviceContext* context,
		CComConstantBuffer* perObjectBuffer,
		const _float4x4& worldMatrix,
		_fmatrix viewProjectionMatrix);
	bool GetShadowBounds(
		const _float4x4& worldMatrix,
		BoundingBox& outBounds) const;
	SPtr<CResTexture2D> Get_MeshTexture(uint32_t iMeshIndex, AI_TEXTURE_TYPE eMaterialType, uint32_t iTextureIndex);

public:
	SPtr<CResStaticModel> GetModel() { return m_pModel; }
	HRESULT ChangeModel(const StringID& sGroupTag, const StringID& sResTag);



	StringID Get_GroupTag() { return m_sGroupTag; }
	StringID Get_ResTag() { return m_sResTag; }

public:

	HRESULT Save_Binary_Json(std::string outpath);

private:
	SPtr<CResStaticModel> m_pModel;
	StringID m_sGroupTag;
	StringID m_sResTag;
public:
	static UPtr<CComStaticModelInstance> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
