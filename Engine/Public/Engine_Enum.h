#pragma once

namespace Engine
{
	enum class WINMODE { FULL, WIN };
	enum class MOUSEKEYSTATE { LB, RB, MB, END };
	enum class MOUSEMOVESTATE { X, Y, Z, END };
	enum class RENDERGROUP { PRIORITY, NONBLEND, BLEND, SKYBOX, COLLIDER,PARTICLE, UI, END };
	enum class RENDERPASS : uint32_t
	{
		DEFAULT = 1 << 0,
		SHADOW = 1 << 1,
	};
	enum class STATE { RIGHT, UP, LOOK, POSITION, END };
	enum class MODEL { NONANIM, ANIM, END };
	//enum class VSYNC{ OFF, ON };
	enum class PARTICLE_TYPE { FIRE,SMOKE,RIBBON, END };
	enum class CPU_GPU{ CPU, GPU, END};

	//나중에 이 성 민 씨 가 옮길거임 접근 금지
	static const uint32_t MAX_SPAWN_PER_CALL = 256;

}