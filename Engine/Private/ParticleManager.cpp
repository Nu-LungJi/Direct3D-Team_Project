#include "pch.h"
#include "ParticleManager.h"
#include "Particle.h"
#include "Beam_CPU.h"
#include "Trail_CPU.h"
#include "ParticlePattern.h"
#include "Particle_GPU.h"
#include "Particle_CPU.h"
#include "ParticleParamImGui.h"
#include "EffectManager.h"
NS_USING(Engine)

std::vector<std::string> ScanFbxFolder(const std::string& strFbxFolder);
std::vector<std::string> ScanTextureFolder(const std::string& strFbxFolder);


void DrawPatternEditor(PatternParamVariant& current)
{
	// 패턴 종류 선택 콤보박스 (생략)

	std::visit([](auto& param)
		{
			DrawImGui(param); // 타입에 맞는 오버로드가 자동 선택됨
		}, current);
}

CParticleManager::CParticleManager()
{
}

CParticleManager::~CParticleManager()
{
}

void CParticleManager::UpdateGUI()
{
	static TextureSlotState slotDiffuse{ "Diffuse",    "SAMPLE_CLINET_TEXTURE", "TEX_RIBBON" };
	static TextureSlotState slotNormal{ "Normal",     "SAMPLE_CLINET_TEXTURE", "TEX_RIBBONNORMAL" };
	static TextureSlotState slotDistortion{ "Distortion", "SAMPLE_CLINET_TEXTURE", "TEX_RIBBONDISTORTION" };
	static TextureSlotState slotNoise{ "Noise",      "SAMPLE_CLINET_TEXTURE", "TEX_RIBBONNOISE" };
	static TextureSlotState slotPositionHdr{ "HdrPosition",      "SAMPLE_CLINET_TEXTURE", "TEX_HDRPOSITION" };
	static TextureSlotState slotNormalHdr{ "HdrNormal",      "SAMPLE_CLINET_TEXTURE", "TEX_HDRNORMAL" };
	static TextureSlotState slotEmpty{ "AnyTexture",      "SAMPLE_CLINET_TEXTURE", "" };

	static TextureSlotState* slots[] = { &slotDiffuse, &slotNormal, &slotDistortion, &slotNoise,&slotEmpty ,&slotPositionHdr ,&slotNormalHdr };
	static int activeSlotIndex = 0;
	// ---- 슬롯별 폴더 경로 (인덱스가 slots[]와 1:1 대응) ----
	static const std::string kTextureFolders[7] = {
		"./Resources/SampleClient/Textures/EffectParticle/Diffuse",
		"./Resources/SampleClient/Textures/EffectParticle/Normal",
		"./Resources/SampleClient/Textures/EffectParticle/Distortion",
		"./Resources/SampleClient/Textures/EffectParticle/Noise",
		 "./Resources/SampleClient/Textures/EffectParticle/AnyTexture",
		"./Resources/SampleClient/Textures/EffectParticle/HdrPosition",
		 "./Resources/SampleClient/Textures/EffectParticle/HdrNormal",
	};

	// ---- 슬롯별 파일 목록 (크기 4로 미리 확보) ----
	static std::vector<std::vector<std::string>> textureFileList(5);
	static bool bTextureScanned = false;

	static char szPresetName[128] = "";
	static char szPresetSavePath[MAX_PATH] = "./Resources/json/Particle/Preset/ParticlePresets.json";
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
	static int iMaxParticles = 1000;
	static uint32_t iBehaviorType = 0;
	static  _string VSIDIName{};
	static  _string PSIDIName{};
	static char szGroupTag[128] = "Rock1";
	static char szResTag[128] = "Static_Model_Resource";
	static char szViBuffer1[128] = "SAMPLE_CLIENT_PARTICLEBF";
	static char szViBuffer2[128] = "VIBUF_ParticleQuad";
	static char VSEntryPoint[128] = "VSMain";
	static char PSEntryPoint[128] = "PSMain";


	
	static int blendType = 0;

	static int iTexRow = 1;
	static int iTexCol = 1;

	static _bool none = false;
	static _bool distortion = false;
	static _bool billboard = false;
	static _bool gravity = false;
	static _bool circleToWave = false;
	static _bool bSmoke = false;
	static _bool bSmokeJump = false;
	static _bool bSmokegv = false;
	static _bool bSmokegw = false;

	static _bool alphaBlend = false;
	static _bool alphaAdd = false;
	static _bool noneBlend = false;
	static int	iSelectedBlend = 0;

	
	static _float4 rotaion = _float4(0, 0, 0, 0);
	static int iGeometryType = 0;

	static const std::string kFbxRealFolder = "./Resources/SampleClient/Models/ParticleMeshes";
	static bool bUseHdrForMesh = false;
	static std::vector<std::string> hdrFileList;
	static bool bHdrScanned = false;
	static int selectedHdrIndex = -1;
	static std::vector<std::string> hdrNormalFileList;   
	static bool bHdrNormalScanned = false;           
	static int selectedHdrNormalIndex = -1;

	ImGui::Begin("SaveResourcesAsJson");



	// ---- 0. WhatKind 대분류 선택 ----
	const char* whatKindNames[] = { "MESH", "TEXTURE" };
	ImGui::Combo("WhatKind", &whatKindIndex, whatKindNames, IM_ARRAYSIZE(whatKindNames));

	ImGui::Separator();

	// ---- 1. MESH일 때만: fbx 섹션 ----
	if (whatKindIndex == 0)
	{
		if (ImGui::Button("Rescan Fbx Folder"))
			fbxFileList = ScanBinFolder(kFbxRealFolder);

		if (!bFbxScanned)
		{
			fbxFileList = ScanBinFolder(kFbxRealFolder);
			//fbxFileList = ScanFbxFolder(kFbxListJsonPath);
			bFbxScanned = true;
		}

		
		ImGui::Text("Fbx Files:");

		if (!fbxFileList.empty())
		{
			std::vector<const char*> namesForCombo;
			for (auto& s : fbxFileList)
				namesForCombo.push_back(s.c_str());
			

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

		ImGui::Separator();


		ImGui::Checkbox("Use HDR (VAT)", &bUseHdrForMesh);
		if (bUseHdrForMesh)
		{
			// ---- Position (VP) ----
			bool bRescanPosClicked = ImGui::Button("Rescan Hdr Position Folder");
			if (!bHdrScanned || bRescanPosClicked)
			{
				hdrFileList = ScanTextureFolder(kTextureFolders[5]); // HdrPosition 폴더
				bHdrScanned = true;
			}
			if (!hdrFileList.empty())
			{
				std::vector<const char*> namesForCombo;
				for (auto& s : hdrFileList)
					namesForCombo.push_back(s.c_str());
				if (ImGui::Combo("Hdr Position List", &selectedHdrIndex, namesForCombo.data(), (int)namesForCombo.size()))
				{
					slotPositionHdr.selectedIndex = selectedHdrIndex;
					slotPositionHdr.selectedPath = kTextureFolders[5] + "/" + hdrFileList[selectedHdrIndex];
				}
				if (!slotPositionHdr.selectedPath.empty())
					ImGui::Text("Selected(Position): %s", slotPositionHdr.selectedPath.c_str());
			}
			else
			{
				ImGui::Text("(HdrPosition 폴더에 .hdr 파일 없음)");
			}
			ImGui::InputText("Hdr Position TextureID1", slotPositionHdr.szTextureID1, IM_ARRAYSIZE(slotPositionHdr.szTextureID1));
			ImGui::InputText("Hdr Position TextureID2", slotPositionHdr.szTextureID2, IM_ARRAYSIZE(slotPositionHdr.szTextureID2));

			ImGui::Separator();

			// ---- Normal (VN) ----
			bool bRescanNormalClicked = ImGui::Button("Rescan Hdr Normal Folder");
			if (!bHdrNormalScanned || bRescanNormalClicked)
			{
				hdrNormalFileList = ScanTextureFolder(kTextureFolders[6]); // HdrNormal 폴더
				bHdrNormalScanned = true;
			}
			if (!hdrNormalFileList.empty())
			{
				std::vector<const char*> namesForCombo;
				for (auto& s : hdrNormalFileList)
					namesForCombo.push_back(s.c_str());
				if (ImGui::Combo("Hdr Normal List", &selectedHdrNormalIndex, namesForCombo.data(), (int)namesForCombo.size()))
				{
					slotNormalHdr.selectedIndex = selectedHdrNormalIndex;
					slotNormalHdr.selectedPath = kTextureFolders[6] + "/" + hdrNormalFileList[selectedHdrNormalIndex];
				}
				if (!slotNormalHdr.selectedPath.empty())
					ImGui::Text("Selected(Normal): %s", slotNormalHdr.selectedPath.c_str());
			}
			else
			{
				ImGui::Text("(HdrNormal 폴더에 .hdr 파일 없음)");
			}
			ImGui::InputText("Hdr Normal TextureID1", slotNormalHdr.szTextureID1, IM_ARRAYSIZE(slotNormalHdr.szTextureID1));
			ImGui::InputText("Hdr Normal TextureID2", slotNormalHdr.szTextureID2, IM_ARRAYSIZE(slotNormalHdr.szTextureID2));
		}
		ImGui::Separator();
		ImGui::Text("Optional Textures (Distortion / Noise)");

		// 썸네일 그리드 그리기 (기존 TEXTURE 모드와 동일한 방식)
		auto DrawTextureThumbnailPicker = [&](const std::string& folder, std::vector<std::string>& fileList, TextureSlotState& slot)
			{
				const float thumbnailSize = 64.0f;
				const float cellPadding = 10.0f;
				const float cellWidth = thumbnailSize + cellPadding;
				int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / cellWidth));
				int i = 0;
				for (auto& texName : fileList)
				{
					std::string fullPath = folder + "/" + texName;
					ID3D11ShaderResourceView* pSRV = GetOrLoadTextureThumbnail(fullPath);
					ImGui::PushID(i);
					ImGui::BeginGroup();
					if (pSRV)
					{
						if (ImGui::ImageButton((ImTextureID)pSRV, ImVec2(thumbnailSize, thumbnailSize)))
						{
							slot.selectedIndex = i;
							slot.selectedPath = fullPath;
							strcpy_s(slot.szTextureID2,texName.c_str());
						}
					}
					else
					{
						if (ImGui::Button("No Img", ImVec2(thumbnailSize, thumbnailSize)))
						{
							slot.selectedIndex = i;
							slot.selectedPath = fullPath;
						}
					}
					ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + thumbnailSize);
					ImGui::TextWrapped("%s", texName.c_str());
					ImGui::PopTextWrapPos();
					ImGui::EndGroup();
					if (slot.selectedIndex == i)
					{
						ImVec2 minPos = ImGui::GetItemRectMin();
						ImVec2 maxPos = ImGui::GetItemRectMax();
						ImGui::GetWindowDrawList()->AddRect(minPos, maxPos, IM_COL32(255, 200, 0, 255), 0.0f, 0, 2.0f);
					}
					ImGui::PopID();
					if ((i + 1) % columns != 0) ImGui::SameLine(0.0f, cellPadding);
					i++;
				}
				ImGui::NewLine();
			};

		// ---- Distortion ----
		static bool bDistortionScannedForMesh = false;
		static std::vector<std::string> distortionFileListForMesh;
		bool bRescanDistortionClicked = ImGui::Button("Rescan Distortion Folder");
		if (!bDistortionScannedForMesh || bRescanDistortionClicked)
		{
			distortionFileListForMesh = ScanTextureFolder(kTextureFolders[2]); 
			bDistortionScannedForMesh = true;
		}
		if (!slotDistortion.selectedPath.empty())
		{
			ImGui::Text("Selected(Distortion): %s", slotDistortion.selectedPath.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Clear##Distortion"))
			{
				slotDistortion.selectedIndex = -1;
				slotDistortion.selectedPath.clear();
			}
		}
		if (!distortionFileListForMesh.empty())
			DrawTextureThumbnailPicker(kTextureFolders[2], distortionFileListForMesh, slotDistortion);
		else
			ImGui::Text("(폴더에 텍스처 파일 없음)");

		//slotDistortion.szTextureID2 = t
		ImGui::InputText("Distortion TextureID1", slotDistortion.szTextureID1, IM_ARRAYSIZE(slotDistortion.szTextureID1));
		ImGui::Text("Distortion TextureID2 : %s", slotDistortion.szTextureID2);// , IM_ARRAYSIZE(slotDistortion.szTextureID2));
		ImGui::Separator();

		// ---- Noise ----
		static bool bNoiseScannedForMesh = false;
		static std::vector<std::string> noiseFileListForMesh;
		bool bRescanNoiseClicked = ImGui::Button("Rescan Noise Folder");
		if (!bNoiseScannedForMesh || bRescanNoiseClicked)
		{
			noiseFileListForMesh = ScanTextureFolder(kTextureFolders[3]);
			bNoiseScannedForMesh = true;
		}
		if (!slotNoise.selectedPath.empty())
		{
			ImGui::Text("Selected(Noise): %s", slotNoise.selectedPath.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Clear##Noise"))
			{
				slotNoise.selectedIndex = -1;
				slotNoise.selectedPath.clear();
			}
		}
		if (!noiseFileListForMesh.empty())
			DrawTextureThumbnailPicker(kTextureFolders[3], noiseFileListForMesh, slotNoise);
		else
			ImGui::Text("(폴더에 텍스처 파일 없음)");
		ImGui::Text("NoiseID2 : %s", slotNoise.szTextureID2);// , IM_ARRAYSIZE(slotDistortion.szTextureID2));

		ImGui::Separator();
		// ---- Any Texture ----
		static bool bAnyTextureScannedForMesh = false;
		static std::vector<std::string> AnyFileListForMesh;
		bool bRescanAnyClicked = ImGui::Button("Rescan AnyTexture Folder");
		if (!bAnyTextureScannedForMesh || bRescanAnyClicked)
		{
			AnyFileListForMesh = ScanTextureFolder(kTextureFolders[4]);
			bAnyTextureScannedForMesh = true;
		}
		if (!slotEmpty.selectedPath.empty())
		{
			ImGui::Text("Selected(Texture): %s", slotEmpty.selectedPath.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Clear##Texture"))
			{
				slotEmpty.selectedIndex = -1;
				slotEmpty.selectedPath.clear();
			}
		}
		if (!AnyFileListForMesh.empty())
			DrawTextureThumbnailPicker(kTextureFolders[4], AnyFileListForMesh, slotEmpty);
		else
			ImGui::Text("(폴더에 텍스처 파일 없음)");

		//slotDistortion.szTextureID2 = t
		ImGui::InputText("Any TextureID1", slotEmpty.szTextureID1, IM_ARRAYSIZE(slotEmpty.szTextureID1));
		ImGui::Text("Any TextureID2 : %s", slotEmpty.szTextureID2);// , IM_ARRAYSIZE(slotDistortion.szTextureID2));
		ImGui::Separator();
		//ImGui::InputText("Noise TextureID1", slotNoise.szTextureID1, IM_ARRAYSIZE(slotNoise.szTextureID1));
		//ImGui::InputText("Noise TextureID2", slotNoise.szTextureID2, IM_ARRAYSIZE(slotNoise.szTextureID2));
	}

	// ---- 2. TEXTURE일 때만: 텍스처 섹션 ----
	if (whatKindIndex == 1)
	{
		if (!bTextureScanned)
		{
			for (int s = 0; s < 5; ++s)
				textureFileList[s] = ScanTextureFolder(kTextureFolders[s]);
			bTextureScanned = true;
		}

		if (ImGui::Button("Rescan Texture Folder"))
		{
			for (int s = 0; s < 5; ++s)
				textureFileList[s] = ScanTextureFolder(kTextureFolders[s]);
		}

		// ---- 슬롯 선택 탭 ----
		if (ImGui::BeginTabBar("TextureSlots"))
		{
			for (int s = 0; s < 5; ++s)
			{
				if (ImGui::BeginTabItem(slots[s]->label.c_str()))
				{
					activeSlotIndex = s;
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}

		TextureSlotState& slot = *slots[activeSlotIndex];
		_bool bOptionalSlot = (activeSlotIndex != 0); // Diffuse(0번)만 필수

		if (bOptionalSlot)
		{
			if (slot.selectedPath.empty())
			{
				ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.f), "(사용 안 함)");
			}
			else
			{
				if (ImGui::Button("None (사용 안 함)"))
				{
					slot.selectedIndex = -1;
					slot.selectedPath.clear();
				}
				ImGui::SameLine();
				ImGui::Text("현재: %s", slot.selectedPath.c_str());
			}
			ImGui::Separator();
		}

		// ---- 지금 활성화된 탭의 폴더/파일목록만 순회 ----
		const float thumbnailSize = 64.0f;
		const float cellPadding = 10.0f;
		const float cellWidth = thumbnailSize + cellPadding;
		int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / cellWidth));

		const std::string& currentFolder = kTextureFolders[activeSlotIndex];
		auto& currentFileList = textureFileList[activeSlotIndex];

		int i = 0;
		for (auto& texName : currentFileList)
		{
			std::string fullPath = currentFolder + "/" + texName;
			ID3D11ShaderResourceView* pSRV = GetOrLoadTextureThumbnail(fullPath);

			ImGui::PushID(i);
			ImGui::BeginGroup();

			if (pSRV)
			{
				if (ImGui::ImageButton((ImTextureID)pSRV, ImVec2(thumbnailSize, thumbnailSize)))
				{
					slot.selectedIndex = i;
					slot.selectedPath = fullPath;
					strcpy_s(slot.szTextureID2,texName.c_str());
				}
			}
			else
			{
				if (ImGui::Button("No Img", ImVec2(thumbnailSize, thumbnailSize)))
				{
					slot.selectedIndex = i;
					slot.selectedPath = fullPath;
				}
			}

			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + thumbnailSize);
			ImGui::TextWrapped("%s", texName.c_str());
			ImGui::PopTextWrapPos();
			ImGui::EndGroup();

			if (slot.selectedIndex == i)
			{
				ImVec2 minPos = ImGui::GetItemRectMin();
				ImVec2 maxPos = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddRect(minPos, maxPos, IM_COL32(255, 200, 0, 255), 0.0f, 0, 2.0f);
			}

			ImGui::PopID();
			if ((i + 1) % columns != 0) ImGui::SameLine(0.0f, cellPadding);
			i++;
		}

		ImGui::NewLine();
		if (!slot.selectedPath.empty())
			ImGui::Text("Selected (%s): %s", slot.label.c_str(), slot.selectedPath.c_str());

		//ImGui::InputText((slot.label + " TextureID1").c_str(), slot.szTextureID1, IM_ARRAYSIZE(slot.szTextureID1));
		//ImGui::InputText((slot.label + " TextureID2").c_str(), slot.szTextureID2, IM_ARRAYSIZE(slot.szTextureID2));

		// ---- 다른 슬롯들 요약 표시 ----
		ImGui::Separator();
		ImGui::Text("Summary:");
		for (int s = 0; s < 4; ++s)
		{
			if (s == 0)
				ImGui::BulletText("%s: %s", slots[s]->label.c_str(),
					slots[s]->selectedPath.empty() ? "(미선택)" : slots[s]->selectedPath.c_str());
			else
				ImGui::BulletText("%s: %s", slots[s]->label.c_str(),
					slots[s]->selectedPath.empty() ? "(사용 안 함)" : slots[s]->selectedPath.c_str());
		}
	}

	ImGui::Separator();

	// ---- 3. 공통 값 입력 ----
	ImGui::InputText("Json Name", szJsonName, IM_ARRAYSIZE(szJsonName));
	ImGui::Combo("Particle Type", &particleTypeIndex, particleTypeNames, IM_ARRAYSIZE(particleTypeNames));
	ImGui::InputText("Particle Name (e.g. ROCK1_CPU)", szParticleName, IM_ARRAYSIZE(szParticleName));
	ImGui::InputInt("MaxParticles", &iMaxParticles);
	ComboList("VSID1", "PERMANENT_PARTICLE_VSSHADER", VSIDIName);
	ImGui::InputText("VS EntryPoint (e.g. VSMain)", VSEntryPoint, IM_ARRAYSIZE(VSEntryPoint));
	ComboList("PSID1", "PERMANENT_PARTICLE_PSSHADER", PSIDIName);
	ImGui::InputText("PS EntryPoint (e.g. PSMain)", PSEntryPoint, IM_ARRAYSIZE(PSEntryPoint));
	

	//ImGui::InputText("PSID2", szPSID2, IM_ARRAYSIZE(szPSID2));

	ImGui::InputInt("TexRowCount", &iTexRow);
	ImGui::InputInt("TexColCount", &iTexCol);

	ImGui::Separator();

	// ---- 4. 저장 ----
	std::string particleNameStr = szParticleName;
	// Diffuse 슬롯이 실질적인 "대표 텍스처 경로"
	std::string targetPath = (whatKindIndex == 1) ? slotDiffuse.selectedPath : selectedFbxPath;


	std::string particleTypeStr = particleTypeNames[particleTypeIndex];
	std::string jsonNameStr = szJsonName;

	if (particleTypeStr == "BEAM_CPU") {
		ImGui::Text("Beam GeometryType");
		ImGui::InputInt("GeometryType", &iGeometryType);
	}
	else {
		ImGui::InputText("VIBuffer1 if CPUTEX", szViBuffer1, IM_ARRAYSIZE(szViBuffer1));
		ImGui::InputText("VIBuffer2  if CPUTEX", szViBuffer2, IM_ARRAYSIZE(szViBuffer2));
	}

	auto IsCombinationSupported = [](int whatKindIdx, const std::string& particleType) -> bool
		{
			if (whatKindIdx == 0) // MESH
				return (particleType == "PARTICLE_GPU" || particleType == "PARTICLE_CPU");
			else // TEXTURE
				return (particleType == "PARTICLE_GPU" || particleType == "PARTICLE_CPU" || particleType == "BEAM_CPU" || particleType == "TRAIL_CPU");
		};

	bool bSupported = IsCombinationSupported(whatKindIndex, particleTypeStr);

	std::vector<std::string> vecErrors;

	if (targetPath.empty())
		vecErrors.push_back(whatKindIndex == 0 ? "Fbx 파일을 선택하세요." : "Diffuse 텍스처를 선택하세요.");

	if (particleNameStr.empty())
		vecErrors.push_back("Particle Name을 입력하세요.");

	if (jsonNameStr.empty())
		vecErrors.push_back("Json Name을 입력하세요.");

	if (whatKindIndex == 0)
	{
		if (std::string(szGroupTag).empty())
			vecErrors.push_back("GroupTag를 입력하세요.");
		if (std::string(szResTag).empty())
			vecErrors.push_back("ResTag를 입력하세요.");
	}
	else
	{
		if (std::string(slotDiffuse.szTextureID1).empty() || std::string(slotDiffuse.szTextureID2).empty())
			vecErrors.push_back("Diffuse TextureID1/2를 입력하세요.");

		if (particleTypeStr == "PARTICLE_CPU" &&
			(std::string(szViBuffer1).empty() || std::string(szViBuffer2).empty()))
			vecErrors.push_back("VIBuffer1/2를 입력하세요. (PARTICLE_CPU 필수)");
	}

	if (!bSupported)
		vecErrors.push_back("[미구현] " + std::string(whatKindIndex == 0 ? "MESH" : "TEXTURE") + " + " + particleTypeStr + " 조합은 아직 로드할 수 없습니다.");

	bool bCanSave = vecErrors.empty();

	for (auto& err : vecErrors)
		ImGui::TextColored(ImVec4(1.f, 0.6f, 0.f, 1.f), "- %s", err.c_str());

	if (bCanSave)
	{
		if (ImGui::Button("Save Json"))
		{
			HRESULT hr = E_FAIL;
			std::filesystem::path savePath = std::filesystem::path(kJsonFolder) / jsonNameStr;
			if (savePath.extension().empty())
				savePath.replace_extension(".json");

			std::string whatKindStr = (whatKindIndex == 0) ? "MESH" : "TEXTURE";

			if (whatKindStr == "MESH") {
				hr = Save_Binary_Json(savePath.string(),
					targetPath, whatKindStr, particleTypeStr, particleNameStr,
					iMaxParticles,
					"PERMANENT_PARTICLE_VSSHADER", VSIDIName, VSEntryPoint,"PERMANENT_PARTICLE_PSSHADER", PSIDIName, PSEntryPoint,
					szGroupTag, szResTag,
					"", "",
					"", "",
					iTexRow, iTexCol,
					"", "",                                                                  // normalTexID1, normalTexID2 (메쉬 노멀맵 미지원)
					slotDistortion.selectedPath.empty() ? "" : slotDistortion.szTextureID1,
					slotDistortion.selectedPath.empty() ? "" : slotDistortion.szTextureID2,
					slotNoise.selectedPath.empty() ? "" : slotNoise.szTextureID1,
					slotNoise.selectedPath.empty() ? "" : slotNoise.szTextureID2,
					"",                                                                       // normalTexPath
					slotDistortion.selectedPath,
					slotNoise.selectedPath,
					bUseHdrForMesh ? slotPositionHdr.szTextureID1 : "",
					bUseHdrForMesh ? slotPositionHdr.szTextureID2 : "",
					bUseHdrForMesh ? slotPositionHdr.selectedPath : "",
					bUseHdrForMesh ? slotNormalHdr.szTextureID1 : "",
					bUseHdrForMesh ? slotNormalHdr.szTextureID2 : "",
					bUseHdrForMesh ? slotNormalHdr.selectedPath : "");
			}
			else {
				if (particleTypeStr == "PARTICLE_CPU") {
					hr = Save_Binary_Json(savePath.string(),
						targetPath, whatKindStr, particleTypeStr, particleNameStr,
						iMaxParticles,
						"PERMANENT_PARTICLE_VSSHADER", VSIDIName, VSEntryPoint,"PERMANENT_PARTICLE_PSSHADER", PSIDIName, PSEntryPoint,
						szGroupTag, szResTag,
						slotDiffuse.szTextureID1, slotDiffuse.szTextureID2,
						szViBuffer1, szViBuffer2, iTexRow, iTexCol,
						slotNormal.selectedPath.empty() ? "" : slotNormal.szTextureID1,
						slotNormal.selectedPath.empty() ? "" : slotNormal.szTextureID2,
						slotDistortion.selectedPath.empty() ? "" : slotDistortion.szTextureID1,
						slotDistortion.selectedPath.empty() ? "" : slotDistortion.szTextureID2,
						slotNoise.selectedPath.empty() ? "" : slotNoise.szTextureID1,
						slotNoise.selectedPath.empty() ? "" : slotNoise.szTextureID2,
						slotNormal.selectedPath,
						slotDistortion.selectedPath,
						slotNoise.selectedPath   ,  "", "", "",     // hdrTexID1, hdrTexID2, hdrTexPath
						"", "", "");    // hdrNormalTexID1, hdrNormalTexID2, hdrNormalTexPath
				}
				else if (particleTypeStr == "BEAM_CPU") {
					hr = Save_Beam_Json(savePath.string(),
						targetPath, whatKindStr, particleTypeStr, particleNameStr,
						iMaxParticles,
						"PERMANENT_PARTICLE_VSSHADER", VSIDIName, "PERMANENT_PARTICLE_PSSHADER", PSIDIName, iGeometryType,
						slotDiffuse.szTextureID1, slotDiffuse.szTextureID2,
						iTexRow, iTexCol);
				}
				else {
					hr = Save_Binary_Json(savePath.string(),
						targetPath, whatKindStr, particleTypeStr, particleNameStr,
						iMaxParticles,
						"PERMANENT_PARTICLE_VSSHADER", VSIDIName, VSEntryPoint, "PERMANENT_PARTICLE_PSSHADER", PSIDIName, PSEntryPoint,
						szGroupTag, szResTag,
						slotDiffuse.szTextureID1, slotDiffuse.szTextureID2,
						szViBuffer1, szViBuffer2, iTexRow, iTexCol,
						slotNormal.selectedPath.empty() ? "" : slotNormal.szTextureID1,
						slotNormal.selectedPath.empty() ? "" : slotNormal.szTextureID2,
						slotDistortion.selectedPath.empty() ? "" : slotDistortion.szTextureID1,
						slotDistortion.selectedPath.empty() ? "" : slotDistortion.szTextureID2,
						slotNoise.selectedPath.empty() ? "" : slotNoise.szTextureID1,
						slotNoise.selectedPath.empty() ? "" : slotNoise.szTextureID2,
						slotNormal.selectedPath,
						slotDistortion.selectedPath,
						slotNoise.selectedPath, 
						"", "", "",  
						"", "", "");    
				}
			}

			m_bLastResultSuccess = SUCCEEDED(hr);
			m_sLastResultMsg = m_bLastResultSuccess
				? ("저장 완료: " + savePath.string())
				: "저장 실패: 값을 확인하세요.";

			if (m_bLastResultSuccess)
			{
				// 저장 성공 후 슬롯 초기화
				slotDiffuse.selectedIndex = -1;
				slotDiffuse.selectedPath.clear();
				slotNormal.selectedIndex = -1;
				slotNormal.selectedPath.clear();
				slotDistortion.selectedIndex = -1;
				slotDistortion.selectedPath.clear();
				slotNoise.selectedIndex = -1;
				slotNoise.selectedPath.clear();
				slotPositionHdr.selectedIndex = -1;
				slotPositionHdr.selectedPath.clear();
				selectedHdrIndex = -1;
				slotEmpty.selectedIndex = -1;
				slotEmpty.selectedPath.clear();
				slotNormalHdr.selectedIndex = -1;
				slotNormalHdr.selectedPath.clear();
				selectedHdrNormalIndex = -1;
				m_Particles.clear();
				CGameInstance::Get().LoadParticleJson("./Resources/json/Particle/ParticleData.json");

			}
		}
	}
	else
	{
		ImGui::TextDisabled("Save Json (조건을 먼저 충족하세요)");
	}

	if (!m_sLastResultMsg.empty())
	{
		ImGui::TextColored(m_bLastResultSuccess ? ImVec4(0.3f, 1.f, 0.3f, 1.f) : ImVec4(1.f, 0.3f, 0.3f, 1.f),
			"%s", m_sLastResultMsg.c_str());
	}
	ImGui::End();


	ImGui::Begin("CParticleManager");


	if (ImGui::Button("Erase")) {
		for (auto& particle : m_Particles) {
			for (auto& real : particle.second) {
				real.second->ClearByOwner((m_iNextOwnerId-1));
			}
		}
		DeleteLoopRequests(m_iNextOwnerId -1);
	}
	static int whatKindFilterIndex = 0;
	static int groupTypeIndex = 0;
	static int typeIndex = 0;
	static SPAWN_COMMAND_KIND currentKind = SPAWN_COMMAND_KIND::STANDARD;
	static int patternKindIndex = 0;
	static PatternParamVariant pendingPattern = SStairsParam{};

	static STANDARD_PARAMS pendingStandard{};
	static BEAM_PARAMS     pendingBeam{};

	if (ImGui::RadioButton("MESH", whatKindFilterIndex == 0)) { whatKindFilterIndex = 0; typeIndex = 0; }
	ImGui::SameLine();
	if (ImGui::RadioButton("TEXTURE", whatKindFilterIndex == 1)) { whatKindFilterIndex = 1; typeIndex = 0; }

	ImGui::Separator();

	const char* groupTypeNames[] = { "PARTICLE_CPU", "PARTICLE_GPU", "BEAM_CPU", "RIBBON_CPU" };
	if (ImGui::Combo("Group (ParticleType)", &groupTypeIndex, groupTypeNames, IM_ARRAYSIZE(groupTypeNames)))
		typeIndex = 0;

	ImGui::Separator();

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
	if (bNeedTypeIndexSync)
	{
		for (int i = 0; i < (int)matchedList.size(); ++i)
		{
			if (matchedList[i].sGroupTag == pendingSyncGroup && matchedList[i].sTypeTag == pendingSyncType)
			{
				typeIndex = i;
				break;
			}
		}
		bNeedTypeIndexSync = false;
	}
	StringID selectedGroup{};
	StringID selectedType{};

	static STANDARD_PARAMS previewParams{};
	static bool bPreviewActive = false;
	static StringID previewGroup, previewType;
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

		if (selectedGroup != previewGroup || selectedType != previewType)
		{
			if (bPreviewActive)
			{
				auto pOld = GetParticle(previewGroup, previewType);
				if (pOld) pOld->ClearByOwner(PREVIEW_OWNER_ID);
			}
			previewGroup = selectedGroup;
			previewType = selectedType;
			bPreviewActive = false;
		}
	}
	else
	{
		ImGui::Text("(No particles found for this category)");
	}
	ImGui::Separator();
	ImGui::Text("=== Load Preset ===");

	static int selectedPresetIndex = -1;
	std::vector<std::string> presetNames;
	for (auto& [name, preset] : m_ParticlePresets)
		presetNames.push_back(name.GetDbgStr());

	std::vector<const char*> presetNamesForCombo;
	for (auto& name : presetNames)
		presetNamesForCombo.push_back(name.c_str());

	if (!presetNamesForCombo.empty())
	{
		selectedPresetIndex = std::clamp(selectedPresetIndex, -1, (int)presetNamesForCombo.size() - 1);

		if (ImGui::Combo("Preset List", &selectedPresetIndex, presetNamesForCombo.data(), (int)presetNamesForCombo.size()))
		{
			const auto& preset = m_ParticlePresets[presetNames[selectedPresetIndex]];

			groupTypeIndex = preset.groupTypeIndex;
			whatKindFilterIndex = preset.whatKindFilterIndex;

			previewParams.life = preset.maxLife;
			previewParams.fSize = preset.fStartSize;
			previewParams.fEndSize = preset.fEndSize;
			previewParams.color = preset.StartColor;
			previewParams.originalEmissive = preset.originalEmissive;
			previewParams.velocity = preset.velocity;
			previewParams.originalVelocity = preset.originalVelocity;
			previewParams.emissive = preset.Emissive;
			previewParams.endEmissive = preset.endEmissive;
			previewParams.rotation = _float4(XMConvertToRadians(preset.rotation.x),
				XMConvertToRadians(preset.rotation.y),
				XMConvertToRadians(preset.rotation.z),
				XMConvertToRadians(preset.rotation.w));
			bNeedTypeIndexSync = true;
			pendingSyncGroup = preset.sGroupTag;
			pendingSyncType = preset.sTypeTag;
		}
		ImGui::Separator();
		if (selectedPresetIndex >= 0 && ImGui::Button("Delete Preset"))
		{
			const std::string& targetName = presetNames[selectedPresetIndex];

			HRESULT hr = DeleteEffectPreset(szPresetSavePath, targetName);

			if (SUCCEEDED(hr))
			{
				m_ParticlePresets.erase(targetName);
				selectedPresetIndex = -1;
			}

			m_bLastResultSuccess = SUCCEEDED(hr);
			m_sLastResultMsg = m_bLastResultSuccess
				? ("프리셋 삭제 완료: " + targetName)
				: "프리셋 삭제 실패!";
		}
	}
	else
	{
		ImGui::TextDisabled("(저장된 프리셋 없음)");
	}

	ImGui::Text("=== Live Preview ===");
	ImGui::PushID("LivePreview");

	ImGui::Checkbox("Distortion", &distortion);
	ImGui::Checkbox("BILLBOARD", &billboard);
	ImGui::Checkbox("GRAVITY", &gravity);
	ImGui::Checkbox("CIRCLE_TO_WAVE", &circleToWave);
	ImGui::Checkbox("SMOKE", &bSmoke);	
	ImGui::Checkbox("SMOKEJUMP", &bSmokeJump);
	ImGui::Checkbox("SMOKEGV", &bSmokegv);
	ImGui::Checkbox("SMOKEGW", &bSmokegw);
	ImGui::Checkbox("None", &none);

	if (none)
		bSmokegw = bSmokegv = bSmokeJump = bSmoke = circleToWave = gravity = billboard = distortion = false;

	ImGui::Separator();
	ImGui::Checkbox("ALPHA_BLEND", &alphaBlend);
	ImGui::Checkbox("ALPHA_ADD", &alphaAdd);
	ImGui::Checkbox("NONE_BLEND", &noneBlend);
	auto particle = GetParticle(selectedGroup, selectedType);
	if (particle) {
		if (alphaBlend)
		{
			alphaAdd = false;
			noneBlend = false;
			if(particle->Get_BlendState() != ETOUI(BLENDTYPE::ALPHABLEND))
				particle->Set_BlendState(BLENDTYPE::ALPHABLEND);
		}
		else if (alphaAdd) {
			alphaBlend = false;
			noneBlend = false;
			if (particle->Get_BlendState() != ETOUI(BLENDTYPE::ALPHAADD))
				particle->Set_BlendState(BLENDTYPE::ALPHAADD);
		}
		else if (noneBlend) {
			alphaAdd = false;
			alphaBlend = false;
			if (particle->Get_BlendState() != ETOUI(BLENDTYPE::NONE))
				particle->Set_BlendState(BLENDTYPE::NONE);
		}
	}
	
	previewParams.iBehaviorType = CParticle::BEHAVIOR_NONE;
	if (distortion)
		previewParams.iBehaviorType |= CParticle::BEHAVIOR_DISTORTION;
	if (billboard)
		previewParams.iBehaviorType |= CParticle::BEHAVIOR_BILLBOARD;
	if (gravity)
		previewParams.iBehaviorType |= CParticle::BEHAVIOR_GRAVITY;
	if(circleToWave)
		previewParams.iBehaviorType |= CParticle::BEHAVIOR_CIRCLE_TO_WAVE;
	if(bSmoke)
		previewParams.iBehaviorType |= CParticle::BEHAVIOR_SMOKE;
	if (bSmokeJump)
		previewParams.iBehaviorType |= CParticle::BEHAVIOR_SMOKEJUMP;
	if (bSmokegv)
		previewParams.iBehaviorType |= CParticle::BEHAVIOR_SMOKEGV;
	if (bSmokegw)
		previewParams.iBehaviorType |= CParticle::BEHAVIOR_SMOKEGW;
	ImGui::Separator();

	ImGui::Checkbox("RandomPos?", &previewParams.bRandomPos);
	if (previewParams.bRandomPos) {
		ImGui::DragFloat3("PosMin", &previewParams.posMin.x, 0.01f);
		ImGui::DragFloat3("PosMax", &previewParams.posMax.x, 0.01f);
	}
	else
		ImGui::DragFloat3("Position", &previewParams.position.x, 0.01f);

	ImGui::Checkbox("RandomVelocity?", &previewParams.bRandomVel);
	if (previewParams.bRandomVel) {
		ImGui::DragFloat3("VelMin", &previewParams.velMin.x, 0.01f);
		ImGui::DragFloat3("VelMax", &previewParams.velMax.x, 0.01f);
	}
	else
		ImGui::DragFloat3("Velocity", &previewParams.velocity.x, 0.01f);

	ImGui::DragFloat("Life", &previewParams.life, 0.01f);

	ImGui::Checkbox("RandomSize?", &previewParams.bRandomSize);
	if (previewParams.bRandomSize) {
		ImGui::DragFloat3("MinStartSize", &previewParams.startSizeMin.x, 0.01f);
		ImGui::DragFloat3("MaxStartSize", &previewParams.startSizeMax.x, 0.01f);

		ImGui::Separator();
		ImGui::DragFloat3("MinEndSize", &previewParams.endSizeMin.x, 0.01f);
		ImGui::DragFloat3("MaxEndSize", &previewParams.endSizeMax.x, 0.01f);
	}
	else {
		ImGui::DragFloat3("StartSize", &previewParams.fSize.x, 0.01f);
		ImGui::DragFloat3("EndSize", &previewParams.fEndSize.x, 0.01f);
	}

	ImGui::Checkbox("RandomRotation?", &previewParams.bRandomRot);
	if (previewParams.bRandomRot) {
		ImGui::DragFloat3("RotMin", &previewParams.rotMin.x, 0.01f);
		ImGui::DragFloat3("RotMax", &previewParams.rotMax.x, 0.01f);
	}
	else
		ImGui::DragFloat4("Rotation", &previewParams.rotation.x, 0.01f);

	ImGui::ColorEdit4("Color", &previewParams.color.x);
	ImGui::ColorEdit3("Emissive", &previewParams.emissive.x);
	ImGui::DragFloat("Emissive Intensity", &previewParams.emissive.w, 0.01f);
	ImGui::ColorEdit3("endEmissive", &previewParams.endEmissive.x);
	ImGui::DragFloat("End Emissive Intensity", &previewParams.endEmissive.w, 0.01f);
	ImGui::Checkbox("Loop", &previewParams.bLoop);
	if (previewParams.bLoop)
		ImGui::DragFloat("Spawn Interval", &previewParams.fSpawnInterval, 0.01f);
	
	if (ImGui::Button("Loop Clear")) {
		DeleteLoopRequests(PREVIEW_OWNER_ID);
	}


	static STANDARD_PARAMS lastPreviewParams{};
	bool bParamsChanged = std::memcmp(&previewParams, &lastPreviewParams, sizeof(STANDARD_PARAMS)) != 0;

	bool bCheckboxToggled = ImGui::Checkbox("Preview Active", &bPreviewActive);

	if (bCheckboxToggled && !bPreviewActive)
	{
		auto pParticle = GetParticle(previewGroup, previewType);
		if (pParticle) pParticle->ClearByOwner(PREVIEW_OWNER_ID);
	}

	auto BuildPreviewSpawnData = [&](STANDARD_PARAMS& p) -> PARTICLE_SPAWN_DATA
		{
			PARTICLE_SPAWN_DATA data{};
			data.position = p.bRandomPos
				? _float3(Randf(p.posMin.x, p.posMax.x), Randf(p.posMin.y, p.posMax.y), Randf(p.posMin.z, p.posMax.z))
				: p.position;
			data.velocity = p.bRandomVel
				? _float3(Randf(p.velMin.x, p.velMax.x), Randf(p.velMin.y, p.velMax.y), Randf(p.velMin.z, p.velMax.z))
				: p.velocity;
			data.originalVelocity = data.velocity;
			data.life = p.life;
			data.fSize = p.bRandomSize 
				? _float3(Randf(p.startSizeMin.x, p.startSizeMax.x), Randf(p.startSizeMin.y, p.startSizeMax.y), Randf(p.startSizeMin.z, p.startSizeMax.z))
				: p.fSize;
			data.fEndSize = p.bRandomSize
				? _float3(Randf(p.endSizeMin.x, p.endSizeMax.x), Randf(p.endSizeMin.y, p.endSizeMax.y), Randf(p.endSizeMin.z, p.endSizeMax.z))
				: p.fEndSize;

			data.rotation = p.bRandomRot
				? _float4(XMConvertToRadians(Randf(p.rotMin.x, p.rotMax.x)),
					XMConvertToRadians(Randf(p.rotMin.y, p.rotMax.y)),
					XMConvertToRadians(Randf(p.rotMin.z, p.rotMax.z)),
					1.f)
				: _float4(XMConvertToRadians(p.rotation.x),
					XMConvertToRadians(p.rotation.y),
					XMConvertToRadians(p.rotation.z),
					XMConvertToRadians(p.rotation.w));

			data.color = p.color;
			data.emissive = p.emissive;
			data.endEmissive = p.endEmissive;
			data.originalEmissive = data.emissive;
			data.ownerID = PREVIEW_OWNER_ID;
			data.iBehaviorType = p.iBehaviorType;
			data.originalPosition = data.position;
			data.loop = p.bLoop;
			return data;
		};

	if (bPreviewActive && (bCheckboxToggled /*|| bParamsChanged*/))
	{
		auto pParticle = GetParticle(previewGroup, previewType);
		if (pParticle)
		{
			pParticle->ClearByOwner(PREVIEW_OWNER_ID);
			PARTICLE_SPAWN_DATA data = BuildPreviewSpawnData(previewParams);
			Spawn(previewGroup, previewType, 1, &data, false, 0.f);
		}
	}
	if (CGameInstance::Get().KeyDown(DIK_HOME)) {
		auto pParticle = GetParticle(previewGroup, previewType);
		if (pParticle)
		{
			pParticle->ClearByOwner(PREVIEW_OWNER_ID);
			PARTICLE_SPAWN_DATA data = BuildPreviewSpawnData(previewParams);
			Spawn(previewGroup, previewType, 1, &data, false, 0.f);
		}
	}

	static float fPreviewLoopElapsed = 0.f;
	if (bPreviewActive && previewParams.bLoop)
	{
		fPreviewLoopElapsed += ImGui::GetIO().DeltaTime;
		if (fPreviewLoopElapsed >= previewParams.fSpawnInterval)
		{
			fPreviewLoopElapsed = 0.f;
			auto pParticle = GetParticle(previewGroup, previewType);
			if (pParticle)
			{
				PARTICLE_SPAWN_DATA data = BuildPreviewSpawnData(previewParams);
				Spawn(previewGroup, previewType, 1, &data, false, 0.f);
			}
		}
	}
	else
	{
		fPreviewLoopElapsed = 0.f;
	}

	lastPreviewParams = previewParams;
	ImGui::PopID();

	ImGui::Separator();

	ImGui::Separator();
	ImGui::Text("=== Save as Preset ===");
	ImGui::InputText("Preset Name", szPresetName, IM_ARRAYSIZE(szPresetName));
	ImGui::InputText("Preset Save Path", szPresetSavePath, IM_ARRAYSIZE(szPresetSavePath));

	bool bCanSavePreset = !std::string(szPresetName).empty() && !matchedList.empty();

	if (bCanSavePreset)
	{
		if (ImGui::Button("Save as Preset"))
		{
			PARTICLE_PRESET preset{};
			preset.presetName = szPresetName;
			preset.sGroupTag = selectedGroup;
			preset.sTypeTag = selectedType;
			preset.StartColor = previewParams.color;
			preset.velocity = previewParams.velocity;
			preset.originalVelocity = preset.velocity;
			preset.originalEmissive = previewParams.originalEmissive;
			preset.Emissive = previewParams.emissive;
			preset.endEmissive = previewParams.endEmissive;
			preset.maxLife = previewParams.life;
			preset.fStartSize = previewParams.fSize;
			preset.fEndSize = previewParams.fEndSize;
			preset.rotation = previewParams.rotation;
			preset.groupTypeIndex = groupTypeIndex;
			preset.whatKindFilterIndex = whatKindFilterIndex;
			preset.iBehaviorType = previewParams.iBehaviorType;

			HRESULT hr = SaveEffectPreset(szPresetSavePath, preset);

			if (SUCCEEDED(hr))
			{
				m_ParticlePresets[preset.presetName] = preset;
			}
			m_bLastResultSuccess = SUCCEEDED(hr);
			m_sLastResultMsg = m_bLastResultSuccess
				? ("프리셋 저장 완료: " + preset.presetName)
				: "프리셋 저장 실패!";
		}
	}
	else
	{
		ImGui::TextDisabled("Save as Preset (Preset Name을 입력하고 파티클을 선택하세요)");
	}

	if (!m_sLastResultMsg.empty())
	{
		ImGui::TextColored(m_bLastResultSuccess ? ImVec4(0.3f, 1.f, 0.3f, 1.f) : ImVec4(1.f, 0.3f, 0.3f, 1.f),
			"%s", m_sLastResultMsg.c_str());
	}

	ImGui::Separator();

	{
		int kindIndex = (int)currentKind;
		const char* kindNames[] = { "Standard", "Beam", "Pattern" };
		if (ImGui::Combo("Spawn Kind", &kindIndex, kindNames, IM_ARRAYSIZE(kindNames)))
			currentKind = (SPAWN_COMMAND_KIND)kindIndex;
	}

	ImGui::Separator();
	if (currentKind == SPAWN_COMMAND_KIND::STANDARD|| currentKind == SPAWN_COMMAND_KIND::BEAM) {
		ImGui::Checkbox("Distortion", &distortion);
		ImGui::Checkbox("BILLBOARD", &billboard);
		ImGui::Checkbox("GRAVITY", &gravity);
		ImGui::Checkbox("CIRCLE_TO_WAVE", &circleToWave);
		ImGui::Checkbox("SMOKE", &bSmoke);
		ImGui::Checkbox("SMOKEJUMP", &bSmokeJump);
		ImGui::Checkbox("SMOKEGV", &bSmokegv);
		ImGui::Checkbox("SMOKEGW", &bSmokegw);
		ImGui::Checkbox("None", &none);

		if (none)
			bSmokegw = bSmokegv = bSmokeJump = bSmoke = circleToWave = gravity = billboard = distortion = false;
	

		pendingStandard.iBehaviorType = CParticle::BEHAVIOR_NONE;
		if (distortion)
			pendingStandard.iBehaviorType |= CParticle::BEHAVIOR_DISTORTION;
		if (billboard)
			pendingStandard.iBehaviorType |= CParticle::BEHAVIOR_BILLBOARD;
		if (gravity)
			pendingStandard.iBehaviorType |= CParticle::BEHAVIOR_GRAVITY;
		if (circleToWave)
			pendingStandard.iBehaviorType |= CParticle::BEHAVIOR_CIRCLE_TO_WAVE;
		if (bSmoke)
			pendingStandard.iBehaviorType |= CParticle::BEHAVIOR_SMOKE;
		if (bSmokeJump)
			pendingStandard.iBehaviorType |= CParticle::BEHAVIOR_SMOKEJUMP;
		if (bSmokegv)
			pendingStandard.iBehaviorType |= CParticle::BEHAVIOR_SMOKEGV;
		if (bSmokegw)
			pendingStandard.iBehaviorType |= CParticle::BEHAVIOR_SMOKEGW;
	}
	
	ImGui::Separator();
	if (currentKind == SPAWN_COMMAND_KIND::STANDARD)
	{
		ImGui::Text("Standard Particle Params");

		int countInput = (int)pendingStandard.count;
		ImGui::InputInt("Count", &countInput);
		pendingStandard.count = (uint32_t)std::clamp(countInput, 1, (int)MAX_SPAWN_PER_CALL);

		ImGui::Checkbox("RandomPos?", &pendingStandard.bRandomPos);
		if (pendingStandard.bRandomPos) {
			ImGui::DragFloat3("PosMin", &pendingStandard.posMin.x, 0.01f);
			ImGui::DragFloat3("PosMax", &pendingStandard.posMax.x, 0.01f);
		}
		else
			ImGui::DragFloat3("Position", &pendingStandard.position.x, 0.01f);

		ImGui::Checkbox("RandomVelocity?", &pendingStandard.bRandomVel);
		if (pendingStandard.bRandomVel) {
			ImGui::DragFloat3("VelMin", &pendingStandard.velMin.x, 0.01f);
			ImGui::DragFloat3("VelMax", &pendingStandard.velMax.x, 0.01f);
		}
		else
			ImGui::DragFloat3("Velocity", &pendingStandard.velocity.x, 0.01f);

		ImGui::DragFloat("Life", &pendingStandard.life, 0.01f);

		ImGui::Checkbox("RandomSize?", &previewParams.bRandomSize);
		if (previewParams.bRandomSize) {
			ImGui::DragFloat3("MinStartSize", &previewParams.startSizeMin.x, 0.01f);
			ImGui::DragFloat3("MaxStartSize", &previewParams.startSizeMax.x, 0.01f);

			ImGui::Separator();
			ImGui::DragFloat3("MinEndSize", &previewParams.endSizeMin.x, 0.01f);
			ImGui::DragFloat3("MaxEndSize", &previewParams.endSizeMax.x, 0.01f);
		}
		else {
			ImGui::DragFloat3("StartSize", &previewParams.fSize.x, 0.01f);
			ImGui::DragFloat3("EndSize", &previewParams.fEndSize.x, 0.01f);
		}

		ImGui::Checkbox("RandomRotation?", &pendingStandard.bRandomRot);

		if (pendingStandard.bRandomRot) {
			ImGui::DragFloat4("RotMin", &pendingStandard.rotMin.x, 0.01f);
			ImGui::DragFloat4("RotMax", &pendingStandard.rotMax.x, 0.01f);
		}
		else
			ImGui::DragFloat4("Rotation", &pendingStandard.rotation.x, 0.01f);
		

		ImGui::ColorEdit4("BaseColor", &pendingStandard.color.x);
		ImGui::ColorEdit3("Emissive Color", &pendingStandard.emissive.x);
		ImGui::DragFloat("Emissive Intensity", &pendingStandard.emissive.w, 0.01f);
		ImGui::ColorEdit3("End Emissive Color", &pendingStandard.endEmissive.x);
		ImGui::DragFloat("End Emissive Intensity", &pendingStandard.endEmissive.w, 0.01f);
		ImGui::DragFloat("SpawnDelay", &pendingStandard.fSpawnDelay, 0.01f);
		ImGui::Checkbox("Loop", &pendingStandard.bLoop);

		if (pendingStandard.bLoop)
			ImGui::DragFloat("Spawn Interval", &pendingStandard.fSpawnInterval, 0.01f);
	}
	else if (currentKind == SPAWN_COMMAND_KIND::BEAM)
	{
		ImGui::Text("Beam Params");
		ImGui::DragFloat4("Start Pos", &pendingBeam.beamStart.x, 0.01f);
		ImGui::DragFloat4("End Pos", &pendingBeam.beamEnd.x, 0.01f);
		ImGui::InputInt("DisplacementIterations", &pendingBeam.iDisplacementIterations);
		ImGui::DragFloat("DisplacementAmplitude", &pendingBeam.fDisplacementAmplitude, 0.01f);
		ImGui::DragFloat("DisplacementDamping", &pendingBeam.fDisplacementDamping, 0.01f);
		ImGui::DragFloat("flickerTimeInverval", &pendingBeam.flickerTimeInverval, 0.01f);
		ImGui::DragFloat("Duration", &pendingBeam.beamDuration, 0.01f);
		ImGui::DragFloat("SpawnDelay", &pendingBeam.fSpawnDelay, 0.01f);
		ImGui::ColorEdit4("BaseColor", &pendingBeam.color.x);
		ImGui::ColorEdit3("Emissive Color", &pendingBeam.emissive.x);
		ImGui::DragFloat("Emissive Intensity", &pendingBeam.emissive.w, 0.01f);
		ImGui::ColorEdit3("End Emissive Color", &pendingBeam.endEmissive.x);
		ImGui::DragFloat("End Emissive Intensity", &pendingBeam.endEmissive.w, 0.01f);
	}
	else if (currentKind == SPAWN_COMMAND_KIND::PATTERN)
	{
		ImGui::Text("Pattern Params");
		if (ImGui::Combo("Pattern Kind", &patternKindIndex, PATTERN_KIND_NAMES, IM_ARRAYSIZE(PATTERN_KIND_NAMES)))
			pendingPattern = MakeDefaultPatternParam(patternKindIndex);
		
		DrawImGui(pendingPattern);
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
		else if (currentKind == SPAWN_COMMAND_KIND::PATTERN) {
			cmd.params = pendingPattern;
		}

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
		else if (cmd.sGroupTag_KindTag == SPAWN_COMMAND_KIND::PATTERN)
		{
			if (std::holds_alternative<PatternParamVariant>(cmd.params))
			{
				const auto& pv = std::get<PatternParamVariant>(cmd.params);
				std::visit([&](const auto& p)
					{
						ImGui::Text("[%s/%s] PATTERN(%s)",
							cmd.sGroupTag.GetDbgStr(), cmd.sTypeTag.GetDbgStr(),
							PATTERN_KIND_NAMES[pv.index()]);
					}, pv);
			}
			else
			{
				ImGui::Text("[%s/%s] PATTERN (baked, %zu particles)",
					cmd.sGroupTag.GetDbgStr(), cmd.sTypeTag.GetDbgStr(),
					std::get<std::vector<PARTICLE_SPAWN_DATA>>(cmd.params).size());
			}
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
		uint32_t newOwnerId = ExecuteCommandQueue(m_vecCommandQueue);
	}

	static char szQueueSavePath[MAX_PATH] = "./Resources/json/Particle/SpawnQueue.json";
	ImGui::InputText("Queue Save Path", szQueueSavePath, IM_ARRAYSIZE(szQueueSavePath));

	if (ImGui::Button("Save Queue"))
	{
		HRESULT hr = SaveCommandQueue(szQueueSavePath);
		m_bLastResultSuccess = SUCCEEDED(hr);
		m_sLastResultMsg = m_bLastResultSuccess ? "Queue Save Success!" : "Queue Save Failed!";
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Queue"))
	{
		HRESULT hr = LoadCommandQueue(szQueueSavePath);
		m_bLastResultSuccess = SUCCEEDED(hr);
		m_sLastResultMsg = m_bLastResultSuccess
			? ("Queue Load Success! (" + std::to_string(m_vecCommandQueue.size()))
			: "Queue Load Failed!";
	}
	if (!m_sLastResultMsg.empty())
	{
		ImGui::TextColored(m_bLastResultSuccess ? ImVec4(0.3f, 1.f, 0.3f, 1.f) : ImVec4(1.f, 0.3f, 0.3f, 1.f),
			"%s", m_sLastResultMsg.c_str());
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
		req.iUserId = pSpawnData->ownerID;
		m_LoopRequests.push_back(std::move(req));
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
	const std::string& FullPath, const std::string& whatKind,
	const std::string& particleType, const std::string& particleName,
	int iMaxParticles,
	const std::string& VSGroup, const std::string& VSID,const std::string& VSEntryPoint,
	const std::string& PSGroup, const std::string& PSID,const std::string& PSEntryPoint,
	const std::string& sGroupTag, const std::string& sResTag,
	const std::string& textureID1, const std::string& textureID2,
	const std::string& viBufferID1, const std::string& viBufferID2,
	int RowCount, int ColCount,
	const std::string& normalTexID1, const std::string& normalTexID2,
	const std::string& distortionTexID1, const std::string& distortionTexID2,
	const std::string& noiseTexID1, const std::string& noiseTexID2,
	const std::string& normalTexPath,
	const std::string& distortionTexPath,
	const std::string& noiseTexPath,
	const std::string& hdrTexID1,
	const std::string& hdrTexID2,
	const std::string& hdrTexPath,
	const std::string& hdrNormalTexID1,   
	const std::string& hdrNormalTexID2,   
	const std::string& hdrNormalTexPath,
	const std::string& AnyTexID1,
	const std::string& AnyTexID2,
	const std::string& AnyTexPath,
	int iSelectedBlend)
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
	newEntry["VSGroup"] = VSGroup;
	newEntry["VSID"] = VSID;
	newEntry["VSEntryPoint"] = VSEntryPoint;
	newEntry["PSGroup"] = PSGroup;
	newEntry["PSID"] = PSID;
	newEntry["PSEntryPoint"] = PSEntryPoint;
	newEntry["BLENDSTATE"] = iSelectedBlend;
	newEntry["AnyTextureID1"] = AnyTexID1;
	newEntry["AnyTextureID2"] = AnyTexID2;
	newEntry["AnyTexturePath"] = AnyTexPath;

	std::string arrayKey;

	if (whatKind == "TEXTURE")
	{
		arrayKey = "textures";
		newEntry["TextureID1"] = textureID1;
		newEntry["TextureID2"] = textureID2;
		newEntry["RowCount"] = RowCount;
		newEntry["ColCount"] = ColCount;

		// ---- 추가 텍스처들: 값이 있을 때만 저장 ----
		if (!normalTexID1.empty())
		{
			newEntry["NormalTextureID1"] = normalTexID1;
			newEntry["NormalTextureID2"] = normalTexID2;
			newEntry["NormalTexturePath"] = normalTexPath;
		}
		if (!distortionTexID1.empty())
		{
			newEntry["DistortionTextureID1"] = distortionTexID1;
			newEntry["DistortionTextureID2"] = distortionTexID2;
			newEntry["DistortionTexturePath"] = distortionTexPath;   // ← 여기로 이동
		}
		if (!noiseTexID1.empty())
		{
			newEntry["NoiseTextureID1"] = noiseTexID1;
			newEntry["NoiseTextureID2"] = noiseTexID2;
			newEntry["NoiseTexturePath"] = noiseTexPath;             // ← 여기로 이동, particleType 조건 없이 항상 저장
		}

		if (particleType == "PARTICLE_CPU")
		{
			newEntry["VIBufferID1"] = viBufferID1;
			newEntry["VIBufferID2"] = viBufferID2;
		}

	}
	else if (whatKind == "MESH")
	{
		arrayKey = "models";
		newEntry["sGroupTag"] = sGroupTag;
		newEntry["sResTag"] = sResTag;
		newEntry["RowCount"] = RowCount;
		newEntry["ColCount"] = ColCount;  
		if (!hdrTexID1.empty())
		{
			newEntry["HdrPositionTextureID1"] = hdrTexID1;
			newEntry["HdrPositionTextureID2"] = hdrTexID2;
			newEntry["HdrPositionTexturePath"] = hdrTexPath;

			if (!hdrNormalTexID1.empty())
			{
				newEntry["HdrNormalTextureID1"] = hdrNormalTexID1;
				newEntry["HdrNormalTextureID2"] = hdrNormalTexID2;
				newEntry["HdrNormalTexturePath"] = hdrNormalTexPath;
			}
		}
		if (!distortionTexID1.empty())                              // <-- 새로 추가
		{
			newEntry["DistortionTextureID1"] = distortionTexID1;
			newEntry["DistortionTextureID2"] = distortionTexID2;
			newEntry["DistortionTexturePath"] = distortionTexPath;
		}
		if (!noiseTexID1.empty())
		{
			newEntry["NoiseTextureID1"] = noiseTexID1;
			newEntry["NoiseTextureID2"] = noiseTexID2;
			newEntry["NoiseTexturePath"] = noiseTexPath;            
		}
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

HRESULT CParticleManager::ClearLoopRequests()
{
	m_LoopRequests.clear();
	return S_OK;
}

HRESULT CParticleManager::DeleteLoopRequests(uint32_t userId)
{
	auto iter = m_LoopRequests.begin();
	for (iter; iter != m_LoopRequests.end();) {
		if ((*iter).iUserId == userId) {
			iter = m_LoopRequests.erase(iter);
		}
		else {
			iter++;
		}
	}
	return S_OK;
}

void CParticleManager::ComboList(_string comboName, _string resourceName, _string& previewName)
{
	auto shaders = CGameInstance::Get().GetResource(resourceName);

	if (ImGui::BeginCombo(comboName.c_str(), previewName.c_str()))
	{
		for (auto [key, value] : shaders)
		{
			_bool bSelect = previewName == key.str;
			ImGui::PushID(key.str);
			if (ImGui::Selectable(key.str, &bSelect))
			{
				previewName = key.str;
			}
			if (bSelect)
				ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}
}

UPtr<CParticleManager> CParticleManager::Create()
{
	return UPtr<CParticleManager>(new CParticleManager{});
}

uint32_t CParticleManager::ExecuteCommandQueue(std::vector<SPAWN_COMMAND>& queue)
{
	if (queue.empty())
		return INVALID_PARTICLE_OWNER_ID;

	const  uint32_t ownerId = m_iNextOwnerId++; // 호출할 때마다 무조건 새 ID 발급

	std::map<std::pair<StringID, StringID>, std::vector<PARTICLE_SPAWN_DATA>> batched;
	std::map<std::pair<StringID, StringID>, float> loopIntervals;

	for (auto& cmd : queue)
	{
		cmd.ownerId = ownerId; // 여기서 무조건 덮어씀, 호출자가 뭘 넘겼든 무시

		if (cmd.sGroupTag_KindTag == SPAWN_COMMAND_KIND::STANDARD)
		{
			const auto& p = std::get<STANDARD_PARAMS>(cmd.params);
			auto& vec = batched[{cmd.sGroupTag, cmd.sTypeTag}];

		/*	if (p.bLoop)
				loopIntervals[{cmd.sGroupTag, cmd.sTypeTag}] = p.fSpawnInterval;*/

			for (uint32_t i = 0; i < p.count; ++i)
			{
				PARTICLE_SPAWN_DATA s{};
				s.position = p.bRandomPos
					? _float3(Randf(p.posMin.x, p.posMax.x), Randf(p.posMin.y, p.posMax.y), Randf(p.posMin.z, p.posMax.z))
					: p.position;
				s.velocity = p.bRandomVel
					? _float3(Randf(p.velMin.x, p.velMax.x), Randf(p.velMin.y, p.velMax.y), Randf(p.velMin.z, p.velMax.z))
					: p.velocity;
				s.originalVelocity = s.velocity;
				s.fSize = p.bRandomSize
					? _float3(Randf(p.startSizeMin.x, p.startSizeMax.x), Randf(p.startSizeMin.y, p.startSizeMax.y), Randf(p.startSizeMin.z, p.startSizeMax.z))
					: p.fSize;
				s.fEndSize = p.bRandomSize
					? _float3(Randf(p.endSizeMin.x, p.endSizeMax.x), Randf(p.endSizeMin.y, p.endSizeMax.y), Randf(p.endSizeMin.z, p.endSizeMax.z))
					: p.fEndSize;
				s.life = p.life;
			
				s.rotation = p.bRandomRot
					? _float4(XMConvertToRadians(Randf(p.rotMin.x, p.rotMax.x)),
						XMConvertToRadians(Randf(p.rotMin.y, p.rotMax.y)),
						XMConvertToRadians(Randf(p.rotMin.z, p.rotMax.z)),
						1.f)
					: _float4(XMConvertToRadians(p.rotation.x),
						XMConvertToRadians(p.rotation.y),
						XMConvertToRadians(p.rotation.z),
						XMConvertToRadians(p.rotation.w));

				s.color = p.color;
				s.emissive = p.emissive;
				s.endEmissive = p.endEmissive;
				s.originalEmissive = s.emissive;
				s.iBehaviorType = p.iBehaviorType;
				s.ownerID = ownerId;
				s.originalPosition = s.position;
				s.loop = p.bLoop;
				s.spawnDelay = p.fSpawnDelay;
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
		}
		else if (cmd.sGroupTag_KindTag == SPAWN_COMMAND_KIND::PATTERN)
		{
			auto& vec = batched[{cmd.sGroupTag, cmd.sTypeTag}];

			if (std::holds_alternative<PatternParamVariant>(cmd.params))
			{
				const auto& pv = std::get<PatternParamVariant>(cmd.params);
				auto spawnList = BuildSpawnData(pv);
				for (auto& s : spawnList)
					s.ownerID = ownerId;
				vec.insert(vec.end(), spawnList.begin(), spawnList.end());
			}
			else if (std::holds_alternative<std::vector<PARTICLE_SPAWN_DATA>>(cmd.params))
			{
				auto& spawnList = std::get<std::vector<PARTICLE_SPAWN_DATA>>(cmd.params);
				for (auto& s : spawnList)
					s.ownerID = ownerId;
				vec.insert(vec.end(), spawnList.begin(), spawnList.end());
			}
		}
	}

	for (auto& [key, spawnList] : batched)
	{
		Spawn(key.first, key.second, (uint32_t)spawnList.size(), spawnList.data(), false, 0.f);
		// bIsLoop, fInterval 둘 다 false/0 고정 — m_LoopRequests 경로는 완전히 안 씀
	}

	return ownerId; // ---- 호출자에게 발급된 오너 ID를 돌려줌 ----
}
HRESULT CParticleManager::Save_Beam_Json(std::string outpath, const std::string& FullPath, const std::string& whatKind, 
	const std::string& particleType, const std::string& particleName, int iMaxParticles, const std::string& VSGroup, const std::string& VSID,
	const std::string& PSGroup, const std::string& PSID, int geometryType, const std::string& textureID1, const std::string& textureID2, int RowCount, int ColCount)
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
		newEntry["GeometryType"] = geometryType;
	}
	//else if (whatKind == "MESH")
	//{
	//	arrayKey = "models";
	//	newEntry["sGroupTag"] = sGroupTag;
	//	newEntry["sResTag"] = sResTag;
	//}
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
HRESULT CParticleManager::LoadParticleJson(const std::string& strJsonPath)
{
	// LoadParticleJson 안, "textures" 배열 for문 시작 직전에 추가
	auto LoadAuxTexture = [](const nlohmann::json& entry,
		const char* pathKey, const char* id1Key, const char* id2Key,
		std::pair<StringID, StringID>& outID) -> void
		{
			if (!entry.contains(pathKey))
				return;

			std::string path = entry[pathKey].get<std::string>();
			std::string id1 = entry.value(id1Key, "");
			std::string id2 = entry.value(id2Key, "");

			if (path.empty() || id1.empty() || id2.empty())
				return;

			if (auto res = CGameInstance::Get().AddResourceT<E::CResTexture2D>(
				id1, id2, E::CResTexture2D::Create(path)))
			{
				res->Load();
			}
			outID = { id1, id2 };
		};

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
			std::string VSGroup = entry.value("VSGroup", "");
			std::string VSID = entry.value("VSID", "");
			std::string VSEntryPoint = entry.value("VSEntryPoint", "");
			std::string PSGroup = entry.value("PSGroup", "");
			std::string PSID = entry.value("PSID", "");
			std::string PSEntryPoint = entry.value("PSEntryPoint", "");
			int RowCount = entry.value("RowCount", 1);
			int ColCount = entry.value("ColCount", 1);
			int selectedBlend = entry.value("BLENDSTATE", 0);

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
				desc.whatKind = MESHORTEXTURE::MESH;
				desc.sGroupTag = sGroupTag;
				desc.sResTag = sResTag;
				desc.TexRows = RowCount;      
				desc.TexColumns = ColCount;
				desc.sVEntryPoint = VSEntryPoint;
				desc.sPEntryPoint = PSEntryPoint;
				LoadAuxTexture(entry, "DistortionTexturePath", "DistortionTextureID1", "DistortionTextureID2", desc.distortionTextureID);
				LoadAuxTexture(entry, "NoiseTexturePath", "NoiseTextureID1", "NoiseTextureID2", desc.noiseTextureID);
				LoadAuxTexture(entry, "HdrPositionTexturePath", "HdrPositionTextureID1", "HdrPositionTextureID2", desc.hdrPositionTextureID);
				LoadAuxTexture(entry, "HdrNormalTexturePath", "HdrNormalTextureID1", "HdrNormalTextureID2", desc.hdrNormalTextureID);
				LoadAuxTexture(entry, "AnyTexturePath", "AnyTextureID1", "AnyTextureID2", desc.anyTextureID);
				desc.blendState = selectedBlend;
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
				desc.TexRows = RowCount;     
				desc.TexColumns = ColCount;
				if (!VSGroup.empty() && !VSID.empty())
					desc.VSID = { VSGroup, VSID };

				if (!PSGroup.empty() && !PSID.empty())
					desc.PSID = { PSGroup, PSID };
				desc.sVEntryPoint = VSEntryPoint;
				desc.sPEntryPoint = PSEntryPoint;
				LoadAuxTexture(entry, "DistortionTexturePath", "DistortionTextureID1", "DistortionTextureID2", desc.distortionTextureID);
				LoadAuxTexture(entry, "NoiseTexturePath", "NoiseTextureID1", "NoiseTextureID2", desc.noiseTextureID);
				LoadAuxTexture(entry, "HdrPositionTexturePath", "HdrPositionTextureID1", "HdrPositionTextureID2", desc.hdrPositionTextureID);
				LoadAuxTexture(entry, "HdrNormalTexturePath", "HdrNormalTextureID1", "HdrNormalTextureID2", desc.hdrNormalTextureID);
				LoadAuxTexture(entry, "AnyTexturePath", "AnyTextureID1", "AnyTextureID2", desc.anyTextureID);

				desc.blendState = selectedBlend;

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
			std::string VSGroup = entry.value("VSGroup", "");
			std::string VSID = entry.value("VSID", "");
			std::string VSEntryPoint = entry.value("VSEntryPoint", "");
			std::string PSGroup = entry.value("PSGroup", "");
			std::string PSID = entry.value("PSID", "");
			std::string PSEntryPoint = entry.value("PSEntryPoint", "");
			std::string textureID1 = entry.value("TextureID1", "");
			std::string textureID2 = entry.value("TextureID2", "");
			int RowCount = entry.value("RowCount", 1);
			int ColCount = entry.value("ColCount", 1);
			int selectedBlend = entry.value("BLENDSTATE", 1);

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
				desc.whatKind = MESHORTEXTURE::TEX;
				desc.textureID = { textureID1, textureID2 };
				desc.TexRows = RowCount;
				desc.TexColumns = ColCount;
				if (!VSGroup.empty() && !VSID.empty()) desc.VSID = { VSGroup, VSID };
				if (!PSGroup.empty() && !PSID.empty()) desc.PSID = { PSGroup, PSID };
				desc.sVEntryPoint = VSEntryPoint;
				desc.sPEntryPoint = PSEntryPoint;
				// ---- 추가 텍스처 로드 ----
				LoadAuxTexture(entry, "NormalTexturePath", "NormalTextureID1", "NormalTextureID2", desc.normalTextureID);
				LoadAuxTexture(entry, "DistortionTexturePath", "DistortionTextureID1", "DistortionTextureID2", desc.distortionTextureID);
				LoadAuxTexture(entry, "NoiseTexturePath", "NoiseTextureID1", "NoiseTextureID2", desc.noiseTextureID);
				LoadAuxTexture(entry, "AnyTexturePath", "AnyTextureID1", "AnyTextureID2", desc.anyTextureID);

				desc.blendState = selectedBlend;

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
				if (!VSGroup.empty() && !VSID.empty()) desc.VSID = { VSGroup, VSID };
				if (!PSGroup.empty() && !PSID.empty()) desc.PSID = { PSGroup, PSID };
				desc.TexRows = RowCount;
				desc.TexColumns = ColCount;
				desc.sVEntryPoint = VSEntryPoint;
				desc.sPEntryPoint = PSEntryPoint;
				// ---- 추가 텍스처 로드 ----
				LoadAuxTexture(entry, "NormalTexturePath", "NormalTextureID1", "NormalTextureID2", desc.normalTextureID);
				LoadAuxTexture(entry, "DistortionTexturePath", "DistortionTextureID1", "DistortionTextureID2", desc.distortionTextureID);
				LoadAuxTexture(entry, "NoiseTexturePath", "NoiseTextureID1", "NoiseTextureID2", desc.noiseTextureID);
				LoadAuxTexture(entry, "AnyTexturePath", "AnyTextureID1", "AnyTextureID2", desc.anyTextureID);

				desc.blendState = selectedBlend;

				particle = CParticle_CPU::Create(&desc);
			}
			else if (particleType == "BEAM_CPU")
			{
				int geometryType = entry.value("GeometryType", 0);

				CBeam_CPU::DESC desc;
				desc.geometryType = geometryType;
				desc.textureID = { textureID1, textureID2 };
				if (!VSGroup.empty() && !VSID.empty()) desc.VSID = { VSGroup, VSID };
				if (!PSGroup.empty() && !PSID.empty()) desc.PSID = { PSGroup, PSID };

				// ---- 추가 텍스처 로드 ----
				LoadAuxTexture(entry, "NormalTexturePath", "NormalTextureID1", "NormalTextureID2", desc.normalTextureID);
				LoadAuxTexture(entry, "DistortionTexturePath", "DistortionTextureID1", "DistortionTextureID2", desc.distortionTextureID);
				LoadAuxTexture(entry, "NoiseTexturePath", "NoiseTextureID1", "NoiseTextureID2", desc.noiseTextureID);
				LoadAuxTexture(entry, "AnyTexturePath", "AnyTextureID1", "AnyTextureID2", desc.anyTextureID);

				desc.sVEntryPoint = VSEntryPoint;
				desc.sPEntryPoint = PSEntryPoint;
				desc.blendState = selectedBlend;

				particle = CBeam_CPU::Create(&desc);
			}
			else if (particleType == "TRAIL_CPU")
			{
				CTrail_CPU::DESC desc;
				desc.textureID = { textureID1, textureID2 };
				if (!VSGroup.empty() && !VSID.empty()) desc.VSID = { VSGroup, VSID };
				if (!PSGroup.empty() && !PSID.empty()) desc.PSID = { PSGroup, PSID };

				// ---- 추가 텍스처 로드 ----
				LoadAuxTexture(entry, "NormalTexturePath", "NormalTextureID1", "NormalTextureID2", desc.normalTextureID);
				LoadAuxTexture(entry, "DistortionTexturePath", "DistortionTextureID1", "DistortionTextureID2", desc.distortionTextureID);
				LoadAuxTexture(entry, "NoiseTexturePath", "NoiseTextureID1", "NoiseTextureID2", desc.noiseTextureID);
				LoadAuxTexture(entry, "AnyTexturePath", "AnyTextureID1", "AnyTextureID2", desc.anyTextureID);

				desc.sVEntryPoint = VSEntryPoint;
				desc.sPEntryPoint = PSEntryPoint;
				desc.blendState = selectedBlend;

				particle = CTrail_CPU::Create(&desc);
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
			_stricmp(ext.c_str(), ".tga") == 0 ||
			_stricmp(ext.c_str(), ".hdr") == 0)

		{
			result.push_back(entry.path().filename().string());
		}
	}

	return result;
}



ID3D11ShaderResourceView* CParticleManager::GetOrLoadTextureThumbnail(const std::string& fullPath)
{
	auto it = m_TextureThumbnailCache.find(fullPath);
	if (it != m_TextureThumbnailCache.end())
		return it->second.Get();

	ComPtr<ID3D11ShaderResourceView> pSRV;
	HRESULT hr = DirectX::CreateWICTextureFromFile(
		CGameInstance::Get().GetGraphicDevice().Get(),
		CGameInstance::Get().GetGraphicDeviceContext().Get(),
		std::wstring(fullPath.begin(), fullPath.end()).c_str(),
		nullptr, pSRV.GetAddressOf());

	if (FAILED(hr) || !pSRV)
		return nullptr;

	m_TextureThumbnailCache[fullPath] = pSRV;
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
				entry["StartSize"] = { p.fSize.x, p.fSize.y, p.fSize.z }; 
				entry["EndSize"] = { p.fEndSize.x, p.fEndSize.y, p.fEndSize.z };
				entry["bRandomRot"] = p.bRandomRot;
				entry["rotMin"] = { p.rotMin.x, p.rotMin.y, p.rotMin.z };
				entry["rotMax"] = { p.rotMax.x, p.rotMax.y, p.rotMax.z };
				entry["Rotation"] = { p.rotation.x, p.rotation.y, p.rotation.z, p.rotation.w};
				entry["color"] = { p.color.x, p.color.y, p.color.z, p.color.w };
				entry["emissive"] = { p.emissive.x, p.emissive.y, p.emissive.z, p.emissive.w };
				entry["endEmissive"] = { p.endEmissive.x, p.endEmissive.y, p.endEmissive.z, p.endEmissive.w };
				entry["fSpawnDelay"] = p.fSpawnDelay;
				entry["bLoop"] = p.bLoop;
				entry["fSpawnInterval"] = p.fSpawnInterval;
				entry["bRandomPos"] = p.bRandomPos;
				entry["posMin"] = { p.posMin.x, p.posMin.y, p.posMin.z };
				entry["posMax"] = { p.posMax.x, p.posMax.y, p.posMax.z };
				entry["bRandomVel"] = p.bRandomVel;
				entry["velMin"] = { p.velMin.x, p.velMin.y, p.velMin.z };
				entry["velMax"] = { p.velMax.x, p.velMax.y, p.velMax.z };
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
				entry["endEmissive"] = { p.endEmissive.x, p.endEmissive.y, p.endEmissive.z, p.endEmissive.w };
				entry["GeometryType"] = p.geometryType;

				break;
			}
			case SPAWN_COMMAND_KIND::PATTERN:
			{
				if (std::holds_alternative<PatternParamVariant>(cmd.params))
				{
					const auto& pv = std::get<PatternParamVariant>(cmd.params);
					entry["patternKindIndex"] = (int)pv.index();
					nlohmann::json paramJson;
					SaveParam(pv, paramJson);
					entry["patternParams"] = paramJson;
				}
				else
				{
					// baked 데이터는 패턴 정보가 없으므로 저장 불가 - 스킵
					continue;
				}
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

	//m_vecCommandQueue.clear();

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


			p.bRandomPos = entry.value("bRandomPos", false);
			auto posMin = entry.value("posMin", std::vector<float>{0, 0, 0});
			p.posMin = { posMin[0], posMin[1], posMin[2] };
			auto posMax = entry.value("posMax", std::vector<float>{0, 0, 0});
			p.posMax = { posMax[0], posMax[1], posMax[2] };

			auto pos = entry.value("position", std::vector<float>{0, 0, 0});
			p.position = { pos[0], pos[1], pos[2] };

			p.bRandomVel = entry.value("bRandomVel", false);
			auto velMin = entry.value("velMin", std::vector<float>{0, 0, 0});
			p.velMin = { velMin[0], velMin[1], velMin[2] };
			auto velMax = entry.value("velMax", std::vector<float>{0, 0, 0});
			p.velMax = { velMax[0], velMax[1], velMax[2] };

			auto vel = entry.value("velocity", std::vector<float>{0, 0, 0});
			p.velocity = { vel[0], vel[1], vel[2] };


			p.life = entry.value("life", 1.f);

			auto startSize = entry.value("StartSize", std::vector<float>{0, 0, 0});
			p.fSize = { startSize[0], startSize[1], startSize[2] };
			auto endSize = entry.value("StartSize", std::vector<float>{0, 0, 0});
			p.fEndSize = { endSize[0], endSize[1], endSize[2] };

			p.bRandomRot = entry.value("bRandomRot", false);
			auto rotMin = entry.value("rotMin", std::vector<float>{0, 0, 0});
			p.rotMin = { rotMin[0], rotMin[1], rotMin[2] };
			auto rotMax = entry.value("rotMax", std::vector<float>{0, 0, 0});
			p.rotMax = { rotMax[0], rotMax[1], rotMax[2] };

			auto rot = entry.value("Rotation", std::vector<float>{0, 0, 0,0});
			p.rotation = { rot[0], rot[1], rot[2],rot[3]};


			auto col = entry.value("color", std::vector<float>{1, 1, 1, 1});
			p.color = { col[0], col[1], col[2], col[3] };

			auto emi = entry.value("emissive", std::vector<float>{0, 0, 0, 0});
			p.emissive = { emi[0], emi[1], emi[2], emi[3] };
			auto endEmi = entry.value("endEmissive", std::vector<float>{0, 0, 0, 0});
			p.endEmissive = { endEmi[0], endEmi[1], endEmi[2], endEmi[3] };

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
			p.geometryType = entry.value("GeometryType", 0.f);

			auto col = entry.value("color", std::vector<float>{1, 1, 1, 1});
			p.color = { col[0], col[1], col[2], col[3] };

			auto emi = entry.value("emissive", std::vector<float>{0, 0, 0, 0});
			p.emissive = { emi[0], emi[1], emi[2], emi[3] };
			auto endEmi = entry.value("endEmissive", std::vector<float>{0, 0, 0, 0});
			p.endEmissive = { endEmi[0], endEmi[1], endEmi[2], endEmi[3] };

			cmd.params = p;
			break;
		}
		case SPAWN_COMMAND_KIND::PATTERN:
		{
			int kindIdx = entry.value("patternKindIndex", 0);
			PatternParamVariant pv = MakeDefaultPatternParam(kindIdx);
			const auto& paramJson = entry["patternParams"];

			std::visit([&](auto& p) { LoadParam(p, paramJson); }, pv);

			cmd.params = pv;
			break;
		}
		default:
			continue;
		}

		m_vecCommandQueue.push_back(cmd);
	}

	return S_OK;
}



