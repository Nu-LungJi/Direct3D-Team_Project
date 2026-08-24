#pragma once
#include "Client_Defines.h"

NS_BEGIN(Client)

namespace EDG_KEY
{
	inline const StringID STATE{ "EdgState" };
	inline const StringID BSTATE_FINISHED{ "StateFinished" };
	inline const StringID EDGPHASE{ "EdgPhase" };

	inline const StringID EDGEFFECT{ "Effect" };
	inline const StringID EPATROL{ "EdgPatrol" };
	inline const StringID LPATROL{ "EdgLeftPatrol" };
	inline const StringID RPATROL{ "EdgRightPatrol" };
}
namespace PUBLIC_KEY
{
	inline const StringID TARGETHANDLE { "TargetHandle" };
}
namespace NPC_KEY
{
	inline const StringID STARTPOS{ "PatrollStat" };
	inline const StringID ENDPOS{ "PatrollEnd" };
	inline const StringID ANIMINDEX{ "AnimIndex" };
	inline const StringID SPEED{ "Speed" };
	inline const StringID STATE{ "NpcState" };
}
NS_END
