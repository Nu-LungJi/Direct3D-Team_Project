#include "pch.h"
#include "BTTurnAnimation.h"
#include "ComAnimator.h" 
NS_USING(Client)

CBTTurnAnimation::CBTTurnAnimation()
{

}
CBTTurnAnimation::CBTTurnAnimation(const CBTTurnAnimation& rhs) : CBTActionNode(rhs)
{

}

CBTTurnAnimation::~CBTTurnAnimation()
{
}
HRESULT CBTTurnAnimation::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::ANIMATION;
	m_MasterName = "BTTurnAnimation";
	return S_OK;
}
HRESULT CBTTurnAnimation::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTTurnAnimation::Evaluate(_float fTimeDelta)
{
	auto pAnimator = (Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));

	auto pSrcTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto& pDestTransform = CGameInstance::Get().GetActiveCamera()->GetTransform();
	if (pAnimator == nullptr)
		return m_eDebug = EVALUATE::FAILED;

	_vector vDestPos = XMLoadFloat3(&pDestTransform.GetPosition());
	_vector vSrcPos = XMLoadFloat3(&pSrcTransform->GetPosition());

	_vector vTargetLook = XMVectorSetY(XMVector3Normalize(vDestPos - vSrcPos),0);
	_vector vSrcLook = XMVectorSetY(XMVector3Normalize(pSrcTransform->GetState(STATE::LOOK) ),0);

	_float fDot = XMVectorGetX(XMVector3Dot(vSrcLook, vTargetLook));
	_float fCrossY = XMVectorGetY(XMVector3Cross(vSrcLook, vTargetLook));

	m_GuiNode.fValue = XMConvertToDegrees(atan2f(fCrossY,fDot));
	
	EVALUATE Resut = EVALUATE::END;
	//- 가 레프트
	// + 가 라이트
	_bool bTurn{ false };
	if (m_GuiNode.fValue > 157.f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::RIGHT_180)];
	}
	else if (m_GuiNode.fValue > 112.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::RIGHT_135)];
	}
	else if (m_GuiNode.fValue > 67.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::RIGHT_90)];
	}
	else if (m_GuiNode.fValue > 22.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::RIGHT_45)];

	}
	else if (m_GuiNode.fValue < -157.f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::LEFT_180)];
	}
	else if(m_GuiNode.fValue < -112.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::LEFT_135)];
	}
	else if (m_GuiNode.fValue < -67.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::LEFT_90)];
	}
	else if (m_GuiNode.fValue < -22.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::LEFT_45)];

	}
	else
	{
		_matrix mat = XMMatrixIdentity();
		_vector vLook = vTargetLook;
		_vector vRight = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), vLook));
		_vector vUp = XMVector3Cross(vLook, vRight);
		mat.r[0] = vRight;
		mat.r[1] = vUp;
		mat.r[2] = vLook;
		XMVECTOR quat = XMQuaternionRotationMatrix(mat);
		pSrcTransform->SetQuaternion(quat);
		return EVALUATE::SUCCESS;
	}
	
	pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop);
	_bool bFinished = pAnimator->GetFinish();

	if (m_bLoop)
		return m_eDebug = EVALUATE::SUCCESS;
	

	if (bFinished)
		return m_eDebug = EVALUATE::SUCCESS;

	return m_eDebug = EVALUATE::RUN;
}
void CBTTurnAnimation::Update_Gui()
{
	ImGui::Text("Angle : %2.f", m_GuiNode.fValue);

	if (ImGui::Button("Loop Change"))
		m_bLoop = !m_bLoop;
	ImGui::Text("Loop : %s", m_bLoop ? "TRUE" : "FALSE");
	
	if (!m_bPopup)
	{
		for (size_t i = 0; i < ETOUI(TURN::END); ++i)
		{
			_string Name = _string("Animation : ") + MagicEnumToStringView(static_cast<TURN>(i)).data();
			if (ImGui::Button(Name.c_str()))
			{
				m_Value.iAnimIndex = i;
				m_bPopup = true;
				break;
			}
		}
	}

	if (m_bPopup && m_Value.iAnimIndex != -1)
	{
		ImGui::Text("Select Animation : "); ImGui::SameLine(150.f);
		ImGui::Text(MagicEnumToStringView(static_cast<TURN>(m_Value.iAnimIndex)).data());

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
		if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::RB))
			m_bPopup = false;
			int32_t iIndex = CGameInstance::Get().GetAnimIndex(m_Handle);
			
			if (-1 != iIndex)
			{
				m_bPopup = false;
				m_iTurnAnimIndex[m_Value.iAnimIndex] = iIndex;
				m_Value.iAnimIndex = -1;
			}

			ImGui::PopStyleColor();
	}

}
nlohmann::json CBTTurnAnimation::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	for (uint32_t i = 0; i < ETOUI(TURN::END); ++i)
	{
		_string Name = "AnimIndex" + std::to_string(i);
		SaveJsonValue(j, Name,m_iTurnAnimIndex[i]);
	}
		
	SaveJsonValue(j, "Loop", m_bLoop);
	return j;
}
HRESULT CBTTurnAnimation::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	for (uint32_t i = 0; i < ETOUI(TURN::END); ++i)
	{
		_string Name = "AnimIndex" + std::to_string(i);
		LoadJsonValue(j, Name, m_iTurnAnimIndex[i]);
	}
	LoadJsonValue(j, "Loop", m_bLoop);
	return S_OK;
}
E::UPtr<CBTTurnAnimation> CBTTurnAnimation::Create()
{
	auto pInstance = E::ToUPtr(new CBTTurnAnimation{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTTurnAnimation");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTTurnAnimation::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTTurnAnimation{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTTurnAnimation");
		return nullptr;
	}

	return pInstance;
}
