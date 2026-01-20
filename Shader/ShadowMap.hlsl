#ifndef _SHADOWMAP_HLSL_
#define _SHADOWMAP_HLSL_

#include "Params.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_Position;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
    
    return vout;
}

void PS(VertexOut pin)
{
}
#endif