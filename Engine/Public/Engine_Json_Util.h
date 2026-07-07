#pragma once

namespace Engine
{
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DEST_NODE, DestName, iDestNode)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GUINODE, iID, Name,  fValue)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ACTION_VALUE, iAnimIndex, fSpeed, fTime, fTick)
	//_float 2 3 4, enum은 수동으로

	class JsonSaveLoadManager
	{
	public:
		/////////////Save
		static void  SaveJsonTypeString(nlohmann::json& jsonFile, const _string& TypeName, const _string& Src)
		{
			jsonFile[TypeName] = Src;
		}
		static void  SaveJsonTypeFloat4(nlohmann::json& jsonFile, const _string& TypeName, _float4& Float4)
		{
			jsonFile[TypeName] = { Float4.x,Float4.y,Float4.z,Float4.w };
		}
		static void  SaveJsonTypeFloat3(nlohmann::json& jsonFile, const _string& TypeName, _float3& Float3)
		{
			jsonFile[TypeName] = { Float3.x,Float3.y,Float3.z };
		}
		static void  SaveJsonTypeFloat2(nlohmann::json& jsonFile, const _string& TypeName, _float2& Float2)
		{
			jsonFile[TypeName] = { Float2.x,Float2.y };
		}
		///////////////Load
		static _bool  LoadJsonTypeString(const nlohmann::json& jsonFile, const _string& TypeName, _string& Src)
		{
			if (jsonFile.contains(TypeName))
			{
				Src = jsonFile[TypeName];
				return true;
			}
			return false;
		}
		static _bool  LoadJsonTypeFloat4(const nlohmann::json& jsonFile, const _string& TypeName, _float4& Float4)
		{
			if (jsonFile.contains(TypeName))
			{
				Float4 = _float4(jsonFile[TypeName][0], jsonFile[TypeName][1], jsonFile[TypeName][2], jsonFile[TypeName][3]);
				return true;
			}
			return false;
		}
		static _bool  LoadJsonTypeFloat3(const nlohmann::json& jsonFile, const _string& TypeName, _float3& Float3)
		{
			if (jsonFile.contains(TypeName))
			{
				Float3 = _float3(jsonFile[TypeName][0], jsonFile[TypeName][1], jsonFile[TypeName][2]);
				return true;
			}
			return false;
		}
		static _bool  LoadJsonTypeFloat2(const nlohmann::json& jsonFile, const _string& TypeName, _float2& Float2)
		{
			if (jsonFile.contains(TypeName))
			{
				Float2 = _float2(jsonFile[TypeName][0], jsonFile[TypeName][1]);
				return true;
			}
			return false;
		}
		static _bool  LoadJsonTypeUINT(const nlohmann::json& jsonFile, const _string& TypeName, uint32_t& iUlnt)
		{
			if (jsonFile.contains(TypeName))
			{
				iUlnt = jsonFile[TypeName];
				return true;
			}
			return false;
		}
		static _bool  LoadJsonTypeINT(const nlohmann::json& jsonFile, const _string& TypeName, int32_t& iUlnt)
		{
			if (jsonFile.contains(TypeName))
			{
				iUlnt = jsonFile[TypeName];
				return true;
			}
			return false;
		}
	};
	
	template<typename T>
		void SaveJsonValue(nlohmann::json& jsonFile, const _string& TypeName, T& Value)
		{
			jsonFile[TypeName] = Value;
		}
	template<typename T>
		_bool LoadJsonValue(const nlohmann::json& jsonFile, const _string& TypeName, T& Value)
		{
			if (jsonFile.contains(TypeName))
			{
				Value = jsonFile[TypeName];
				return true;
			}
			return false;
		}


	template<typename ENUM>
	void		SaveJsonEnum(nlohmann::json& jsonFile, const _string& TypeName, ENUM& eNum)
	{
		uint32_t iType = static_cast<uint32_t>(eNum);
		jsonFile[TypeName] = iType;
	}
	template<typename ENUM>
	_bool		LoadJsonEnum(const nlohmann::json& jsonFile, const _string& TypeName, ENUM& eNum)
	{
		if (jsonFile.contains(TypeName))
		{
			uint32_t iType = jsonFile[TypeName];
			eNum = static_cast<ENUM>(iType);
			return true;
		}
		return false;
	}
}
