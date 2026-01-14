#ifndef _DEFAULT_HLSL_
#define _DEFAULT_HLSL_

#include "Params.hlsli"
#include "LightingUtil.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
    vout.PosW = posW.xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    pin.NormalW = normalize(pin.NormalW);
    
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    
    float4 ambient = gAmbientLight * gAlbedo;
    
    const float shininess = 1.0f - gRoughness;
    
    Material mat = { gAlbedo, gFresnel, shininess };
    
    float4 directLight = ComputeLighting(gLights, gLightCount, mat, pin.PosW, pin.NormalW, toEyeW);
    
    float4 litColor = ambient + directLight;
    
    litColor.a = gAlbedo.a;
    return litColor;
}

#endif