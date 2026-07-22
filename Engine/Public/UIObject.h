#pragma once

#include "GameObject.h"
#include "UIComponent.h"
#include "ButtonComponent.h"
#include "TweenComponent.h"

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

/****************UI***********************/
// 개별 애니메이션 트랙 데이터
typedef struct tagUITweenTrack
{
	EUITweenTarget TargetType;
	bool bUseCurrentStart; // 시작값을 현재 UI의 상태값으로 쓸 것인지?
	float fStartValue;     // bUseCurrentStart가 false일 때 쓸 고정 시작값
	float fEndValue;       // 목표값
	float fDuration;       // 걸리는 시
}UI_TWEENTRACK;

typedef struct tagUIEventString
{
	std::string ClickFunc;
	std::string ClickAction;
	std::string EnterAction;
	std::string ExitAction;
	std::string AppearAction;
	std::string DisappearAction;
}UI_EVENT;

// 애니메이션 클립
typedef struct tagUIAnimClip
{
	std::string ClipName;
	std::vector<UI_TWEENTRACK> Tracks;
}UI_ANIMCLIP;

class ENGINE_DLL CUIObject : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CUIObject, CGameObject)

public:
	enum class UI_STATE
	{
		NONE = 0,
		ENTER = 1 << 0, // 1
		HOVERED = 1 << 1, // 2
		EXIT = 1 << 2, // 4
		CLICK = 1 << 3, // 8
		APPEAR = 1 << 4, // 16
		DISAPPEAR = 1 << 5  // 32
	};

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

public:
	virtual void PlayEffect(uint32_t uiState);

protected:
	std::vector<CUIComponent*> m_UIComponents;

	CComConstantBuffer* m_pComCBufferPerUI = nullptr;
	CButtonComponent* m_pComCButton = nullptr;
	TweenComponent* m_pComTween = nullptr;
public:
	TweenComponent* GetTweenCom() { return m_pComTween; }

	/* 이벤트 콜백 함수 */
	std::function<void(CUIObject* pCaller)> OnClicked;
	std::function<void(CUIObject* pCaller)> OnHoverEnter;
	std::function<void(CUIObject* pCaller)> OnHoverExit;
	std::function<void(CUIObject* pCaller)> Appear;
	std::function<void(CUIObject* pCaller)> Disappear;

	std::function<void(std::string text)> OnClickedAction;

	void ClearEffectTweens();

protected:
	UI_INFO		m_UIINFO{};
	UI_EVENT	m_UIEVENT{};
	UI_STATE	m_CurrentState = UI_STATE::NONE;
	uint32_t	m_AnimState = 0;
	_bool		m_isActive = true;
	_bool		m_isVisible = true;
	_float		m_ScaleRatio = 1.f;
	_float2		m_vPivot{};

	bool m_bInputLocked = false;

public:
	const UI_INFO& GetUIInfo() const { return m_UIINFO; }
	UI_INFO& GetUIInfo() { return m_UIINFO; }

	const UI_EVENT& GetUIEvent() const { return m_UIEVENT; }
	UI_EVENT& GetUIEvent() { return m_UIEVENT; }

	_bool	GetInputLcok() { return m_bInputLocked; }
	void	SetInputLcok(_bool inputlock) { m_bInputLocked = inputlock; }

	_float2 GetPos() { return { m_UIINFO.fX, m_UIINFO.fY }; }
	_float2	GetLocalPos() { return { m_UIINFO.fX, m_UIINFO.fY }; }
	_float	GetAlpha() { return m_UIINFO.Alpha; }
	_float2 GetSize() { return { m_UIINFO.SizeX, m_UIINFO.SizeY };  }

	void	SetPos(_float2 pos) { m_UIINFO.fX = pos.x; m_UIINFO.fY = pos.y; }
	void	SetLocalPos(_float2 pos) { m_UIINFO.LocalX = pos.x; m_UIINFO.LocalY = pos.y; }
	void	SetAlpha(_float alpha) { m_UIINFO.Alpha = alpha; }
	void	SetSize(_float2 size) { m_UIINFO.SizeX = size.x; m_UIINFO.SizeY = size.y; }

	_bool GetActive() { return m_isActive; }
	_bool GetVisible() { return m_isVisible; }
	_float GetAlphaRatio() { return m_UIINFO.AlphaRatio; }
	_float GetScaleRatio() { return m_ScaleRatio; }
	_float GetLocalRot() { return m_UIINFO.LocalRot; }

	void SetActive(bool isActive) { m_isActive = isActive; }
	void SetVisible(bool isVisible) { m_isVisible = isVisible; }
	void SetAlphaRatio(_float alpha) { m_UIINFO.AlphaRatio = alpha; }
	void SetScaleRatio(_float scale) { m_ScaleRatio = scale; }
	void SetLocalRot(_float rot) { m_UIINFO.LocalRot = rot; }

	const char* GetName() { return m_UIINFO.Name.c_str(); }
	const uint32_t* GetUIType() { return &m_UIINFO.UIType; }
	void SetUIType(uint32_t uiType) { m_UIINFO.UIType = uiType; }
	void SetColor(_float3 vColor) { m_UIINFO.Color = vColor; }
	int GetWeight() { return m_UIINFO.Weight; }
	int GetWeight() const { return m_UIINFO.Weight; }
public:
	void						SetParent(std::optional<CHandle> parentUI) { m_pParent = parentUI; }
	std::optional<CHandle>		GetParent() { return m_pParent; }
	void						AddChildren(CHandle childUI) { m_vChildren.push_back(childUI); }
	const std::vector<CHandle>& GetChildren() const { return m_vChildren; }
	void SetPivot(_float2 vPivot) { m_vPivot = vPivot; }
	_float2 GetPivot() { return m_vPivot; }

public:
	void DeleteChild(CHandle childHandle);
	void CalcUICoord();
private:
	void SetChildPivot();

protected:
	std::optional<CHandle> m_pParent = std::nullopt;

	std::vector<CHandle> m_vChildren;
};

NS_END
