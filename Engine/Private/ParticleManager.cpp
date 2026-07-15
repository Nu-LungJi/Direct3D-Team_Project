#include "pch.h"
#include "ParticleManager.h"
#include "Particle.h"
#include "Beam_CPU.h"
#include "Trail_CPU.h"
#include "ParticlePattern.h"
#include "Particle_GPU.h"
#include "Particle_CPU.h"
#include "Trail_CPU.h"
NS_USING(Engine)

std::vector<std::string> ScanFbxFolder(const std::string& strFbxFolder);
std::vector<std::string> ScanTextureFolder(const std::string& strFbxFolder);
ID3D11ShaderResourceView* GetOrLoadTextureThumbnail(const std::string& strFbxFolder);

CParticleManager::CParticleManager()
{
}

CParticleManager::~CParticleManager()
{
}

void CParticleManager::UpdateGUI()
{
	/*
				serializer.Write("iMaxParticles", iMaxParticles);
			serializer.Write("iBehaviorType", iBehaviorType);
			serializer.Write("PARTICLE_TYPE", type);
			//serializer.Write("textureID", textureID);
			serializer.Write("whatKind", whatKind);
	*/
	ImGui::Begin("SaveJson");
	//if (ImGui::Button("Test Save"))
	//{
	//	CParticle_GPU::TESTDESC testDesc{};
	//	testDesc.sStr = "hello";
	//	testDesc.i = 123;
	//
	//
	//	CParticle_GPU::DESC desc;
	//	desc.testDesc.push_back(testDesc);
	//	desc.testDesc.push_back(testDesc);
	//	desc.testDesc.push_back(testDesc);
	//	desc.iBehaviorType = 1;
	//	desc.iMaxParticles = 500;
	//	desc.type = PARTICLE_TYPE::FIRE_CPU;
	//	desc.whatKind = MESHORTEXTURE::MESH;
	//	CGameInstance::Get().JsonSerialize("TestParticle.json", desc);
	//	int x = 0;
	//}
	//
	//if (ImGui::Button("Test Load"))
	//{
	//	CParticle_GPU::DESC desc;
	//	CGameInstance::Get().JsonDeSerialize("TestParticle.json", desc);
	//	int x = 0;
	//}
	ImGui::End();
	ImGui::Begin("SaveJson");
	

	static const std::string kFbxListJsonPath = "./Resources/SampleClient/Models/ParticleModelJson/ParticleModel.json";
	static const std::string kJsonFolder = "./Resources/json/Particle";

	static std::vector<std::string> fbxFileList;
	static bool bFbxScanned = false;
	static int selectedFbxIndex = -1;
	static std::string selectedFbxPath;

	static char szJsonName[MAX_PATH] = "ParticleData.json";
	static int whatKindIndex = 0; // 0: MESH, 1: TEXTURE
	static int particleTypeIndex = 0;
	const char* particleTypeNames[] = { "PARTICLE_CPU", "PARTICLE_GPU", "BEAM_CPU", "TRAIL_CPU" };
	static char szParticleName[128] = "";
	static char szTextureID1[128] = "SAMPLE_CLINET_TEXTURE";
	static char szTextureID2[128] = "TEX_RIBBON";
	static int iMaxParticles = 1000;
	static int iBehaviorType = 1;
	static char szVSID1[128] = "SAMPLE_CLIENT_SHADER";
	static char szVSID2[128] = "VS_VTX_GPU_PARTICLE_MESH";
	static char szPSID1[128] = "SAMPLE_CLIENT_SHADER";
	static char szPSID2[128] = "PS_VTX_GPU_PARTICLE_MESH";
	static char szGroupTag[128] = "Rock1";
	static char szResTag[128] = "Static_Model_Resource";
	static char szViBuffer1[128] = "SAMPLE_CLIENT_PARTICLEBF";
	static char szViBuffer2[128] = "VIBUF_ParticleQuad";

	static const std::string kTextureRealFolder = "./Resources/SampleClient/Textures/EffectParticle";
	static std::vector<std::string> textureFileList;
	static bool bTextureScanned = false;
	static int selectedTextureIndex = -1;
	static std::string selectedTexturePath;


	static int iTexRow = 1;
	static int iTexCol = 1;

	// ---- 0. WhatKind 대분류 선택 ----
	const char* whatKindNames[] = { "MESH", "TEXTURE" };
	ImGui::Combo("WhatKind", &whatKindIndex, whatKindNames, IM_ARRAYSIZE(whatKindNames));

	ImGui::Separator();

	// ---- 1. MESH일 때만: fbx 섹션 ----
	if (whatKindIndex == 0)
	{
		if (!bFbxScanned)
		{
			fbxFileList = ScanFbxFolder(kFbxListJsonPath);
			bFbxScanned = true;
		}

		if (ImGui::Button("Rescan Fbx Folder"))
			fbxFileList = ScanFbxFolder(kFbxListJsonPath);

		ImGui::Text("Fbx Files:");

		if (!fbxFileList.empty())
		{
			std::vector<const char*> namesForCombo;
			for (auto& s : fbxFileList)
				namesForCombo.push_back(s.c_str());

			static const std::string kFbxRealFolder = "./Resources/SampleClient/Models/OriginData/Static/ParticleMeshes";

			if (ImGui::Combo("Fbx List", &selectedFbxIndex, namesForCombo.data(), (int)namesForCombo.size()))
			{
				selectedFbxPath = kFbxRealFolder + "/" + fbxFileList[selectedFbxIndex];
			}

			if (!selectedFbxPath.empty())
				ImGui::Text("Selected: %s", selectedFbxPath.c_str());
		}
		else
		{
			ImGui::Text("(No fbx files found in folder)");
		}

		ImGui::InputText("GroupTag", szGroupTag, IM_ARRAYSIZE(szGroupTag));
		ImGui::InputText("ResTag", szResTag, IM_ARRAYSIZE(szResTag));
	}

	// ---- 2. TEXTURE일 때만: 텍스처 섹션 ----
	if (whatKindIndex == 1)
	{
		if (!bTextureScanned)
		{
			textureFileList = ScanTextureFolder(kTextureRealFolder);
			bTextureScanned = true;
		}

		if (ImGui::Button("Rescan Texture Folder"))
			textureFileList = ScanTextureFolder(kTextureRealFolder);

		ImGui::Text("Textures:");
		ImGui::InputText("VIBuffer1 if CPUTEX", szViBuffer1, IM_ARRAYSIZE(szViBuffer1));
		ImGui::InputText("VIBuffer2  if CPUTEX", szViBuffer2, IM_ARRAYSIZE(szViBuffer2));
		ImGui::InputInt("TexRowCount", &iTexRow);
		ImGui::InputInt("TexColCount", &iTexCol);

		const float thumbnailSize = 64.0f;
		const float cellPadding = 10.0f;
		const float cellWidth = thumbnailSize + cellPadding;
		int columns = 4;
		float windowWidth = ImGui::GetContentRegionAvail().x;
		columns = std::max(1, (int)(windowWidth / cellWidth));

		int i = 0;
		for (auto& texName : textureFileList)
		{
			std::string fullPath = kTextureRealFolder + "/" + texName;
			ID3D11ShaderResourceView* pSRV = GetOrLoadTextureThumbnail(fullPath);

			ImGui::PushID(i);
			ImGui::BeginGroup();

			if (pSRV)
			{
				if (ImGui::ImageButton((ImTextureID)pSRV, ImVec2(thumbnailSize, thumbnailSize)))
				{
					selectedTextureIndex = i;
					selectedTexturePath = fullPath;
				}
			}
			else
			{
				if (ImGui::Button("No Img", ImVec2(thumbnailSize, thumbnailSize)))
				{
					selectedTextureIndex = i;
					selectedTexturePath = fullPath;
				}
			}

			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + thumbnailSize);
			ImGui::TextWrapped("%s", texName.c_str());
			ImGui::PopTextWrapPos();

			ImGui::EndGroup();

			if (selectedTextureIndex == i)
			{
				ImVec2 minPos = ImGui::GetItemRectMin();
				ImVec2 maxPos = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddRect(minPos, maxPos, IM_COL32(255, 200, 0, 255), 0.0f, 0, 2.0f);
			}

			ImGui::PopID();

			if ((i + 1) % columns != 0)
				ImGui::SameLine(0.0f, cellPadding);

			i++;
		}

		ImGui::NewLine();

		if (!selectedTexturePath.empty())
			ImGui::Text("Selected Texture: %s", selectedTexturePath.c_str());

		ImGui::InputText("TextureID1", szTextureID1, IM_ARRAYSIZE(szTextureID1));
		ImGui::InputText("TextureID2", szTextureID2, IM_ARRAYSIZE(szTextureID2));
	}

	ImGui::Separator();

	// ---- 3. 공통 값 입력 ----
	ImGui::InputText("Json Name", szJsonName, IM_ARRAYSIZE(szJsonName));
	ImGui::Combo("Particle Type", &particleTypeIndex, particleTypeNames, IM_ARRAYSIZE(particleTypeNames));
	ImGui::InputText("Particle Name (e.g. ROCK1_CPU)", szParticleName, IM_ARRAYSIZE(szParticleName));
	ImGui::InputInt("MaxParticles", &iMaxParticles);
	ImGui::InputInt("BehaviorType", &iBehaviorType);
	ImGui::InputText("VSID1", szVSID1, IM_ARRAYSIZE(szVSID1));
	ImGui::InputText("VSID2", szVSID2, IM_ARRAYSIZE(szVSID2));
	ImGui::InputText("PSID1", szPSID1, IM_ARRAYSIZE(szPSID1));
	ImGui::InputText("PSID2", szPSID2, IM_ARRAYSIZE(szPSID2));

	ImGui::Separator();

	// ---- 4. 저장 ----
	if (ImGui::Button("Save Json"))
	{
		std::string targetPath = (whatKindIndex == 1) ? selectedTexturePath : selectedFbxPath;

		if (!targetPath.empty())
		{
			std::string saveName = szJsonName;
			if (!saveName.empty())
			{
				std::filesystem::path savePath = std::filesystem::path(kJsonFolder) / saveName;
				if (savePath.extension().empty())
					savePath.replace_extension(".json");

				std::string whatKindStr = (whatKindIndex == 0) ? "MESH" : "TEXTURE";
				std::string particleTypeStr = particleTypeNames[particleTypeIndex];
				std::string particleNameStr = szParticleName;

				if (whatKindStr == "MESH") {
					Save_Binary_Json(savePath.string(),
						targetPath,
						whatKindStr,
						particleTypeStr,
						particleNameStr,
						iMaxParticles,
						iBehaviorType,
						szVSID1, szVSID2, szPSID1, szPSID2,
						szGroupTag, szResTag);
				}
				else if (whatKindStr == "TEXTURE") {

					if (particleTypeStr == "PARTICLE_CPU") {
						Save_Binary_Json(savePath.string(),
							targetPath,
							whatKindStr,
							particleTypeStr,
							particleNameStr,
							iMaxParticles,
							iBehaviorType,
							szVSID1, szVSID2, szPSID1, szPSID2,
							szGroupTag, szResTag, szTextureID1, szTextureID2, szViBuffer1, szViBuffer2, iTexRow, iTexCol);
					}
					else {
						Save_Binary_Json(savePath.string(),
							targetPath,
							whatKindStr,
							particleTypeStr,
							particleNameStr,
							iMaxParticles,
							iBehaviorType,
							szVSID1, szVSID2, szPSID1, szPSID2,
							szGroupTag, szResTag, szTextureID1, szTextureID2,"","", iTexRow, iTexCol);
					}
			
				}
			}
		}
	}

	ImGui::End();
	ImGui::Begin("CParticleManager");

	static int whatKindFilterIndex = 0; // 0: MESH, 1: TEXTURE
	static int groupTypeIndex = 0;      // PARTICLE_CPU / PARTICLE_GPU / BEAM_CPU / RIBBON_CPU
	static int typeIndex = 0;
	static SPAWN_COMMAND_KIND currentKind = SPAWN_COMMAND_KIND::STANDARD;

	static STANDARD_PARAMS pendingStandard{};
	static BEAM_PARAMS     pendingBeam{};
	static STAIR_PARAMS    pendingStair{};
	static STRAIGHT_PARAMS pendingStraight{};

	// ---- 0. WhatKind 필터 ----
	if (ImGui::RadioButton("MESH", whatKindFilterIndex == 0)) { whatKindFilterIndex = 0; typeIndex = 0; }
	ImGui::SameLine();
	if (ImGui::RadioButton("TEXTURE", whatKindFilterIndex == 1)) { whatKindFilterIndex = 1; typeIndex = 0; }

	ImGui::Separator();

	// ---- 1. Group: 파티클 타입(카테고리) 콤보 ----
	const char* groupTypeNames[] = { "PARTICLE_CPU", "PARTICLE_GPU", "BEAM_CPU", "RIBBON_CPU" };
	if (ImGui::Combo("Group (ParticleType)", &groupTypeIndex, groupTypeNames, IM_ARRAYSIZE(groupTypeNames)))
		typeIndex = 0;

	ImGui::Separator();

	// ---- 2. Type: 선택된 카테고리에 맞는 실제 파티클 이름 목록 ----
	struct MatchedParticle
	{
		StringID sGroupTag;
		StringID sTypeTag;
		std::string sDisplayName;
	};
	std::vector<MatchedParticle> matchedList;

	for (auto& [groupTag, typeMap] : m_Particles)
	{
		for (auto& [typeTag, particlePtr] : typeMap)
		{
			CParticle* pParticle = particlePtr.get();
			if (!pParticle)
				continue;

			bool bCategoryMatch = false;

			switch (groupTypeIndex)
			{
			case 0: bCategoryMatch = (dynamic_cast<CParticle_CPU*>(pParticle) != nullptr); break;
			case 1: bCategoryMatch = (dynamic_cast<CParticle_GPU*>(pParticle) != nullptr); break;
			case 2: bCategoryMatch = (dynamic_cast<CBeam_CPU*>(pParticle) != nullptr); break;
			case 3: bCategoryMatch = (dynamic_cast<CTrail_CPU*>(pParticle) != nullptr); break;
			}

			if (!bCategoryMatch)
				continue;

			MESHORTEXTURE wantedKind = (whatKindFilterIndex == 0) ? MESHORTEXTURE::MESH : MESHORTEXTURE::TEX;

			if (auto pGPU = dynamic_cast<CParticle_GPU*>(pParticle))
			{
				if (pGPU->GetWhatKind() != wantedKind)
					continue;
			}
			else if (auto pCPU = dynamic_cast<CParticle_CPU*>(pParticle))
			{
				if (pCPU->GetWhatKind() != wantedKind)
					continue;
			}

			matchedList.push_back({ groupTag, typeTag, typeTag.GetDbgStr() });
		}
	}

	StringID selectedGroup{};
	StringID selectedType{};

	if (!matchedList.empty())
	{
		std::vector<const char*> namesForCombo;
		for (auto& m : matchedList)
			namesForCombo.push_back(m.sDisplayName.c_str());

		typeIndex = std::clamp(typeIndex, 0, (int)namesForCombo.size() - 1);
		ImGui::Combo("Type", &typeIndex, namesForCombo.data(), (int)namesForCombo.size());

		selectedGroup = matchedList[typeIndex].sGroupTag;
		selectedType = matchedList[typeIndex].sTypeTag;

		auto pSelected = GetParticle(selectedGroup, selectedType);
		if (dynamic_cast<CBeam_CPU*>(pSelected) != nullptr)
			currentKind = SPAWN_COMMAND_KIND::BEAM;
	}
	else
	{
		ImGui::Text("(No particles found for this category)");
	}

	ImGui::Separator();

	{
		int kindIndex = (int)currentKind;
		const char* kindNames[] = { "Standard", "Beam", "Stair", "Straight" };
		if (ImGui::Combo("Spawn Kind", &kindIndex, kindNames, IM_ARRAYSIZE(kindNames)))
			currentKind = (SPAWN_COMMAND_KIND)kindIndex;
	}

	ImGui::Separator();

	if (currentKind == SPAWN_COMMAND_KIND::STANDARD)
	{
		ImGui::Text("Standard Particle Params");
		int countInput = (int)pendingStandard.count;
		ImGui::InputInt("Count", &countInput);
		pendingStandard.count = (uint32_t)std::clamp(countInput, 1, (int)MAX_SPAWN_PER_CALL);

		ImGui::InputFloat3("Position", &pendingStandard.position.x);
		ImGui::InputFloat3("Velocity", &pendingStandard.velocity.x);
		ImGui::InputFloat("Life", &pendingStandard.life);
		ImGui::InputFloat("Size", &pendingStandard.size);
		ImGui::ColorEdit4("BaseColor", &pendingStandard.color.x);
		ImGui::ColorEdit3("Emissive Color", &pendingStandard.emissive.x);
		ImGui::InputFloat("Emissive Intensity", &pendingStandard.emissive.w);
		ImGui::InputFloat("SpawnDelay", &pendingStandard.fSpawnDelay);
		ImGui::Checkbox("Loop", &pendingStandard.bLoop);

		if (pendingStandard.bLoop)
			ImGui::InputFloat("Spawn Interval", &pendingStandard.fSpawnInterval);
	}
	else if (currentKind == SPAWN_COMMAND_KIND::BEAM)
	{
		ImGui::Text("Beam Params");
		ImGui::InputFloat4("Start Pos", &pendingBeam.beamStart.x);
		ImGui::InputFloat4("End Pos", &pendingBeam.beamEnd.x);
		ImGui::InputInt("DisplacementIterations", &pendingBeam.iDisplacementIterations);
		ImGui::InputFloat("DisplacementAmplitude", &pendingBeam.fDisplacementAmplitude);
		ImGui::InputFloat("DisplacementDamping", &pendingBeam.fDisplacementDamping);
		ImGui::InputFloat("flickerTimeInverval", &pendingBeam.flickerTimeInverval);
		ImGui::InputFloat("Duration", &pendingBeam.beamDuration);
		ImGui::InputFloat("SpawnDelay", &pendingBeam.fSpawnDelay);
		ImGui::ColorEdit4("BaseColor", &pendingBeam.color.x);
		ImGui::ColorEdit3("Emissive Color", &pendingBeam.emissive.x);
		ImGui::InputFloat("Emissive Intensity", &pendingBeam.emissive.w);
	}
	else if (currentKind == SPAWN_COMMAND_KIND::STAIR)
	{
		ImGui::Text("Stair Params");
		ImGui::InputFloat3("Start Pos", &pendingStair.vStartPos.x);
		int stepCount = (int)pendingStair.iStepCount;
		ImGui::InputInt("Step Count", &stepCount);
		pendingStair.iStepCount = (uint32_t)std::max(1, stepCount);
		ImGui::InputFloat("Step Width", &pendingStair.fStepWidth);
		ImGui::InputFloat("Step Height", &pendingStair.fStepHeight);
		ImGui::InputFloat("Step Depth", &pendingStair.fStepDepth);
		ImGui::InputFloat("Life", &pendingStair.life);
		ImGui::InputFloat("SpawnDelay", &pendingStair.fSpawnDelay);
		ImGui::ColorEdit4("BaseColor", &pendingStair.color.x);
		ImGui::ColorEdit3("Emissive Color", &pendingStair.emissive.x);
		ImGui::InputFloat("Emissive Intensity", &pendingStair.emissive.w);
	}
	else if (currentKind == SPAWN_COMMAND_KIND::STRAIGHT)
	{
		ImGui::Text("Straight Params");
		ImGui::InputFloat3("Start Pos", &pendingStraight.vStartPos.x);
		int rowCount = (int)pendingStraight.row;
		int colCount = (int)pendingStraight.col;
		ImGui::InputInt("Row Count", &rowCount);
		ImGui::InputInt("Column Count", &colCount);
		pendingStraight.row = (uint32_t)std::max(1, rowCount);
		pendingStraight.col = (uint32_t)std::max(1, colCount);
		ImGui::InputFloat("OffSetX", &pendingStraight.offSetX);
		ImGui::InputFloat("OffsetZ", &pendingStraight.offsetZ);
		ImGui::InputFloat("SpawnDelay", &pendingStraight.spawnDelay);
		ImGui::InputFloat("Size", &pendingStraight.fSize);
		ImGui::InputFloat("Life", &pendingStraight.fLife);
		ImGui::ColorEdit4("BaseColor", &pendingStraight.color.x);
		ImGui::ColorEdit3("Emissive Color", &pendingStraight.emissive.x);
		ImGui::InputFloat("Emissive Intensity", &pendingStraight.emissive.w);
	}

	if (ImGui::Button("Add to List") && !matchedList.empty())
	{
		SPAWN_COMMAND cmd{};
		cmd.sGroupTag_KindTag = currentKind;
		cmd.sGroupTag = selectedGroup;
		cmd.sTypeTag = selectedType;

		if (currentKind == SPAWN_COMMAND_KIND::STANDARD)
			cmd.params = pendingStandard;
		else if (currentKind == SPAWN_COMMAND_KIND::BEAM)
			cmd.params = pendingBeam;
		else if (currentKind == SPAWN_COMMAND_KIND::STAIR)
			cmd.params = pendingStair;
		else if (currentKind == SPAWN_COMMAND_KIND::STRAIGHT)
			cmd.params = pendingStraight;

		m_vecCommandQueue.push_back(cmd);
	}

	ImGui::Separator();

	ImGui::Text("Spawn Queue (%zu)", m_vecCommandQueue.size());
	for (int i = 0; i < (int)m_vecCommandQueue.size(); ++i)
	{
		auto& cmd = m_vecCommandQueue[i];
		ImGui::PushID(i);

		if (cmd.sGroupTag_KindTag == SPAWN_COMMAND_KIND::STANDARD)
		{
			const auto& p = std::get<STANDARD_PARAMS>(cmd.params);
			ImGui::Text("[%s/%s] count=%u pos=(%.1f,%.1f,%.1f)",
				cmd.sGroupTag.GetDbgStr(), cmd.sTypeTag.GetDbgStr(),
				p.count, p.position.x, p.position.y, p.position.z);
		}
		else if (cmd.sGroupTag_KindTag == SPAWN_COMMAND_KIND::BEAM)
		{
			const auto& p = std::get<BEAM_PARAMS>(cmd.params);
			ImGui::Text("[%s/%s] BEAM start=(%.1f,%.1f,%.1f)",
				cmd.sGroupTag.GetDbgStr(), cmd.sTypeTag.GetDbgStr(),
				p.beamStart.x, p.beamStart.y, p.beamStart.z);
		}
		else if (cmd.sGroupTag_KindTag == SPAWN_COMMAND_KIND::STAIR)
		{
			const auto& p = std::get<STAIR_PARAMS>(cmd.params);
			ImGui::Text("[%s/%s] STAIR start=(%.1f,%.1f,%.1f) steps=%u",
				cmd.sGroupTag.GetDbgStr(), cmd.sTypeTag.GetDbgStr(),
				p.vStartPos.x, p.vStartPos.y, p.vStartPos.z, p.iStepCount);
		}
		else if (cmd.sGroupTag_KindTag == SPAWN_COMMAND_KIND::STRAIGHT)
		{
			const auto& p = std::get<STRAIGHT_PARAMS>(cmd.params);
			ImGui::Text("[%s/%s] STRAIGHT start=(%.1f,%.1f,%.1f) row=%u col=%u",
				cmd.sGroupTag.GetDbgStr(), cmd.sTypeTag.GetDbgStr(),
				p.vStartPos.x, p.vStartPos.y, p.vStartPos.z, p.row, p.col);
		}

		ImGui::SameLine();
		if (ImGui::Button("Remove"))
		{
			m_vecCommandQueue.erase(m_vecCommandQueue.begin() + i);
			ImGui::PopID();
			break;
		}

		ImGui::PopID();
	}

	if (ImGui::Button("Clear List"))
	{
		m_vecCommandQueue.clear();
	}

	ImGui::SameLine();

	if (ImGui::Button("Execute Spawn (All)"))
	{
		ExecuteCommandQueue();
	}

	static char szQueueSavePath[MAX_PATH] = "./Resources/json/Particle/SpawnQueue.json";
	ImGui::InputText("Queue Save Path", szQueueSavePath, IM_ARRAYSIZE(szQueueSavePath));

	if (ImGui::Button("Save Queue"))
	{
		SaveCommandQueue(szQueueSavePath);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Queue"))
	{
		LoadCommandQueue(szQueueSavePath);
	}
	ImGui::End();
}

