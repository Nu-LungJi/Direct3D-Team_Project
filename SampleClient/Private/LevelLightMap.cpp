#include "pch.h"
#include "LevelLightMap.h"
#include "GameInstance.h"
#include "LevelLoading.h"

#include "FlyCamera.h"

#include "ResCBuffer.h"
#include "BackGround.h"
#include "Light.h"
#include "Terrain.h"
#include "TestModel.h"
#include "LightObject.h"

NS_USING(Client)

CLevelLightMap::CLevelLightMap()
{
}

CLevelLightMap::~CLevelLightMap()
{
}

HRESULT CLevelLightMap::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();

	{
		CLightObject::DESC LDesc{};
		LDesc.sObjectTag = "LightObject";
		auto ObjectHandle = E::CGameInstance::Get().AddGameObjectToLayer("LIGHT", "Prototype_GameObject_LightObject", "01_LightObject", &LDesc);
		if (!ObjectHandle.has_value())	return E_FAIL;
		auto LightObject = E::CGameInstance::Get().GetGameObjectByHandle(ObjectHandle.value());
		if (!LightObject)	return E_FAIL;
	}
	{
		CLightObject::DESC LDesc{};
		LDesc.sObjectTag = "LightObject2";
		auto ObjectHandle = E::CGameInstance::Get().AddGameObjectToLayer("LIGHT", "Prototype_GameObject_LightObject", "02_LightObject", &LDesc);
		if (!ObjectHandle.has_value())	return E_FAIL;
		auto LightObject = E::CGameInstance::Get().GetGameObjectByHandle(ObjectHandle.value());
		if (!LightObject)	return E_FAIL;

		LightObject->GetComponent<CComTransform>("Com_Transform")->SetScale(XMVectorSet(70.f, 70.f, 70.f, 1.f));
		LightObject->GetComponent<CComTransform>("Com_Transform")->SetPosition(XMVectorSet(1.f, 10.f, 0.f, 1.f));
	}
	{
		CTerrain::DESC Desc{};
		Desc.sObjectTag = "Terrain";

		if (!(E::CGameInstance::Get().AddGameObjectToLayer("LIGHT", "Prototype_GameObject_Terrain",
			"02_Terrain", &Desc)))
		{
			return E_FAIL;
		}
	}
	
	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
		Desc.vAt = { 0.f, 0.f, 0.f };
		Desc.vEye = { 0.f, 0.f, -5.f };
		Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
		Desc.fFovY = 75.f;
		Desc.fNear = 0.1f;
		Desc.fFar = 100.f;
		Desc.sObjectTag = "FlyCam";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("FLY", flyCam.value())))
			{
				MSG_BOX("MSG_BOX_123");
			}
			E::CGameInstance::Get().SetActiveCamera("FLY");
		}
	}
	if (E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_Light", CLight::Create()))	return E_FAIL;
	CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);
	CGameInstance::Get().Add_PointLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 100.f, 10.f);

	return S_OK;
}

void CLevelLightMap::Update(E::_float fTimeDelta)
{
}

HRESULT CLevelLightMap::Render()
{
	return S_OK;
}

void CLevelLightMap::UpdateGUI()
{
	ImGui::Begin("LEVEL: CLevelLightMap");



	ImGui::End();
}

void CLevelLightMap::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelLightMap> CLevelLightMap::Create()
{
	auto	pInstance = Engine::UPtr<CLevelLightMap>(new CLevelLightMap{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelLightMap");
	}

	return pInstance;
}

void CLevelLightMap::Free()
{
	CGameInstance::Get().Clear_DynamicLightList();
	E::CGameInstance::Get().DelPrototype("LIGHT");
	E::CGameInstance::Get().DelResource("SAMPLE_CLIENT_TEX");
	E::CGameInstance::Get().DelResource("SAMPLE_CLIENT_BUFFER");
	E::CGameInstance::Get().DelResource("LOBJ", "Model_Resource");
	CLevel::Free();
}
