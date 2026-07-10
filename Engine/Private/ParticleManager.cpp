#include "pch.h"
#include "ParticleManager.h"
#include "Particle.h"
#include "Beam_CPU.h"
#include "Trail_CPU.h"
#include "ParticlePattern.h"
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
	ImGui::Begin("SaveJson");

	static const std::string kFbxListJsonPath = "./Resources/SampleClient/Models/ParticleModelJson/ParticleModel.json";
	static const std::string kJsonFolder = "./Resources/json/Particle";

	static std::vector<std::string> fbxFileList;
	static bool bFbxScanned = false;
	static int selectedFbxIndex = -1;
	static std::string selectedFbxPath;

	static char szJsonName[MAX_PATH] = "ParticleData.json";
	static int whatKindIndex = 0; // 0: MESH, 1: TEXTURE
	static char szParticleType[128] = "";
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

	static const std::string kTextureRealFolder = "./Resources/SampleClient/Textures/EffectParticle";
	static std::vector<std::string> textureFileList;
	static bool bTextureScanned = false;
	static int selectedTextureIndex = -1;
	static std::string selectedTexturePath;

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

			static const std::string kFbxRealFolder = "./SampleClient/Models/OriginData/Static/ParticleMeshes";

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
	ImGui::InputText("Particle Type (e.g. Particle_CPU)", szParticleType, IM_ARRAYSIZE(szParticleType));
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
				std::string particleTypeStr = szParticleType;
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
					Save_Binary_Json(savePath.string(),
						targetPath,
						whatKindStr,
						particleTypeStr,
						particleNameStr,
						iMaxParticles,
						iBehaviorType,
						szVSID1, szVSID2, szPSID1, szPSID2,
						szGroupTag, szResTag, szTextureID1, szTextureID2);
				}
			}
		}
	}

	ImGui::End();
	ImGui::Begin("CParticleManager");

	static StringID selectedGroup;
	static StringID selectedType;
	static int groupIndex = 0;
	static int typeIndex = 0;
	static SPAWN_COMMAND_KIND currentKind = SPAWN_COMMAND_KIND::STANDARD;

	static STANDARD_PARAMS pendingStandard{};
	static BEAM_PARAMS     pendingBeam{};
	static STAIR_PARAMS    pendingStair{};
	static STRAIGHT_PARAMS pendingStraight{};

	std::vector<StringID> groupKeys;
	groupKeys.reserve(m_Particles.size());
	for (auto& [groupTag, typeMap] : m_Particles)
		groupKeys.push_back(groupTag);

	if (!groupKeys.empty())
	{
		std::vector<std::string> groupNamesStorage;
		groupNamesStorage.reserve(groupKeys.size());
		for (auto& key : groupKeys)
			groupNamesStorage.emplace_back(key.GetDbgStr());

		std::vector<const char*> groupNames;
		for (auto& s : groupNamesStorage)
			groupNames.push_back(s.c_str());

		groupIndex = std::clamp(groupIndex, 0, (int)groupNames.size() - 1);
		if (ImGui::Combo("Group", &groupIndex, groupNames.data(), (int)groupNames.size()))
			typeIndex = 0;

		selectedGroup = groupKeys[groupIndex];

		auto& typeMap = m_Particles[selectedGroup];
		std::vector<StringID> typeKeys;
		typeKeys.reserve(typeMap.size());
		for (auto& [typeTag, particle] : typeMap)
			typeKeys.push_back(typeTag);

		if (!typeKeys.empty())
		{
			std::vector<std::string> typeNamesStorage;
			typeNamesStorage.reserve(typeKeys.size());
			for (auto& key : typeKeys)
				typeNamesStorage.emplace_back(key.GetDbgStr());

			std::vector<const char*> typeNames;
			for (auto& s : typeNamesStorage)
				typeNames.push_back(s.c_str());

			typeIndex = std::clamp(typeIndex, 0, (int)typeNames.size() - 1);
			ImGui::Combo("Type", &typeIndex, typeNames.data(), (int)typeNames.size());

			selectedType = typeKeys[typeIndex];

			auto pSelected = typeMap[selectedType].get();
			if (dynamic_cast<CBeam_CPU*>(pSelected) != nullptr)
				currentKind = SPAWN_COMMAND_KIND::BEAM;
		}
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

	if (ImGui::Button("Add to List") && !groupKeys.empty())
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
	const std::string& textureID2)
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

HRESULT CParticleManager::LoadParticleModelJson(const std::string& strJsonPath, const std::string& strFbxRealFolder)
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

	if (!j.contains("models") || !j["models"].is_array())
		return E_FAIL;

	HRESULT hr = S_OK;

	for (const auto& entry : j["models"])
	{
		if (!entry.contains("path") || !entry["path"].is_string())
			continue;

		std::string fbxPath = entry["path"].get<std::string>();
		if (fbxPath.empty())
			continue;

		std::string particleType = entry.value("particleType", "");
		std::string sGroupTag = entry.value("sGroupTag", "");

		if (particleType.empty() || sGroupTag.empty())
			continue;

		// TODO: 실제 파티클 생성/등록 로직은 프로젝트 구조에 맞게 채워야 함
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

static std::unordered_map<std::string, ID3D11ShaderResourceView*> s_TextureThumbnailCache;

ID3D11ShaderResourceView* GetOrLoadTextureThumbnail(const std::string& fullPath)
{
	auto it = s_TextureThumbnailCache.find(fullPath);
	if (it != s_TextureThumbnailCache.end())
		return it->second;

	ID3D11ShaderResourceView* pSRV = nullptr;

	HRESULT hr = DirectX::CreateWICTextureFromFile(CGameInstance::Get().GetGraphicDevice().Get(), CGameInstance::Get().GetGraphicDeviceContext().Get(),
		std::wstring(fullPath.begin(), fullPath.end()).c_str(), nullptr, &pSRV);

	if (SUCCEEDED(hr) && pSRV)
		s_TextureThumbnailCache[fullPath] = pSRV;
	else
		pSRV = nullptr;

	return pSRV;
}