void CParticleManager::Update(_float fTimeDelta)
{
	for (auto& [groupTag, typeMap] : m_Particles)
	{
		for (auto& [typeTag, particle] : typeMap)
		{
			particle->PriorityUpdate(fTimeDelta);
			particle->Update(fTimeDelta);
			particle->LateUpdate(fTimeDelta);
		}
	}

	for (auto& req : m_LoopRequests)
	{
		req.fElapsed += fTimeDelta;
		if (req.fElapsed < req.fSpawnInterval)
			continue;

		req.fElapsed -= req.fSpawnInterval;
		Spawn(req.sGroupTag, req.sTypeTag, (uint32_t)req.vecSpawnData.size(), req.vecSpawnData.data());
	}
}
//BLEND에서 하고 있었던거고
HRESULT CParticleManager::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	for (auto& [groupTag, typeMap] : m_Particles)
	{
		for (auto& [typeTag, particle] : typeMap)
		{
			particle->Render(pContext, ctx);
		}
	}
	return S_OK;
}

HRESULT CParticleManager::Add_Particle(const StringID& sGroupTag, const StringID& sTypeTag, UPtr<CParticle> particle)
{
	if (particle == nullptr)
		return E_FAIL;

	if (FAILED(particle->Initialize(nullptr)))
		return E_FAIL;

	auto& typeMap = m_Particles[sGroupTag];
	if (typeMap.contains(sTypeTag))
		return E_FAIL;

	typeMap[sTypeTag] = std::move(particle);
	return S_OK;
}