//////////////////

HRESULT CParticleManager::SaveEffectPreset(const std::string& strJsonPath, const PARTICLE_PRESET& preset)
{
	nlohmann::json j;

	if (std::filesystem::exists(strJsonPath))
	{
		std::ifstream inFile(strJsonPath);
		if (inFile.is_open())
		{
			try { inFile >> j; }
			catch (...) { j = nlohmann::json{}; }
		}
	}

	if (!j.contains("presets") || !j["presets"].is_array())
		j["presets"] = nlohmann::json::array();

	nlohmann::json entry;
	entry["presetName"] = preset.presetName;
	entry["sGroupTag"] = preset.sGroupTag.GetDbgStr();
	entry["sTypeTag"] = preset.sTypeTag.GetDbgStr();
	entry["defaultColor"] = { preset.StartColor.x, preset.StartColor.y, preset.StartColor.z, preset.StartColor.w };
	entry["defaultEmissive"] = { preset.Emissive.x, preset.Emissive.y, preset.Emissive.z, preset.Emissive.w };
	entry["defaultEndEmissive"] = { preset.endEmissive.x, preset.endEmissive.y, preset.endEmissive.z, preset.endEmissive.w };
	entry["defaultLife"] = preset.maxLife;
	entry["defaultSize"] = { preset.fStartSize.x, preset.fStartSize.y, preset.fStartSize.z}; 
	entry["defaultEndSize"] = { preset.fEndSize.x, preset.fEndSize.y, preset.fEndSize.z }; 
	entry["groupTypeIndex"] = preset.groupTypeIndex;
	entry["whatKindFilterIndex"] = preset.whatKindFilterIndex;
	entry["behaviorType"] = preset.iBehaviorType;
	entry["rotation"] = { preset.rotation.x, preset.rotation.y, preset.rotation.z, preset.rotation.w };
	// 같은 이름 있으면 덮어쓰기
	bool bReplaced = false;
	for (auto& e : j["presets"])
	{
		if (e.value("presetName", "") == preset.presetName)
		{
			e = entry;
			bReplaced = true;
			break;
		}
	}
	if (!bReplaced)
		j["presets"].push_back(entry);

	std::filesystem::path savePath(strJsonPath);
	if (!savePath.parent_path().empty())
		std::filesystem::create_directories(savePath.parent_path());

	std::ofstream file(savePath);
	if (!file.is_open())
		return E_FAIL;

	file << j.dump(4);
	return S_OK;
}

