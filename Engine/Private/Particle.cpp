#include "pch.h"
#include "Particle.h"
#include "GameInstance.h"
#include "ResTexture2D.h"
#include "ComModelInstance.h"

NS_USING(Engine)

CParticle::CParticle()
{

}

CParticle::CParticle(const CParticle& rhs)
{
}

CParticle::~CParticle()
{
}

HRESULT CParticle::LoadParticleTexture(std::pair<StringID, StringID> textureId)
{

	//if (auto res = CGameInstance::Get().AddResourceT<E::CResTestModel>("LOBJ", "Model_Resource", CResTestModel::Create("./Resources/SampleClient/Models/LightObject/LightObject.fbx"))) {
	//	E::CResTestModel::DESC pDesc = { MODEL::NONANIM, XMMatrixIdentity() };
	//	if (FAILED(res->Load(pDesc)))	return E_FAIL;
	//}
	//
	//
	//CComModelInstance::DESC Desc{};
	//Desc.sGroupTag = "LOBJ";
	//Desc.sResTag = "Model_Resource";
	//
	//if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
	//{
	//	return E_FAIL;
	//};


	m_pParticleTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>(textureId.first, textureId.second);
	return m_pParticleTexture ? S_OK : E_FAIL;
}