HRESULT CParticleManager::Spawn(const StringID& sGroupTag, const StringID& sTypeTag,
	uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
	_bool bLoop, _float fSpawnInterval)
{
	if (pSpawnData == nullptr || count == 0)
		return E_FAIL;

	auto groupIt = m_Particles.find(sGroupTag);
	if (groupIt == m_Particles.end())
		return E_FAIL;

	auto typeIt = groupIt->second.find(sTypeTag);
	if (typeIt == groupIt->second.end())
		return E_FAIL;

	std::vector<PARTICLE_SPAWN_DATA> spawnList(pSpawnData, pSpawnData + count);
	typeIt->second->RequestSpawn(spawnList);
	HRESULT hr = S_OK;

	if (bLoop)
	{
		PARTICLE_LOOP_REQUEST req{};
		req.sGroupTag = sGroupTag;
		req.sTypeTag = sTypeTag;
		req.vecSpawnData.assign(pSpawnData, pSpawnData + count);
		req.fSpawnInterval = fSpawnInterval;
		req.fElapsed = 0.f;

		m_LoopRequests.push_back(std::move(req));
	}

	return hr;
}

HRESULT CParticleManager::SpawnRandomInGroup(const StringID& sGroupTag,
	uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
	_bool bLoop, _float fSpawnInterval)
{
	if (pSpawnData == nullptr || count == 0)
		return E_FAIL;

	auto groupIt = m_Particles.find(sGroupTag);
	if (groupIt == m_Particles.end() || groupIt->second.empty())
		return E_FAIL;

	auto& typeMap = groupIt->second;
	uint32_t randIndex = rand() % (uint32_t)typeMap.size();

	auto it = typeMap.begin();
	std::advance(it, randIndex);

	HRESULT hr = it->second->Spawn(count, pSpawnData);
	if (FAILED(hr))
		return hr;

	if (bLoop)
	{
		PARTICLE_LOOP_REQUEST req{};
		req.sGroupTag = sGroupTag;
		req.sTypeTag = it->first;
		req.vecSpawnData.assign(pSpawnData, pSpawnData + count);
		req.fSpawnInterval = fSpawnInterval;
		req.fElapsed = 0.f;

		m_LoopRequests.push_back(std::move(req));
	}

	return hr;
}