HRESULT CParticleManager::LoadParticlePresets(const std::string& strJsonPath)
{
	if (!std::filesystem::exists(strJsonPath))
		return E_FAIL;

	std::ifstream file(strJsonPath);
	if (!file.is_open())
		return E_FAIL;

	nlohmann::json j;
	try { file >> j; }
	catch (...) { return E_FAIL; }

	if (!j.contains("presets") || !j["presets"].is_array())
		return E_FAIL;

	for (const auto& entry : j["presets"])
	{
		PARTICLE_PRESET preset{};
		preset.presetName = entry.value("presetName", "");
		preset.sGroupTag = entry.value("sGroupTag", "");
		preset.sTypeTag = entry.value("sTypeTag", "");



		auto col = entry.value("defaultColor", std::vector<float>{1, 1, 1, 1});
		preset.StartColor = { col[0], col[1], col[2], col[3] };

		auto emi = entry.value("defaultEmissive", std::vector<float>{0, 0, 0, 0});
		preset.Emissive = { emi[0], emi[1], emi[2], emi[3] };
		auto endEmi = entry.value("defaultEndEmissive", std::vector<float>{0, 0, 0, 0});
		preset.endEmissive = { endEmi[0], endEmi[1], endEmi[2], endEmi[3] };

		preset.maxLife = entry.value("defaultLife", 1.f);
		auto startSize = entry.value("defaultSize", std::vector<float>{0, 0, 0, 0});
		preset.fStartSize = { startSize[0], startSize[1], startSize[2]};

		auto endSize = entry.value("defaultEndSize", std::vector<float>{0, 0, 0, 0});
		preset.fEndSize = { endSize[0], endSize[1], endSize[2] };

		auto rot = entry.value("rotation", std::vector<float>{0, 0, 0, 0});
		preset.rotation = { rot[0], rot[1], rot[2], rot[3] };
		preset.groupTypeIndex = entry.value("groupTypeIndex", 0);
		preset.whatKindFilterIndex = entry.value("whatKindFilterIndex", 0);
		preset.iBehaviorType = entry.value("behaviorType", 0);
		if (!preset.presetName.empty())
			m_ParticlePresets[preset.presetName] = preset;
	}

	return S_OK;
}



