#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

typedef struct tagUIInfo
{
	/* 루트 노드 인포 */
	_float fX{ 0.f }, fY{ 0.f };
	_float SizeX{ 0.f }, SizeY{ 0.f }, Alpha{ 1.f };
	_float Rot{ 0.f };
	int Weight{ 0 };

	/* 자식 로컬 인포*/
	_float LocalX{ 0.f }, LocalY{ 0.f };
	_float WidthRatioX{ 1.f }, WidthRatioY{ 1.f }, AlphaRatio{ 1.f };
	_float LocalRot{ 0.f };
	int WeightOffset{ 0 };

	std::string		Name = "";						// 이름
	std::string		Restag = "";					// 리소스 태그
	uint32_t		UIType{ 0 };					// 텍스처냐 플립북이냐
	uint32_t		EffectType{ 0 };				// 호버링이펙트, 클릭이펙트 등등
	_float3			Color = { 0.f, 0.f, 0.f };		// 색
}UI_INFO;

class ENGINE_DLL CUIObject : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CUIObject, CGameObject)

public:
	enum class UI_STATE { ENTER, IDLE, EXIT, CLICK, APPEAR, DISAPPEAR, NONE };

public:
	typedef struct tagUIObjectDesc : public CGameObject::GAMEOBJECT_DESC
	{
		_float			fX, fY, fSizeX, fSizeY, fAlpha;
		std::string		ResTag;
		std::string		Name;
		uint32_t		ResWeight;
		uint32_t		UIType;
	}UIOBJECT_DESC;

protected:
	CUIObject();
	~CUIObject() override;

public:
	void UpdateGUI() override;

public:
	HRESULT Initialize(void* pArg) override;
	virtual void Update(_float fTimeDelta);
	virtual void LateUpdate(_float fTimeDelta);

protected:
	UI_INFO		m_UIINFO{};
	UI_STATE	m_CurrentState = UI_STATE::NONE;
	uint32_t	m_AnimState = 0;
	_bool		m_isActive = true;
	_bool		m_isVisible = true;

public:
	const UI_INFO& GetUIInfo() const { return m_UIINFO; }
	UI_INFO& GetUIInfo() { return m_UIINFO; }

	_bool GetActive() { return m_isActive; }
	_bool GetVisible() { return m_isVisible; }

	void SetActive(bool isActive) { m_isActive = isActive; }
	void SetVisible(bool isVisible) { m_isVisible = isVisible; }

	const char* GetName() { return m_UIINFO.Name.c_str(); }
public:
	void SetParent(std::optional<CHandle> parentUI) { m_pParent = parentUI; }
	std::optional<CHandle>  GetParent() { return m_pParent; }
	void AddChildren(CHandle childUI) { m_vChildren.push_back(childUI); }
	const std::vector<CHandle>& GetChildren() const { return m_vChildren; }

protected:
	_float m_fX{}, m_fY{}, m_fSizeX{}, m_fSizeY{}, m_fAlpha{};
	int m_iWeight{};
	char m_cName[256] = "";
	uint32_t m_UIType{};
	uint32_t m_iEffectType{};

	_float m_fLocalX{ 0 }, m_fLocalY{ 0 }, m_fWidthRatioX{ 1 }, m_fWidthRatioY{ 1 }, m_fAlphaRatio{ 1 };
	int m_iWeightOffset{ 0 };

	_float3 m_vColor = { 0, 0, 0 };

	std::string m_sRestag;

public:
	_float2 GetOrigin() const { return { m_fX , m_fY }; }
	_float2 GetSize() const { return{ m_fSizeX , m_fSizeY }; }
	void SetOrigin(_float2 f) { m_fX = f.x; m_fY = f.y;   CalcUICoord(); }
	void SetSize(_float2 f) { m_fSizeX = f.x ; m_fSizeY = f.y; CalcUICoord();}
	void SetAlpha(_float fAlpha) { m_fAlpha = fAlpha; }
	_float GetAlpha() { return m_fAlpha; }

	int			GetWeight() const { return m_iWeight; }
	void		SetWeight(int weight) { m_iWeight = weight; }

	
	void		SetName(_string name)	{ strcpy_s(m_cName, name.c_str()); }

	void SetLocalPos(_float2 localPos) { m_fLocalX = localPos.x; m_fLocalY = localPos.y;     CalcUICoord();};
	void SetWorldPos(_float2 worldPos) { m_fX = worldPos.x; m_fY = worldPos.y;     CalcUICoord();}

	_float2 GetLocalPos() { _float2 localPos = { m_fLocalX , m_fLocalY }; return localPos; }
	_float2 GetWorldPos() { _float2 worldPos = { m_fX , m_fY }; return worldPos; }



	_float GetLocalX() { return m_fLocalX; };
	_float GetLocalY() { return m_fLocalY; };
	_float GetWidthRatioX() { return m_fWidthRatioX; };
	_float GetWidthRatioY() { return m_fWidthRatioY; };
	_float GetAlphaRatio() { return m_fAlphaRatio; };
	int GetWeightOffset() { return m_iWeightOffset; };

	void SetLocalX(_float x) { m_fLocalX = x; };
	void SetLocalY(_float y) { m_fLocalY = y; };
	void SetWidthRatioX(_float widthX) { m_fWidthRatioX = widthX; };
	void SetWidthRatioY(_float widthY) { m_fWidthRatioY = widthY; };
	void SetAlphaRatio(_float alphaRatio) { m_fAlphaRatio = alphaRatio; };
	void SetWeightOffset(int weightOffset) { m_iWeightOffset = weightOffset; };

	uint32_t GetUIType() { return m_UIType; }
	void SetUIType(uint32_t uiType) { m_UIType = uiType; }

	std::string Get_ResTag() { return m_sRestag; }
	void Set_ResTag(std::string tag) { m_sRestag = tag; }

	_float3 GetColor() { return m_vColor; }
	void SetColor(_float3 vColor) { m_vColor = vColor; }

	uint32_t GetEffectType() { return m_iEffectType; }
	void SetEffectType(uint32_t effectType) { m_iEffectType = effectType; }
public:
	void DeleteChild(CHandle childHandle);
	void CalcUICoord();
protected:
	_bool CheckHovered();

protected:
	std::optional<CHandle> m_pParent = std::nullopt;

	std::vector<CHandle> m_vChildren;
};

NS_END