HRESULT CParticleManager::SpawnAllInGroup(const StringID& sGroupTag,
	uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData)
{
	if (pSpawnData == nullptr || count == 0)
		return E_FAIL;

	auto groupIt = m_Particles.find(sGroupTag);
	if (groupIt == m_Particles.end() || groupIt->second.empty())
		return E_FAIL;

	HRESULT hr = S_OK;
	for (auto& [typeTag, particle] : groupIt->second)
	{
		if (FAILED(particle->Spawn(count, pSpawnData)))
			hr = E_FAIL;
	}
	return hr;
}

HRESULT CParticleManager::SpawnRibbon(uint32_t quantity, const _float4& start, const _float4& end, _float fDisplacementAmplitude, _float iDisplacementIterations, _float fDisplacementDamping, _float fFlickerInterval, const _float4& vColor, _float4 emissive, _float fDuration)
{
	auto pParticle = GetParticle("BEAM", "ATTACK");
	if (!pParticle)
		return E_FAIL;
	auto pBeam = static_cast<CBeam_CPU*>(pParticle);

	for (uint32_t i = 0; i < quantity; i++) {
		int32_t idx1 = pBeam->AddBeam(start, end, fDisplacementAmplitude, (uint32_t)iDisplacementIterations, fDisplacementDamping, fFlickerInterval, vColor, emissive, fDuration);
	}
	return S_OK;
}