// 3) 예전 호출부(strJsonPath로 바로 부르던 곳) 호환용 오버로드 - 필요하면 유지
uint32_t CParticleManager::Spawn(const std::string& strJsonPath,
	const _float4x4& worldMat, _fvector endPos)
{
	auto found = m_ParsedCommandCache.find(strJsonPath);
	if (found == m_ParsedCommandCache.end())
		found = m_ParsedCommandCache.emplace(strJsonPath, Parse_Command(strJsonPath)).first;

	return Spawn(found->second, worldMat, endPos);
}
//uint32_t CParticleManager::Spawn( const std::string& strJsonPath, const _float4x4& worldMat, _fvector endPos)
//{
//	std::string path = "./Resources/json/Particle/";
//	path += strJsonPath;
//	if (!std::filesystem::exists(path))
//		return E_FAIL;
//	std::ifstream file(path);
//	if (!file.is_open())
//		return E_FAIL;
//	nlohmann::json j;
//	try { file >> j; }
//	catch (...) { return E_FAIL; }
//	if (!j.contains("commands") || !j["commands"].is_array())
//		return E_FAIL;
//
//	XMMATRIX matWorld = XMLoadFloat4x4(&worldMat);   // 여기서 한 번만 로드
//
//	std::vector<SPAWN_COMMAND> localQueue;
//	for (const auto& entry : j["commands"])
//	{
//		SPAWN_COMMAND cmd{};
//		cmd.sGroupTag_KindTag = (SPAWN_COMMAND_KIND)entry.value("kind", 0);
//		cmd.sGroupTag = entry.value("sGroupTag", "");
//		cmd.sTypeTag = entry.value("sTypeTag", "");
//
//		switch (cmd.sGroupTag_KindTag)
//		{
//		case SPAWN_COMMAND_KIND::STANDARD:
//		{
//			STANDARD_PARAMS p{};
//			p.count = entry.value("count", 1u);
//			p.bRandomPos = entry.value("bRandomPos", false);
//			auto posMin = entry.value("posMin", std::vector<float>{0, 0, 0});
//			auto posMax = entry.value("posMax", std::vector<float>{0, 0, 0});
//			auto pos = entry.value("position", std::vector<float>{0, 0, 0});
//
//			// 위치: 로컬 오프셋을s matWorld로 통째로 변환 (회전 + 이동)
//			// 회전이 걸려있으면 min/max 두 점을 각각 변환해도 축별 대소관계가
//			// 뒤집힐 수 있으므로, 변환 후 컴포넌트별로 다시 min/max를 재정렬한다.
//			_float3 posMinT, posMaxT;
//			XMStoreFloat3(&posMinT, XMVector3TransformCoord(
//				XMVectorSet(posMin[0], posMin[1], posMin[2], 1.f), matWorld));
//			XMStoreFloat3(&posMaxT, XMVector3TransformCoord(
//				XMVectorSet(posMax[0], posMax[1], posMax[2], 1.f), matWorld));
//			p.posMin = { std::min(posMinT.x, posMaxT.x), std::min(posMinT.y, posMaxT.y), std::min(posMinT.z, posMaxT.z) };
//			p.posMax = { std::max(posMinT.x, posMaxT.x), std::max(posMinT.y, posMaxT.y), std::max(posMinT.z, posMaxT.z) };
//
//			XMStoreFloat3(&p.position, XMVector3TransformCoord(
//				XMVectorSet(pos[0], pos[1], pos[2], 1.f), matWorld));
//
//			p.bRandomVel = entry.value("bRandomVel", false);
//			auto velMin = entry.value("velMin", std::vector<float>{0, 0, 0});
//			auto velMax = entry.value("velMax", std::vector<float>{0, 0, 0});
//			auto vel = entry.value("velocity", std::vector<float>{0, 0, 0});
//
//			// 속도: 방향값이니까 회전만 적용, 이동 성분은 무시 (TransformNormal, w=0)
//			// 위치와 마찬가지로 회전 때문에 축별 대소관계가 뒤집힐 수 있어 재정렬한다.
//			_float3 velMinT, velMaxT;
//			XMStoreFloat3(&velMinT, XMVector3TransformNormal(
//				XMVectorSet(velMin[0], velMin[1], velMin[2], 0.f), matWorld));
//			XMStoreFloat3(&velMaxT, XMVector3TransformNormal(
//				XMVectorSet(velMax[0], velMax[1], velMax[2], 0.f), matWorld));
//			p.velMin = { std::min(velMinT.x, velMaxT.x), std::min(velMinT.y, velMaxT.y), std::min(velMinT.z, velMaxT.z) };
//			p.velMax = { std::max(velMinT.x, velMaxT.x), std::max(velMinT.y, velMaxT.y), std::max(velMinT.z, velMaxT.z) };
//
//			XMStoreFloat3(&p.velocity, XMVector3TransformNormal(
//				XMVectorSet(vel[0], vel[1], vel[2], 0.f), matWorld));
//
//			p.life = entry.value("life", 1.f);
//			p.fSize = entry.value("StartSize", 1.f);
//			p.fEndSize = entry.value("EndSize", 1.f);
//
//			p.bRandomRot = entry.value("bRandomRot", false);
//			// 회전 min/max는 좌표가 아니라 오일러 각도(도) 범위이므로 matWorld로
//			// 변환하면 안 된다. 캐스터 회전까지 합성하려면 나중에 쿼터니언으로 별도 처리.
//			auto rotMin = entry.value("rotMin", std::vector<float>{0, 0, 0});
//			p.rotMin = { rotMin[0], rotMin[1], rotMin[2] };
//			auto rotMax = entry.value("rotMax", std::vector<float>{0, 0, 0});
//			p.rotMax = { rotMax[0], rotMax[1], rotMax[2] };
//			auto rot = entry.value("Rotation", std::vector<float>{0, 0, 0, 0});
//			p.rotation = { rot[0], rot[1], rot[2], rot[3] };
//			// TODO: 캐스터 회전까지 합성하려면 matWorld에서 쿼터니언 뽑아서
//			// rotation과 XMQuaternionMultiply 필요 (지금은 위치/속도만 처리)
//
//			auto col = entry.value("color", std::vector<float>{1, 1, 1, 1});
//			p.color = { col[0], col[1], col[2], col[3] };
//			auto emi = entry.value("emissive", std::vector<float>{0, 0, 0, 0});
//			p.emissive = { emi[0], emi[1], emi[2], emi[3] };
//			auto endEmi = entry.value("endEmissive", std::vector<float>{0, 0, 0, 0});
//			p.endEmissive = { endEmi[0], endEmi[1], endEmi[2], endEmi[3] };
//			p.fSpawnDelay = entry.value("fSpawnDelay", 0.f);
//			p.bLoop = entry.value("bLoop", false);
//			p.fSpawnInterval = entry.value("fSpawnInterval", 0.f);
//			cmd.params = p;
//			break;
//		}
//		case SPAWN_COMMAND_KIND::BEAM:
//		{
//			BEAM_PARAMS p{};
//			auto bs = entry.value("beamStart", std::vector<float>{0, 0, 0, 0});
//			p.beamStart = { bs[0], bs[1], bs[2], bs[3] };
//			XMStoreFloat4(&p.beamStart, XMVector3TransformCoord(
//				XMLoadFloat4(&p.beamStart), matWorld));          // 캐스터 기준: 회전 + 이동
//
//			auto be = entry.value("beamEnd", std::vector<float>{0, 0, 0, 0});
//			p.beamEnd = { be[0], be[1], be[2], be[3] };
//			XMStoreFloat4(&p.beamEnd, XMLoadFloat4(&p.beamEnd) + endPos);   // 타겟 기준: 순수 이동만 (원래대로)
//
//			p.iDisplacementIterations = entry.value("iDisplacementIterations", 0);
//			p.fDisplacementAmplitude = entry.value("fDisplacementAmplitude", 0.f);
//			p.fDisplacementDamping = entry.value("fDisplacementDamping", 0.f);
//			p.flickerTimeInverval = entry.value("flickerTimeInverval", 0.f);
//			p.beamDuration = entry.value("beamDuration", 0.f);
//			p.fSpawnDelay = entry.value("fSpawnDelay", 0.f);
//			p.geometryType = entry.value("GeometryType", 0.f);
//			auto col = entry.value("color", std::vector<float>{1, 1, 1, 1});
//			p.color = { col[0], col[1], col[2], col[3] };
//			auto emi = entry.value("emissive", std::vector<float>{0, 0, 0, 0});
//			p.emissive = { emi[0], emi[1], emi[2], emi[3] };
//			auto endEmi = entry.value("endEmissive", std::vector<float>{0, 0, 0, 0});
//			p.endEmissive = { endEmi[0], endEmi[1], endEmi[2], endEmi[3] };
//			cmd.params = p;
//			break;
//		}
//		case SPAWN_COMMAND_KIND::PATTERN:
//		{
//			int kindIdx = entry.value("patternKindIndex", 0);
//			PatternParamVariant pv = MakeDefaultPatternParam(kindIdx);
//			const auto& paramJson = entry["patternParams"];
//			std::visit([&](auto& p) { LoadParam(p, paramJson); }, pv);
//			ApplyWorldMatToPattern(pv, matWorld);
//			auto spawnList = BuildSpawnData(pv);
//			cmd.sGroupTag_KindTag = SPAWN_COMMAND_KIND::PATTERN;
//			cmd.params = spawnList;
//			break;
//		}
//		default:
//			continue;
//		}
//		localQueue.push_back(cmd);
//	}
//	ExecuteCommandQueue(localQueue);
//	return m_iNextOwnerId++;
//}
// 1) 순수 파싱: matWorld 관여 없음, 로컬값 그대로
std::vector<SPAWN_COMMAND> CParticleManager::Parse_Command(const std::string& strJsonPath)
{
	std::vector<SPAWN_COMMAND> parsed;

	std::string path = "./Resources/json/Particle/" + strJsonPath;
	std::ifstream file(path);
	if (!file.is_open())
		return parsed;

	nlohmann::json j;
	try { file >> j; }
	catch (...) { return parsed; }
	if (!j.contains("commands") || !j["commands"].is_array())
		return parsed;

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
			p.bRandomPos = entry.value("bRandomPos", false);
			p.bRandomVel = entry.value("bRandomVel", false);
			p.bRandomRot = entry.value("bRandomRot", false);
			p.bLoop = entry.value("bLoop", false);

			auto posMin = entry.value("posMin", std::vector<float>{0, 0, 0});
			auto posMax = entry.value("posMax", std::vector<float>{0, 0, 0});
			auto pos = entry.value("position", std::vector<float>{0, 0, 0});
			p.posMin = { posMin[0], posMin[1], posMin[2] };
			p.posMax = { posMax[0], posMax[1], posMax[2] };
			p.position = { pos[0], pos[1], pos[2] };

			auto velMin = entry.value("velMin", std::vector<float>{0, 0, 0});
			auto velMax = entry.value("velMax", std::vector<float>{0, 0, 0});
			auto vel = entry.value("velocity", std::vector<float>{0, 0, 0});
			p.velMin = { velMin[0], velMin[1], velMin[2] };
			p.velMax = { velMax[0], velMax[1], velMax[2] };
			p.velocity = { vel[0], vel[1], vel[2] };

			auto rotMin = entry.value("rotMin", std::vector<float>{0, 0, 0});
			auto rotMax = entry.value("rotMax", std::vector<float>{0, 0, 0});
			auto rot = entry.value("Rotation", std::vector<float>{0, 0, 0, 0});
			p.rotMin = { rotMin[0], rotMin[1], rotMin[2] };
			p.rotMax = { rotMax[0], rotMax[1], rotMax[2] };
			p.rotation = { rot[0], rot[1], rot[2], rot[3] };

			p.life = entry.value("life", 1.f);

			auto startSize = entry.value("StartSize", std::vector<float>{0, 0, 0, 0});
			p.fSize = { startSize[0], startSize[1], startSize[2] };

			auto endSize = entry.value("EndSize", std::vector<float>{0, 0, 0, 0});
			p.fEndSize = { endSize[0], endSize[1], endSize[2] };

			auto col = entry.value("color", std::vector<float>{1, 1, 1, 1});
			auto emi = entry.value("emissive", std::vector<float>{0, 0, 0, 0});
			auto endEmi = entry.value("endEmissive", std::vector<float>{0, 0, 0, 0});
			p.color = { col[0], col[1], col[2], col[3] };
			p.emissive = { emi[0], emi[1], emi[2], emi[3] };
			p.endEmissive = { endEmi[0], endEmi[1], endEmi[2], endEmi[3] };

			p.fSpawnDelay = entry.value("fSpawnDelay", 0.f);
			p.fSpawnInterval = entry.value("fSpawnInterval", 0.f);

			cmd.params = p;
			break;
		}
		case SPAWN_COMMAND_KIND::BEAM:
		{
			BEAM_PARAMS p{};
			auto bs = entry.value("beamStart", std::vector<float>{0, 0, 0, 0});
			auto be = entry.value("beamEnd", std::vector<float>{0, 0, 0, 0});
			p.beamStart = { bs[0], bs[1], bs[2], bs[3] };
			p.beamEnd = { be[0], be[1], be[2], be[3] };

			p.iDisplacementIterations = entry.value("iDisplacementIterations", 0);
			p.fDisplacementAmplitude = entry.value("fDisplacementAmplitude", 0.f);
			p.fDisplacementDamping = entry.value("fDisplacementDamping", 0.f);
			p.flickerTimeInverval = entry.value("flickerTimeInverval", 0.f);
			p.beamDuration = entry.value("beamDuration", 0.f);
			p.fSpawnDelay = entry.value("fSpawnDelay", 0.f);
			p.geometryType = entry.value("GeometryType", 0.f);

			auto col = entry.value("color", std::vector<float>{1, 1, 1, 1});
			auto emi = entry.value("emissive", std::vector<float>{0, 0, 0, 0});
			auto endEmi = entry.value("endEmissive", std::vector<float>{0, 0, 0, 0});
			p.color = { col[0], col[1], col[2], col[3] };
			p.emissive = { emi[0], emi[1], emi[2], emi[3] };
			p.endEmissive = { endEmi[0], endEmi[1], endEmi[2], endEmi[3] };

			cmd.params = p;
			break;
		}
		case SPAWN_COMMAND_KIND::PATTERN:
		{
			int kindIdx = entry.value("patternKindIndex", 0);
			PatternParamVariant pv = MakeDefaultPatternParam(kindIdx);
			const auto& paramJson = entry["patternParams"];
			std::visit([&](auto& pp) { LoadParam(pp, paramJson); }, pv);
			cmd.params = pv; // 아직 matWorld 적용 전
			break;
		}
		default:
			continue;
		}

		parsed.push_back(cmd);
	}

	return parsed;
}
// 2) 진짜 스폰: 이미 파싱된 벡터만 받아서 변환+실행
uint32_t CParticleManager::Spawn(const std::vector<SPAWN_COMMAND>& templateCommands,
	const _float4x4& worldMat, _fvector endPos)
{
	XMMATRIX matWorld = XMLoadFloat4x4(&worldMat);

	if (XMVector4IsNaN(matWorld.r[0]) || XMVector4IsNaN(matWorld.r[1]) ||
		XMVector4IsNaN(matWorld.r[2]) || XMVector4IsNaN(matWorld.r[3]) ||
		XMVector4IsInfinite(matWorld.r[0]) || XMVector4IsInfinite(matWorld.r[1]) ||
		XMVector4IsInfinite(matWorld.r[2]) || XMVector4IsInfinite(matWorld.r[3]))
	{
		OutputDebugStringA("[CParticleManager::Spawn] invalid matWorld\n");
		return (uint32_t)E_FAIL;
	}

	std::vector<SPAWN_COMMAND> localQueue;
	localQueue.reserve(templateCommands.size());

	for (const auto& srcCmd : templateCommands)
	{
		SPAWN_COMMAND cmd = srcCmd; // 복사본에만 변환, 원본(호출자가 들고 있는 벡터)은 그대로
		cmd.ownerId = m_iNextOwnerId;

		switch (cmd.sGroupTag_KindTag)
		{
		case SPAWN_COMMAND_KIND::STANDARD:
		{
			auto& p = std::get<STANDARD_PARAMS>(cmd.params);

			_float3 posMinT, posMaxT;
			XMStoreFloat3(&posMinT, XMVector3TransformCoord(XMLoadFloat3(&p.posMin), matWorld));
			XMStoreFloat3(&posMaxT, XMVector3TransformCoord(XMLoadFloat3(&p.posMax), matWorld));
			p.posMin = { std::min(posMinT.x, posMaxT.x), std::min(posMinT.y, posMaxT.y), std::min(posMinT.z, posMaxT.z) };
			p.posMax = { std::max(posMinT.x, posMaxT.x), std::max(posMinT.y, posMaxT.y), std::max(posMinT.z, posMaxT.z) };

			_float3 posT;
			XMStoreFloat3(&posT, XMVector3TransformCoord(XMLoadFloat3(&p.position), matWorld));
			p.position = posT;

			_float3 velMinT, velMaxT;
			XMStoreFloat3(&velMinT, XMVector3TransformNormal(XMLoadFloat3(&p.velMin), matWorld));
			XMStoreFloat3(&velMaxT, XMVector3TransformNormal(XMLoadFloat3(&p.velMax), matWorld));
			p.velMin = { std::min(velMinT.x, velMaxT.x), std::min(velMinT.y, velMaxT.y), std::min(velMinT.z, velMaxT.z) };
			p.velMax = { std::max(velMinT.x, velMaxT.x), std::max(velMinT.y, velMaxT.y), std::max(velMinT.z, velMaxT.z) };

			_float3 velT;
			XMStoreFloat3(&velT, XMVector3TransformNormal(XMLoadFloat3(&p.velocity), matWorld));
			p.velocity = velT;
			// rotMin/rotMax는 변환하지 않음
			break;
		}
		case SPAWN_COMMAND_KIND::BEAM:
		{
			auto& p = std::get<BEAM_PARAMS>(cmd.params);
			XMStoreFloat4(&p.beamStart, XMVector3TransformCoord(XMLoadFloat4(&p.beamStart), matWorld));
			XMStoreFloat4(&p.beamEnd, XMLoadFloat4(&p.beamEnd) + endPos);
			break;
		}
		case SPAWN_COMMAND_KIND::PATTERN:
		{
			PatternParamVariant pv = std::get<PatternParamVariant>(cmd.params);
			ApplyWorldMatToPattern(pv, matWorld);
			cmd.params = BuildSpawnData(pv);
			break;
		}
		default:
			break;
		}

		localQueue.push_back(std::move(cmd));
	}


	return 	ExecuteCommandQueue(localQueue);
}


