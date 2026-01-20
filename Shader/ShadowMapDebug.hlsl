#ifndef _SHADOWMAPDEBUG_HLSL_
#define _SHADOWMAPDEBUG_HLSL_

#include "Params.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
    float2 Uv : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float2 Uv : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.PosH = float4(vin.PosL, 1.0f);
    vout.Uv = vin.Uv;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(gTexture_ShadowMap.Sample(gSampler, pin.Uv).rrr, 1.0f);
}
#endif