HRESULT CParticleManager::Save_Binary_Json(std::string outpath,
	const std::string& FullPath,
	const std::string& whatKind,
	const std::string& particleType,
	const std::string& particleName,
	int iMaxParticles,
	int iBehaviorType,
	const std::string& VSGroup,
	const std::string& VSID,
	const std::string& PSGroup,
	const std::string& PSID,
	const std::string& sGroupTag,
	const std::string& sResTag,
	const std::string& textureID1,
	const std::string& textureID2,
	const std::string& viBufferID1,
	const std::string& viBufferID2,
	int RowCount,
	int ColCount)
{
	if (outpath.empty() || FullPath.empty())
		return E_FAIL;

	std::filesystem::path savePath(outpath);

	if (savePath.extension().empty())
		savePath.replace_extension(".json");

	if (!savePath.parent_path().empty())
		std::filesystem::create_directories(savePath.parent_path());

	std::string fbxName = std::filesystem::path(FullPath).filename().string();

	if (fbxName.empty())
		return E_FAIL;

	std::string fullPath = FullPath;

	nlohmann::json j;

	if (std::filesystem::exists(savePath))
	{
		std::ifstream inFile(savePath);
		if (inFile.is_open())
		{
			try { inFile >> j; }
			catch (...) { j = nlohmann::json{}; }
			inFile.close();
		}
	}

	nlohmann::json newEntry;
	newEntry["path"] = fullPath;
	newEntry["whatKind"] = whatKind;
	newEntry["particleType"] = particleType;
	newEntry["particleName"] = particleName;
	newEntry["iMaxParticles"] = iMaxParticles;
	newEntry["iBehaviorType"] = iBehaviorType;
	newEntry["VSGroup"] = VSGroup;
	newEntry["VSID"] = VSID;
	newEntry["PSGroup"] = PSGroup;
	newEntry["PSID"] = PSID;

	std::string arrayKey;

	if (whatKind == "TEXTURE")
	{
		arrayKey = "textures";
		newEntry["TextureID1"] = textureID1;
		newEntry["TextureID2"] = textureID2;
		newEntry["RowCount"] = RowCount;
		newEntry["ColCount"] = ColCount;
		if (particleType == "PARTICLE_CPU") {
			newEntry["VIBufferID1"] = viBufferID1;
			newEntry["VIBufferID2"] = viBufferID2;
		}
	}
	else if (whatKind == "MESH")
	{
		arrayKey = "models";
		newEntry["sGroupTag"] = sGroupTag;
		newEntry["sResTag"] = sResTag;
	}
	else
	{
		return E_FAIL;
	}

	if (!j.contains(arrayKey) || !j[arrayKey].is_array())
		j[arrayKey] = nlohmann::json::array();

	bool bReplaced = false;
	for (auto& entry : j[arrayKey])
	{
		if (entry.contains("path") && entry["path"].is_string() &&
			entry.contains("particleType") && entry["particleType"].is_string() &&
			entry["path"].get<std::string>() == fullPath &&
			entry["particleType"].get<std::string>() == particleType)
		{
			entry = newEntry;
			bReplaced = true;
			break;
		}
	}

	if (!bReplaced)
		j[arrayKey].push_back(newEntry);

	std::ofstream file(savePath, std::ios::out);
	if (!file.is_open())
		return E_FAIL;

	file << j.dump(4);
	file.close();

	return S_OK;
}

CParticle* CParticleManager::GetParticle(const StringID& sGroupTag, const StringID& sTypeTag) const
{
	auto groupIt = m_Particles.find(sGroupTag);
	if (groupIt == m_Particles.end())
		return nullptr;

	auto typeIt = groupIt->second.find(sTypeTag);
	if (typeIt == groupIt->second.end())
		return nullptr;

	return typeIt->second.get();
}

bool CParticleManager::HasGroup(const StringID& sGroupTag) const
{
	return m_Particles.find(sGroupTag) != m_Particles.end();
}

UPtr<CParticleManager> CParticleManager::Create()
{
	return UPtr<CParticleManager>(new CParticleManager{});
}

