#pragma once

#include "UIObject.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
NS_END


NS_BEGIN(Client)

class CFlipBook final : public E::CUIObject
{
public:
	DECLARE_DERIVED_TYPE(CFlipBook, CUIObject)
public:
	typedef struct tagFlipbookDesc : public E::CUIObject::UIOBJECT_DESC
	{
		_float	cellsize;
		int		m_TotalFrame;
		_float 	m_fDuration;
	}FLIPBOOK_DESC;

private:
	CFlipBook();
	~CFlipBook() override;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	_float	cellsize = 4096;
	_float	m_fPadding = 2 / 4096;

	int		m_CurrentFrame = 0;
	int		m_StartFrame = 0;
	int		m_TotalFrame = 64;

	int		m_Columns = 8;
	int		m_Rows = 8;
	_float	m_curColum = 0;
	_float	m_curRow = 0;

	_float m_fDuration = 1.5f;
	_float m_fSumTime = 0.f;

	_float2 m_texcoord;
	_float2 m_uvSize;

	bool  m_Loop = true;
	
	uint32_t	m_iPuaseFrame = 9;
	_float		m_fPauseTime = 0.2f;
	_float		m_fPauseSumTime = 0.f;

	bool m_isPause = false;
	CComConstantBuffer* m_pComCBufferPerUI = nullptr;
public:
	_float GetCellSize()	{ return cellsize; }
	int GetTotalFrame()		{ return m_TotalFrame; }
	_float GetDuration()	{ return m_fDuration;}

	void SetCellSize(_float fcellsize) { cellsize = fcellsize; }
	void SetTotalFrame(int totalFrame) { m_TotalFrame = totalFrame; }
	void SetDuration(_float fDuration) { m_fDuration = fDuration; }

public:
	static E::UPtr<CFlipBook> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END