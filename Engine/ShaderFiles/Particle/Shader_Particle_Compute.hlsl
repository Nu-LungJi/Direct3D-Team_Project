#include "../Particle/Particle_Common_Struct_Func.hlsl"



cbuffer CB_PER_PARTICLE : register(b11)
{
    float g_fTimeDelta;
    uint g_iNumInstances;
    uint g_iFlipbookRows;
    uint g_iFlipbookColumns;
    uint g_iTotalFrames;
    float g_fTime;
    float2 g_fPadding;
};


AppendStructuredBuffer<uint> gDeadList : register(u0);
RWStructuredBuffer<ParticleData> g_ParticleBuffer : register(u1);
 
[numthreads(256, 1, 1)]
void CSMain(uint id : SV_DispatchThreadID)
{
    ParticleData p = g_ParticleBuffer[id];
    if (p.alive == 0)
        return;

    p.life -= g_fTimeDelta;
    if ((p.iBehaviorType & BEHAVIOR_GRAVITY) != 0)
    {
        const float kGravity = -9.8f;
        p.velocity.y += kGravity * g_fTimeDelta; 
    }
    p.position += p.velocity * g_fTimeDelta; 
	float elapsedTime = p.maxLife - p.life;

	float lifeRatio = saturate(elapsedTime / max(p.maxLife, 0.0001f));

	float sizeRatio = lifeRatio;

	if ((p.iBehaviorType & BEHAVIOR_SIZESTOP) != 0)
	{
		if (p.stopSizeTime <= 0.f)
			sizeRatio = 1.f;
		else
			sizeRatio = saturate(elapsedTime / p.stopSizeTime);
	}

	p.size = lerp(p.startSize, p.endSize, sizeRatio);
    
    
	if (g_iTotalFrames > 0)
	{
		uint frame = (uint) (lifeRatio * g_iTotalFrames);
		p.frameIndex = min(frame, g_iTotalFrames - 1);
	}
	else
	{
		p.frameIndex = 0;
	}

    if (p.life <= 0)
    {
        if (p.loop == 1)
        {
            p.life = p.maxLife;
            p.position = p.originalPosition;
            p.velocity = p.originalVelocity;
            p.alive = 1;
            p.emissive = p.originalEmissive;
                

        }
        else
        {
            p.alive = 0;
            p.size = 0;
            p.color = 0;
            p.ownerID = 0;
            p.frameIndex = 0;
            p.originalPosition = 0;
            p.iBehaviorType = 0;
            p.velocity = 0;
            
            gDeadList.Append(id);
        }
    }
    
    g_ParticleBuffer[id] = p;
}