HRESULT CParticleManager::ExecuteCommandQueue()
{
	HRESULT hr = S_OK;

	std::map<std::pair<StringID, StringID>, std::vector<PARTICLE_SPAWN_DATA>> batched;

	for (auto& cmd : m_vecCommandQueue)
	{
		if (cmd.sGroupTag_KindTag == SPAWN_COMMAND_KIND::STANDARD)
		{
			const auto& p = std::get<STANDARD_PARAMS>(cmd.params);

			auto& vec = batched[{cmd.sGroupTag, cmd.sTypeTag}];
			for (uint32_t i = 0; i < p.count; ++i)
			{
				PARTICLE_SPAWN_DATA s{};
				s.position = p.position;
				s.velocity = p.velocity;
				s.life = p.life;
				s.size = p.size;
				s.color = p.color;
				s.emissive = p.emissive;
				vec.push_back(s);
			}
		}
		else if (cmd.sGroupTag_KindTag == SPAWN_COMMAND_KIND::BEAM)
		{
			const auto& p = std::get<BEAM_PARAMS>(cmd.params);

			auto pParticle = GetParticle(cmd.sGroupTag, cmd.sTypeTag);
			if (pParticle)
			{
				auto pBeam = static_cast<CBeam_CPU*>(pParticle);
				pBeam->AddBeam(p.beamStart, p.beamEnd,
					p.fDisplacementAmplitude, (uint32_t)p.iDisplacementIterations, p.fDisplacementDamping,
					p.flickerTimeInverval, p.color, p.emissive, p.beamDuration);
			}
			else hr = E_FAIL;
		}
		else if (cmd.sGroupTag_KindTag == SPAWN_COMMAND_KIND::STAIR)
		{
			const auto& p = std::get<STAIR_PARAMS>(cmd.params);

			auto spawnList = ParticlePattern::MakeStairs(
				p.vStartPos, p.iStepCount, p.fStepWidth, p.fStepHeight, p.fStepDepth,
				p.life, p.color, p.emissive);

			auto& vec = batched[{cmd.sGroupTag, cmd.sTypeTag}];
			vec.insert(vec.end(), spawnList.begin(), spawnList.end());
		}
		else if (cmd.sGroupTag_KindTag == SPAWN_COMMAND_KIND::STRAIGHT)
		{
			const auto& p = std::get<STRAIGHT_PARAMS>(cmd.params);

			auto spawnList = ParticlePattern::MakeStrightGround(p.vStartPos, p.row, p.col, p.offSetX, p.offsetZ, p.spawnDelay, p.fSize, p.fLife, p.color, p.emissive);

			auto& vec = batched[{cmd.sGroupTag, cmd.sTypeTag}];
			vec.insert(vec.end(), spawnList.begin(), spawnList.end());
		}
	}

	for (auto& [key, spawnList] : batched)
	{
		if (FAILED(Spawn(key.first, key.second, (uint32_t)spawnList.size(), spawnList.data())))
			hr = E_FAIL;
	}

	return hr;
}
HRESULT CParticleManager::LoadParticleJson(const std::string& strJsonPath)
{
	if (!std::filesystem::exists(strJsonPath))
		return E_FAIL;

	std::ifstream file(strJsonPath);
	if (!file.is_open())
		return E_FAIL;

	nlohmann::json j;

	try
	{
		file >> j;
	}
	catch (...)
	{
		return E_FAIL;
	}

	HRESULT hr = S_OK;

	// ==================================================
	// ---- 1. MESH ("models" 배열) ----
	// ==================================================
	if (j.contains("models") && j["models"].is_array())
	{
		for (const auto& entry : j["models"])
		{
		
			if (!entry.contains("path") || !entry["path"].is_string())
				continue;

			std::string fbxPath = entry["path"].get<std::string>();
			if (fbxPath.empty())
				continue;

			std::string whatKind = entry.value("whatKind", "MESH");

			if (whatKind != "MESH")
			{
				hr = E_FAIL;   // models 배열인데 whatKind가 MESH가 아니면 데이터 이상
				continue;
			}
			std::string particleType = entry.value("particleType", "");
			std::string particleName = entry.value("particleName", "");
			std::string sGroupTag = entry.value("sGroupTag", "");
			std::string sResTag = entry.value("sResTag", "Static_Model_Resource");
			int iMaxParticles = entry.value("iMaxParticles", 1000);
			int iBehaviorType = entry.value("iBehaviorType", 0);
			std::string VSGroup = entry.value("VSGroup", "");
			std::string VSID = entry.value("VSID", "");
			std::string PSGroup = entry.value("PSGroup", "");
			std::string PSID = entry.value("PSID", "");
		
			if (particleType.empty() || sGroupTag.empty() || particleName.empty())
			{
				hr = E_FAIL;
				continue;
			}

			// ---- 모델 리소스 등록 (아직 등록 안 되어 있으면) ----
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(
				sGroupTag, sResTag, CResStaticModel::Create(fbxPath)))
			{
				E::CResStaticModel::DESC modelDesc{};
				modelDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);
				if (FAILED(res->Load(modelDesc)))
				{
					hr = E_FAIL;
					continue;
				}
			}

			// ---- particleType에 맞게 실제 클래스 생성 ----
			UPtr<CParticle> particle;

			if (particleType == "PARTICLE_GPU")
			{
				CParticle_GPU::DESC desc{};
				desc.iMaxParticles = iMaxParticles;
				desc.iBehaviorType = iBehaviorType;
				desc.whatKind = MESHORTEXTURE::MESH;
				desc.sGroupTag = sGroupTag;
				desc.sResTag = sResTag;

				if (!VSGroup.empty() && !VSID.empty())
					desc.VSID = { VSGroup, VSID };

				if (!PSGroup.empty() && !PSID.empty())
					desc.PSID = { PSGroup, PSID };

				particle = CParticle_GPU::Create(&desc);
			}
			else if (particleType == "PARTICLE_CPU")
			{
	
				CParticle_CPU::DESC desc{};
				desc.iMaxParticles = iMaxParticles;
				desc.whatKind = MESHORTEXTURE::MESH;
				desc.sGroupTag = sGroupTag;
				desc.sResTag = sResTag;

				if (!VSGroup.empty() && !VSID.empty())
					desc.VSID = { VSGroup, VSID };

				if (!PSGroup.empty() && !PSID.empty())
					desc.PSID = { PSGroup, PSID };

				///TODO
				particle = CParticle_CPU::Create(&desc);
			}
			else if (particleType == "BEAM_CPU")
			{
				//아직 model을 이용하는게 없음
				//particle = CBeam_CPU::Create(&desc);  
			}
			else if (particleType == "TRAIL_CPU")
			{
				//아직 model을 이용하는게 없음

				//particle = CTrail_CPU::Create(&desc); 
			}
			else
			{
				hr = E_FAIL;
				continue;
			}

			if (!particle)
			{
				hr = E_FAIL;
				continue;
			}

			// ---- 등록 ----
			auto& typeMap = m_Particles[sGroupTag];
			if (typeMap.contains(particleName))
			{
				hr = E_FAIL;
				continue;
			}

			typeMap[particleName] = std::move(particle);
		}
	}

	// ==================================================
	// ---- 2. TEXTURE ("textures" 배열) ----
	// ==================================================
	if (j.contains("textures") && j["textures"].is_array())
	{
		for (const auto& entry : j["textures"])
		{
			if (!entry.contains("path") || !entry["path"].is_string())
				continue;

			std::string texPath = entry["path"].get<std::string>();
			if (texPath.empty())
				continue;
			std::string whatKind = entry.value("whatKind", "TEXTURE");

			if (whatKind != "TEXTURE")
			{
				hr = E_FAIL;
				continue;
			}

			std::string particleType = entry.value("particleType", "");
			std::string particleName = entry.value("particleName", "");
			std::string sGroupTag = entry.value("sGroupTag", "");
			int iMaxParticles = entry.value("iMaxParticles", 1000);
			int iBehaviorType = entry.value("iBehaviorType", 0);
			std::string VSGroup = entry.value("VSGroup", "");
			std::string VSID = entry.value("VSID", "");
			std::string PSGroup = entry.value("PSGroup", "");
			std::string PSID = entry.value("PSID", "");
			std::string textureID1 = entry.value("TextureID1", "");
			std::string textureID2 = entry.value("TextureID2", "");
			int RowCount = entry.value("RowCount", 1);
			int ColCount = entry.value("ColCount", 1);
			if (particleType.empty() || particleName.empty() || textureID1.empty() || textureID2.empty())
			{
				hr = E_FAIL;
				continue;
			}

			// ---- 텍스처 리소스 등록 ----
			if (auto res = CGameInstance::Get().AddResourceT<E::CResTexture2D>(
				textureID1, textureID2, E::CResTexture2D::Create(texPath)))
			{
				if (FAILED(res->Load()))
				{
					hr = E_FAIL;
					continue;
				}
			}

			// ---- particleType에 맞게 실제 클래스 생성 ----
			UPtr<CParticle> particle;

			if (particleType == "PARTICLE_GPU")
			{
				CParticle_GPU::DESC desc{};
				
				desc.iMaxParticles = iMaxParticles;
				desc.iBehaviorType = iBehaviorType;
				desc.whatKind = MESHORTEXTURE::TEX;
				desc.textureID = { textureID1, textureID2 };
				desc.TexRows = RowCount;
				desc.TexColumns = ColCount;
				if (!VSGroup.empty() && !VSID.empty())
					desc.VSID = { VSGroup, VSID };

				if (!PSGroup.empty() && !PSID.empty())
					desc.PSID = { PSGroup, PSID };

				particle = CParticle_GPU::Create(&desc);
			}
			else if (particleType == "PARTICLE_CPU")
			{
				std::string VIBufferID1 = entry.value("VIBufferID1", "");
				std::string VIBufferID2 = entry.value("VIBufferID2", "");

				CParticle_CPU::DESC desc{};
				desc.iMaxParticles = iMaxParticles;
				desc.whatKind = MESHORTEXTURE::TEX;
				desc.textureID = { textureID1, textureID2 };
				desc.viBufferID = { VIBufferID1, VIBufferID2 };
				if (!VSGroup.empty() && !VSID.empty())
					desc.VSID = { VSGroup, VSID };

				if (!PSGroup.empty() && !PSID.empty())
					desc.PSID = { PSGroup, PSID };
				desc.TexRows = RowCount;
				desc.TexColumns = ColCount;
				particle = CParticle_CPU::Create(&desc);
			}
			else if (particleType == "BEAM_CPU")
			{
				/*   std::pair<StringID, StringID> textureID;
        std::pair<StringID, StringID> VSID;
        std::pair<StringID, StringID> PSID;
        PARTICLE_TYPE type;
		*/
	

				CBeam_CPU::DESC desc;
				desc.textureID = { textureID1, textureID2 };
				if (!VSGroup.empty() && !VSID.empty())
					desc.VSID = { VSGroup, VSID };

				if (!PSGroup.empty() && !PSID.empty())
					desc.PSID = { PSGroup, PSID };
				particle = CBeam_CPU::Create(&desc);
				//PARTICLE_TYPE
			}
			else if (particleType == "RIBBON_CPU")
			{
				//particle = CTrail_CPU::Create(&desc);
			}
			else
			{
				hr = E_FAIL;
				continue;
			}

			if (!particle)
			{
				hr = E_FAIL;
				continue;
			}

			// ---- 등록 ----
			// sGroupTag가 json에 없을 수 있으니, 없으면 particleName을 그룹키로도 사용
			std::string groupKey = !sGroupTag.empty() ? sGroupTag : particleName;

			auto& typeMap = m_Particles[groupKey];
			if (typeMap.contains(particleName))
			{
				hr = E_FAIL;
				continue;
			}

			typeMap[particleName] = std::move(particle);
		}
	}

	return hr;
}
std::vector<std::string> ScanFbxFolder(const std::string& strJsonPath)
{
	std::vector<std::string> result;

	if (!std::filesystem::exists(strJsonPath))
		return result;

	std::ifstream file(strJsonPath);
	if (!file.is_open())
		return result;

	try
	{
		nlohmann::json j;
		file >> j;

		if (j.contains("fbx") && j["fbx"].is_array())
		{
			for (const auto& elem : j["fbx"])
			{
				if (elem.is_string())
					result.push_back(elem.get<std::string>());
			}
		}
	}
	catch (...)
	{
	}

	return result;
}

