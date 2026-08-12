#include "FlipbookUI.h"

CFlipbookUI::CFlipbookUI()
{
}

CFlipbookUI::~CFlipbookUI()
{
}

HRESULT CFlipbookUI::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CFlipbookUI::FLIPBOOK_DESC*>(pArg);
	m_FLIPINFO.cellsize = pDesc->cellsize;
	m_FLIPINFO.TotalFrame = pDesc->TotalFrame;
	m_FLIPINFO.Columns = pDesc->Columns;
	m_FLIPINFO.Rows = pDesc->Rows;
	m_FLIPINFO.Duration = pDesc->Duration;
	m_FLIPINFO.Padding = pDesc->Padding;

	// A flipbook created during another object's Update can be rendered before
	// its own first Update. Prepare frame zero here so that first render does
	// not sample a zero-sized UV region and stretch one texel over the quad.
	const uint32_t cellSize = std::max(1u, m_FLIPINFO.cellsize);
	m_Columns = std::max(1u, m_FLIPINFO.Columns);
	m_Rows = std::max(1u, m_FLIPINFO.Rows);
	const uint32_t frameCount = std::clamp(
		std::max(1u, m_FLIPINFO.TotalFrame),
		1u,
		m_Columns * m_Rows);
	m_CurrentFrame %= frameCount;
	m_curColum = static_cast<_float>(m_CurrentFrame % m_Columns);
	m_curRow = static_cast<_float>(m_CurrentFrame / m_Columns);
	const _float padding = 2.f / static_cast<_float>(cellSize);
	m_texcoord = {
		m_curColum / static_cast<_float>(m_Columns) + padding,
		m_curRow / static_cast<_float>(m_Rows) + padding
	};
	m_uvSize = {
		1.f / static_cast<_float>(m_Columns) - padding * 2.f,
		1.f / static_cast<_float>(m_Rows) - padding * 2.f
	};

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	return S_OK;
}

void CFlipbookUI::Update(_float fTimeDelta)
{
	CUIObject::Update(fTimeDelta);

	uint32_t	cellSize = std::max(1u, m_FLIPINFO.cellsize);
	uint32_t	totalFrame = std::max(1u, m_FLIPINFO.TotalFrame);
	_float		duration = m_FLIPINFO.Duration;
	_float		padding = m_FLIPINFO.Padding / cellSize;

	padding = 2.f / cellSize;
	m_Columns = std::max(1u, m_FLIPINFO.Columns);
	m_Rows = std::max(1u, m_FLIPINFO.Rows);
	const uint32_t frameCount = std::clamp(
		totalFrame,
		1u,
		m_Columns * m_Rows);
	m_CurrentFrame %= frameCount;

	if (m_Loop == false && m_CurrentFrame >= frameCount - 1)
		return;


	if (m_UIINFO.Name != "House")
	{
		m_fSumTime += fTimeDelta;

		const uint32_t playableFrameCount = std::max(
			1u,
			frameCount - std::min(m_StartFrame, frameCount - 1));
		float delta = duration / playableFrameCount;

		if (m_fSumTime >= delta)
		{
			m_fSumTime = 0.f;
			m_CurrentFrame = m_StartFrame +
				(m_CurrentFrame - m_StartFrame + 1) % playableFrameCount;
		}
	}
	else
	{
		if (m_CurrentFrame % m_iPuaseFrame == 0 && m_fPauseSumTime < m_fPauseTime && m_CurrentFrame != 0)
		{
			m_fPauseSumTime += fTimeDelta;
		}
		else
		{
			m_fPauseSumTime = 0.f;

			m_fSumTime += fTimeDelta;

			const uint32_t playableFrameCount = std::max(
				1u,
				frameCount - std::min(m_StartFrame, frameCount - 1));
			float delta = duration / playableFrameCount;

			if (m_fSumTime >= delta)
			{
				m_fSumTime = 0.f;
				m_CurrentFrame = m_StartFrame +
					(m_CurrentFrame - m_StartFrame + 1) % playableFrameCount;
			}
		}
	}

	m_curColum = m_CurrentFrame % m_Columns;
	m_curRow = m_CurrentFrame / m_Columns;

	m_texcoord = { m_curColum / (float)m_Columns + padding, m_curRow / (float)m_Rows + padding };
	m_uvSize = { 1 / (float)m_Columns - padding * 2 , 1 / (float)m_Rows - padding * 2 };
}

void CFlipbookUI::LateUpdate(_float fTimeDelta)
{
}

void CFlipbookUI::Restart()
{
	const uint32_t cellSize = std::max(1u, m_FLIPINFO.cellsize);
	m_Columns = std::max(1u, m_FLIPINFO.Columns);
	m_Rows = std::max(1u, m_FLIPINFO.Rows);
	const uint32_t frameCount = std::clamp(
		std::max(1u, m_FLIPINFO.TotalFrame),
		1u,
		m_Columns * m_Rows);

	m_StartFrame = std::min(m_StartFrame, frameCount - 1u);
	m_CurrentFrame = m_StartFrame;
	m_fSumTime = 0.f;
	m_fPauseSumTime = 0.f;
	m_curColum = static_cast<_float>(m_CurrentFrame % m_Columns);
	m_curRow = static_cast<_float>(m_CurrentFrame / m_Columns);

	const _float padding = 2.f / static_cast<_float>(cellSize);
	m_texcoord = {
		m_curColum / static_cast<_float>(m_Columns) + padding,
		m_curRow / static_cast<_float>(m_Rows) + padding
	};
	m_uvSize = {
		1.f / static_cast<_float>(m_Columns) - padding * 2.f,
		1.f / static_cast<_float>(m_Rows) - padding * 2.f
	};
}

void CFlipbookUI::PlayEffect(uint32_t uiState)
{
}