HRESULT CParticleManager::PlayEffect(const std::string& presetName, const _float3& position, uint32_t count)
{
	auto it = m_ParticlePresets.find(presetName);
	if (it == m_ParticlePresets.end()) {
		OutputDebugStringA(("PlayEffect: 프리셋을 찾을 수 없음 - " + presetName + "\n").c_str());
		return E_FAIL;
	}

	const auto& preset = it->second;

	PARTICLE_SPAWN_DATA data{};
	data.position = position;
	data.velocity =  preset.velocity;
	data.life = preset.maxLife;
	data.fSize = preset.fStartSize;
	data.fEndSize = preset.fEndSize;
	data.color = preset.StartColor;
	data.emissive = preset.Emissive;
	data.endEmissive = preset.endEmissive;
	data.iBehaviorType = preset.iBehaviorType;
	data.rotation = preset.rotation;
	data.originalPosition = position;
	data.originalVelocity = data.velocity;
	return Spawn(preset.sGroupTag, preset.sTypeTag, count, &data);
}
// ParticleManager.cpp
HRESULT CParticleManager::DeleteEffectPreset(const std::string& strJsonPath, const std::string& presetName)
{
	if (!std::filesystem::exists(strJsonPath))
		return E_FAIL;

	nlohmann::json j;
	std::ifstream inFile(strJsonPath);
	if (inFile.is_open())
	{
		try { inFile >> j; }
		catch (...) { return E_FAIL; }
	}

	if (!j.contains("presets") || !j["presets"].is_array())
		return E_FAIL;

	bool bRemoved = false;
	auto& arr = j["presets"];
	for (auto it = arr.begin(); it != arr.end(); ++it)
	{
		if (it->value("presetName", "") == presetName)
		{
			arr.erase(it);
			bRemoved = true;
			break;
		}
	}

	if (!bRemoved)
		return E_FAIL;

	std::ofstream file(strJsonPath, std::ios::out | std::ios::trunc);
	if (!file.is_open())
		return E_FAIL;

	file << j.dump(4);
	return S_OK;
}