std::vector<std::string> ScanTextureFolder(const std::string& strTextureFolder)
{
	std::vector<std::string> result;

	if (!std::filesystem::exists(strTextureFolder))
		return result;

	for (const auto& entry : std::filesystem::directory_iterator(strTextureFolder))
	{
		if (!entry.is_regular_file())
			continue;

		std::string ext = entry.path().extension().string();

		if (_stricmp(ext.c_str(), ".png") == 0 ||
			_stricmp(ext.c_str(), ".dds") == 0 ||
			_stricmp(ext.c_str(), ".jpg") == 0 ||
			_stricmp(ext.c_str(), ".tga") == 0)
		{
			result.push_back(entry.path().filename().string());
		}
	}

	return result;
}

static std::unordered_map<std::string, ComPtr<ID3D11ShaderResourceView>> s_TextureThumbnailCache;

ID3D11ShaderResourceView* GetOrLoadTextureThumbnail(const std::string& fullPath)
{
	auto it = s_TextureThumbnailCache.find(fullPath);
	if (it != s_TextureThumbnailCache.end())
		return it->second.Get();

	ComPtr<ID3D11ShaderResourceView> pSRV;
	HRESULT hr = DirectX::CreateWICTextureFromFile(
		CGameInstance::Get().GetGraphicDevice().Get(),
		CGameInstance::Get().GetGraphicDeviceContext().Get(),
		std::wstring(fullPath.begin(), fullPath.end()).c_str(),
		nullptr, pSRV.GetAddressOf());

	if (FAILED(hr) || !pSRV)
		return nullptr;

	s_TextureThumbnailCache[fullPath] = pSRV;
	return pSRV.Get();
}
HRESULT CParticleManager::SaveCommandQueue(const std::string& strJsonPath)
{
	nlohmann::json j;
	j["commands"] = nlohmann::json::array();

	for (auto& cmd : m_vecCommandQueue)
	{
		nlohmann::json entry;
		entry["kind"] = (int)cmd.sGroupTag_KindTag;
		entry["sGroupTag"] = cmd.sGroupTag.GetDbgStr();
		entry["sTypeTag"] = cmd.sTypeTag.GetDbgStr();

		switch (cmd.sGroupTag_KindTag)
		{
		case SPAWN_COMMAND_KIND::STANDARD:
		{
			const auto& p = std::get<STANDARD_PARAMS>(cmd.params);
			entry["count"] = p.count;
			entry["position"] = { p.position.x, p.position.y, p.position.z };
			entry["velocity"] = { p.velocity.x, p.velocity.y, p.velocity.z };
			entry["life"] = p.life;
			entry["size"] = p.size;
			entry["color"] = { p.color.x, p.color.y, p.color.z, p.color.w };
			entry["emissive"] = { p.emissive.x, p.emissive.y, p.emissive.z, p.emissive.w };
			entry["fSpawnDelay"] = p.fSpawnDelay;
			entry["bLoop"] = p.bLoop;
			entry["fSpawnInterval"] = p.fSpawnInterval;
			break;
		}
		case SPAWN_COMMAND_KIND::BEAM:
		{
			const auto& p = std::get<BEAM_PARAMS>(cmd.params);
			entry["beamStart"] = { p.beamStart.x, p.beamStart.y, p.beamStart.z, p.beamStart.w };
			entry["beamEnd"] = { p.beamEnd.x, p.beamEnd.y, p.beamEnd.z, p.beamEnd.w };
			entry["iDisplacementIterations"] = p.iDisplacementIterations;
			entry["fDisplacementAmplitude"] = p.fDisplacementAmplitude;
			entry["fDisplacementDamping"] = p.fDisplacementDamping;
			entry["flickerTimeInverval"] = p.flickerTimeInverval;
			entry["beamDuration"] = p.beamDuration;
			entry["fSpawnDelay"] = p.fSpawnDelay;
			entry["color"] = { p.color.x, p.color.y, p.color.z, p.color.w };
			entry["emissive"] = { p.emissive.x, p.emissive.y, p.emissive.z, p.emissive.w };
			break;
		}
		case SPAWN_COMMAND_KIND::STAIR:
		{
			const auto& p = std::get<STAIR_PARAMS>(cmd.params);
			entry["vStartPos"] = { p.vStartPos.x, p.vStartPos.y, p.vStartPos.z };
			entry["iStepCount"] = p.iStepCount;
			entry["fStepWidth"] = p.fStepWidth;
			entry["fStepHeight"] = p.fStepHeight;
			entry["fStepDepth"] = p.fStepDepth;
			entry["life"] = p.life;
			entry["fSpawnDelay"] = p.fSpawnDelay;
			entry["color"] = { p.color.x, p.color.y, p.color.z, p.color.w };
			entry["emissive"] = { p.emissive.x, p.emissive.y, p.emissive.z, p.emissive.w };
			break;
		}
		case SPAWN_COMMAND_KIND::STRAIGHT:
		{
			const auto& p = std::get<STRAIGHT_PARAMS>(cmd.params);
			entry["vStartPos"] = { p.vStartPos.x, p.vStartPos.y, p.vStartPos.z };
			entry["row"] = p.row;
			entry["col"] = p.col;
			entry["offSetX"] = p.offSetX;
			entry["offsetZ"] = p.offsetZ;
			entry["spawnDelay"] = p.spawnDelay;
			entry["fSize"] = p.fSize;
			entry["fLife"] = p.fLife;
			entry["color"] = { p.color.x, p.color.y, p.color.z, p.color.w };
			entry["emissive"] = { p.emissive.x, p.emissive.y, p.emissive.z, p.emissive.w };
			break;
		}
		}

		j["commands"].push_back(entry);
	}

	std::filesystem::path savePath(strJsonPath);
	if (!savePath.parent_path().empty())
		std::filesystem::create_directories(savePath.parent_path());

	std::ofstream file(savePath);
	if (!file.is_open())
		return E_FAIL;

	file << j.dump(4);
	return S_OK;
}
HRESULT CParticleManager::LoadCommandQueue(const std::string& strJsonPath)
{
	if (!std::filesystem::exists(strJsonPath))
		return E_FAIL;

	std::ifstream file(strJsonPath);
	if (!file.is_open())
		return E_FAIL;

	nlohmann::json j;
	try
	{
		file >> j;
	}
	catch (...)
	{
		return E_FAIL;
	}

	if (!j.contains("commands") || !j["commands"].is_array())
		return E_FAIL;

	m_vecCommandQueue.clear();

	for (const auto& entry : j["commands"])
	{
		SPAWN_COMMAND cmd{};
		cmd.sGroupTag_KindTag = (SPAWN_COMMAND_KIND)entry.value("kind", 0);
		cmd.sGroupTag = entry.value("sGroupTag", "");
		cmd.sTypeTag = entry.value("sTypeTag", "");

		switch (cmd.sGroupTag_KindTag)
		{
		case SPAWN_COMMAND_KIND::STANDARD:
		{
			STANDARD_PARAMS p{};
			p.count = entry.value("count", 1u);

			auto pos = entry.value("position", std::vector<float>{0, 0, 0});
			p.position = { pos[0], pos[1], pos[2] };

			auto vel = entry.value("velocity", std::vector<float>{0, 0, 0});
			p.velocity = { vel[0], vel[1], vel[2] };

			p.life = entry.value("life", 1.f);
			p.size = entry.value("size", 1.f);

			auto col = entry.value("color", std::vector<float>{1, 1, 1, 1});
			p.color = { col[0], col[1], col[2], col[3] };

			auto emi = entry.value("emissive", std::vector<float>{0, 0, 0, 0});
			p.emissive = { emi[0], emi[1], emi[2], emi[3] };

			p.fSpawnDelay = entry.value("fSpawnDelay", 0.f);
			p.bLoop = entry.value("bLoop", false);
			p.fSpawnInterval = entry.value("fSpawnInterval", 0.f);

			cmd.params = p;
			break;
		}
		case SPAWN_COMMAND_KIND::BEAM:
		{
			BEAM_PARAMS p{};

			auto bs = entry.value("beamStart", std::vector<float>{0, 0, 0, 0});
			p.beamStart = { bs[0], bs[1], bs[2], bs[3] };

			auto be = entry.value("beamEnd", std::vector<float>{0, 0, 0, 0});
			p.beamEnd = { be[0], be[1], be[2], be[3] };

			p.iDisplacementIterations = entry.value("iDisplacementIterations", 0);
			p.fDisplacementAmplitude = entry.value("fDisplacementAmplitude", 0.f);
			p.fDisplacementDamping = entry.value("fDisplacementDamping", 0.f);
			p.flickerTimeInverval = entry.value("flickerTimeInverval", 0.f);
			p.beamDuration = entry.value("beamDuration", 0.f);
			p.fSpawnDelay = entry.value("fSpawnDelay", 0.f);

			auto col = entry.value("color", std::vector<float>{1, 1, 1, 1});
			p.color = { col[0], col[1], col[2], col[3] };

			auto emi = entry.value("emissive", std::vector<float>{0, 0, 0, 0});
			p.emissive = { emi[0], emi[1], emi[2], emi[3] };

			cmd.params = p;
			break;
		}
		case SPAWN_COMMAND_KIND::STAIR:
		{
			STAIR_PARAMS p{};

			auto sp = entry.value("vStartPos", std::vector<float>{0, 0, 0});
			p.vStartPos = { sp[0], sp[1], sp[2] };

			p.iStepCount = entry.value("iStepCount", 1u);
			p.fStepWidth = entry.value("fStepWidth", 1.f);
			p.fStepHeight = entry.value("fStepHeight", 1.f);
			p.fStepDepth = entry.value("fStepDepth", 1.f);
			p.life = entry.value("life", 1.f);
			p.fSpawnDelay = entry.value("fSpawnDelay", 0.f);

			auto col = entry.value("color", std::vector<float>{1, 1, 1, 1});
			p.color = { col[0], col[1], col[2], col[3] };

			auto emi = entry.value("emissive", std::vector<float>{0, 0, 0, 0});
			p.emissive = { emi[0], emi[1], emi[2], emi[3] };

			cmd.params = p;
			break;
		}
		case SPAWN_COMMAND_KIND::STRAIGHT:
		{
			STRAIGHT_PARAMS p{};

			auto sp = entry.value("vStartPos", std::vector<float>{0, 0, 0});
			p.vStartPos = { sp[0], sp[1], sp[2] };

			p.row = entry.value("row", 1u);
			p.col = entry.value("col", 1u);
			p.offSetX = entry.value("offSetX", 0.f);
			p.offsetZ = entry.value("offsetZ", 0.f);
			p.spawnDelay = entry.value("spawnDelay", 0.f);
			p.fSize = entry.value("fSize", 1.f);
			p.fLife = entry.value("fLife", 1.f);

			auto col = entry.value("color", std::vector<float>{1, 1, 1, 1});
			p.color = { col[0], col[1], col[2], col[3] };

			auto emi = entry.value("emissive", std::vector<float>{0, 0, 0, 0});
			p.emissive = { emi[0], emi[1], emi[2], emi[3] };

			cmd.params = p;
			break;
		}
		default:
			continue;
		}

		m_vecCommandQueue.push_back(cmd);
	}

	return S_OK;
}
