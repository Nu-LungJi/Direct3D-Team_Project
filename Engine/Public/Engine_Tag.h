#pragma once
namespace Engine
{
#define DECLARE_TAG(a, b) \
    static constexpr const char* a = #b; \
    static constexpr const _string_id ID##a = STRID(#b);

	//DECLARE_TAG(TAG_RES_VIBUFFER_QUADCOL, VIBUFFER_QUADCOL)
	//DECLARE_TAG(TAG_RES_VIBUFFER_QUADTEX, VIBUFFER_QUADTEX)
	//DECLARE_TAG(TAG_RES_CBUFFER, CBUFFER);
	//DECLARE_TAG(TAG_RES_FMOD_SOUND, FMOD_SOUND);
	//DECLARE_TAG(TAG_RES_JSON, JSON);
	//DECLARE_TAG(TAG_RES_VERTEX_SHADER, VERTEX_SHADER);
	//DECLARE_TAG(TAG_RES_PIXEL_SHADER, PIXEL_SHADER);

	static constexpr const char* TAG_RES_GRP_MAPEDITOR_STATIC_MODEL = "MAPEDITOR_STATIC_MODEL"; // 리소스매니저 스태틱모델그룹 태그
	static constexpr const char* TAG_RES_MAPEDITOR_DEFAULT_STATIC_MODEL = "HorseStatue_SM_HorseStatue"; // 디폴트로 사용할 스태틱모델 태그
	static constexpr const char* PATH_MAPEDITOR_STATIC_MODEL_DIR = R"(.\Resources\SampleClient\Models\LevelAnimEditor\Static)"; // 스태틱모델 바이너리화 한 폴더 // 여기서 스태틱 모델 다 긁어와서 리소스매니저에 집어넣을거임
	static constexpr const char* MAP_SAVE_ROOT = "./Resources/json/MapSaved/"; // Map저장 json 경로
	static constexpr const char* MAPMESHOBJECTLAYER = "98_MAPMESHOBJECT"; // MapMeshObject만의 게임오브젝트 레이어
}