std::vector<PARTICLE_SPAWN_DATA> CParticleManager::BuildSpawnData(const PatternParamVariant& v)
{
	return std::visit([](const auto& param) -> std::vector<PARTICLE_SPAWN_DATA>
		{
			using T = std::decay_t<decltype(param)>;
			if constexpr (std::is_same_v<T, SStairsParam>)
				return ParticlePattern::MakeStairs(param);
			else if constexpr (std::is_same_v<T, SCircleParam>)
				return ParticlePattern::MakeCircle(param);
			else if constexpr (std::is_same_v<T, SSpiralParam>)
				return ParticlePattern::MakeSpiral(param);
			else if constexpr (std::is_same_v<T, SStraightGroundParam>)
				return ParticlePattern::MakeStraightGround(param);
			else if constexpr (std::is_same_v<T, SCircleSpreadParam>)
				return ParticlePattern::MakeCircleAndSpread(param);
			else if constexpr (std::is_same_v<T, SMOKE>)
				return ParticlePattern::MakeSmoke(param);
			else if constexpr (std::is_same_v<T, SLightning>)
				return ParticlePattern::MakeLightning(param);
			//else if constexpr (std::is_same_v<T, SLightning_Tex>)
			//	return ParticlePattern::MakeLightning(param);
			else
			{
				static_assert(!sizeof(T*), "BuildSpawnData: unhandled PatternParamVariant type");
				return {};
			}
		}, v);
}
void CParticleManager::ApplyStartEndToPattern(PatternParamVariant& pv, _fvector startPos, _fvector endPos)
{
	std::visit([&](auto& p)
		{
			using T = std::decay_t<decltype(p)>;

			if constexpr (std::is_same_v<T, SStairsParam>)
			{
				
				XMStoreFloat3(&p.vStartPos, startPos);
			}
			else if constexpr (std::is_same_v<T, SCircleParam>)
			{
				XMStoreFloat3(&p.vCenter, startPos);
			}
			else if constexpr (std::is_same_v<T, SSpiralParam>)
			{
				XMStoreFloat3(&p.vCenter, startPos);
			}
			else if constexpr (std::is_same_v<T, SStraightGroundParam>)
			{
				XMStoreFloat3(&p.vStartPos, startPos);
			}
			//XMStoreFloat3(&p.vStartPos, startPos);
			//XMStoreFloat3(&p.vEndPos, endPos);

		}, pv);
}
void CParticleManager::ApplyWorldMatToPattern(PatternParamVariant& pv, FXMMATRIX matWorld)
{
	XMVECTOR vWorldOrigin = XMVector3TransformCoord(XMVectorZero(), matWorld);

	std::visit([&](auto& p)
		{
			using T = std::decay_t<decltype(p)>;

			if constexpr (std::is_same_v<T, SStairsParam>)
			{
				XMStoreFloat3(&p.vStartPos, vWorldOrigin);
			}
			else if constexpr (std::is_same_v<T, SCircleParam>)
			{
				XMStoreFloat3(&p.vCenter, vWorldOrigin);
			}
			else if constexpr (std::is_same_v<T, SSpiralParam>)
			{
				XMStoreFloat3(&p.vCenter, vWorldOrigin);
			}
			else if constexpr (std::is_same_v<T, SStraightGroundParam>)
			{
				XMStoreFloat3(&p.vStartPos, vWorldOrigin);
			}
		}, pv);
}
std::vector<std::string> CParticleManager::ScanBinFolder(const std::string& strBinFolder)
{
	std::vector<std::string> result;
	if (!std::filesystem::exists(strBinFolder))
		return result;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(strBinFolder))
	{
		if (!entry.is_regular_file())
			continue;
		if (_stricmp(entry.path().extension().string().c_str(), ".bin") == 0)
		{
			std::filesystem::path relPath = std::filesystem::relative(entry.path(), strBinFolder);
			std::string s = relPath.string();
			std::replace(s.begin(), s.end(), '\\', '/');   // 백슬래시 -> 슬래시
			result.push_back(s);
		}
	}
	return result;
}
void CParticleManager::ClearByOwner(uint32_t ownerId)
{
	if (ownerId == INVALID_PARTICLE_OWNER_ID)
		return;

	for (auto& [groupTag, particleGroup] : m_Particles)
	{
		for (auto& [typeTag, particle] : particleGroup)
		{
			if (particle)
				particle->ClearByOwner(ownerId);
		}
	}

	DeleteLoopRequests(ownerId);
}
void CParticleManager::TranslateOwner(uint32_t ownerId,const _float3& delta)
{
	for (auto& [groupTag, particleGroup] :m_Particles)
	{
		for (auto& [typeTag, particle] :
			particleGroup)
		{
			if (particle)
			{
				particle->TranslateOwner(ownerId,delta);
			}
		}
	}
}
void CParticleManager::TransformOwner(uint32_t ownerId, const _float4x4& deltaMatrixData)
{
	for (auto& [groupTag, particleGroup] :m_Particles)
	{
		for (auto& [typeTag, particle] :
			particleGroup)
		{
			if (particle)
			{
				particle->TransformOwner(ownerId, deltaMatrixData);
			}
		}
	}
}
std::vector<std::string> CParticleManager::Load_FilePath_ByExtension(const std::filesystem::path& _FolderPath, std::string_view _Extension) {
	std::vector<std::string> FilePathStorage{};
	FilePathStorage.reserve(32);

	{
		namespace fs = std::filesystem;

		std::error_code ErrorCode{};

		if (fs::exists(_FolderPath) == false || fs::is_directory(_FolderPath) == false) {
			std::wstring MSGContent = L"Invalid FolderPath : " + _FolderPath.wstring();
			MessageBoxW(NULL, MSGContent.c_str(), L"System Message", MB_OK);

			return FilePathStorage;      // Empty vector return
		}
		auto Optimization = fs::directory_options::skip_permission_denied;

		fs::recursive_directory_iterator iterator(_FolderPath, Optimization, ErrorCode);
		fs::recursive_directory_iterator End;

		for (; iterator != End && !ErrorCode; iterator.increment(ErrorCode)) {
			if (iterator->is_regular_file(ErrorCode)) {
				const auto& FilePath = iterator->path();

				if (FilePath.extension() == _Extension) {
					FilePathStorage.push_back(FilePath.string());
				}
			}
		}
		return FilePathStorage;
	}
}

HRESULT CParticleManager::Load_ParticleJsonPackage(const std::vector<std::string>& _FilePathPackage) {
	if (_FilePathPackage.size() <= 0) return E_FAIL;
	for (const auto& FilePath : _FilePathPackage) {
		CGameInstance::Get().LoadParticleJson(FilePath.c_str());
	}
	return S_OK;
}
