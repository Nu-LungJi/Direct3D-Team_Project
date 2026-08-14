#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CResPixelShader;
class CResTexture2D;

enum class DECAL_PARAMETER_TYPE : uint8_t
{
	FLOAT,
	FLOAT2,
	FLOAT3,
	FLOAT4,
	COLOR3,
	COLOR4
};

class ENGINE_DLL CDecalMaterial final
{
public:
	static constexpr UINT TEXTURE_SLOT_BEGIN = 2;
	static constexpr UINT TEXTURE_SLOT_END = 7;
	static constexpr size_t PARAMETER_FLOAT_COUNT = 32;

	struct PARAMETER_DESC
	{
		_string name{};
		DECAL_PARAMETER_TYPE type{ DECAL_PARAMETER_TYPE::FLOAT };
		uint32_t offset{};
		uint32_t count{ 1 };
		_float minValue{};
		_float maxValue{ 1.f };
		_float speed{ 0.01f };
	};

	struct TEXTURE_DESC
	{
		_string name{};
		UINT slot{ TEXTURE_SLOT_BEGIN };
		_string path{};
		SPtr<CResTexture2D> texture{};
	};

private:
	explicit CDecalMaterial(_string path);

public:
	~CDecalMaterial() = default;

	HRESULT Load();
	HRESULT Bind(ID3D11DeviceContext* context) const;
	void Unbind(ID3D11DeviceContext* context) const;

	const _string& GetPath() const { return m_Path; }
	const _string& GetName() const { return m_Name; }
	const std::array<_float, PARAMETER_FLOAT_COUNT>& GetDefaultParameters() const { return m_DefaultParameters; }
	const std::vector<PARAMETER_DESC>& GetParameters() const { return m_Parameters; }
	const std::vector<TEXTURE_DESC>& GetTextures() const { return m_Textures; }
	const PARAMETER_DESC* FindParameter(const _string& name) const;

	static SPtr<CDecalMaterial> LoadShared(const _string& path);
	static std::vector<_string> FindMaterialFiles(const _string& rootPath);

private:
	_string m_Path{};
	_string m_Name{};
	SPtr<CResPixelShader> m_PixelShader{};
	std::array<_float, PARAMETER_FLOAT_COUNT> m_DefaultParameters{};
	std::vector<PARAMETER_DESC> m_Parameters{};
	std::vector<TEXTURE_DESC> m_Textures{};
};

NS_END
