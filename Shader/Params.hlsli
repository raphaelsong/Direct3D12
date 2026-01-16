#ifndef _PARAMS_HLSLI_
#define _PARAMS_HLSLI_

#define MAXLIGHTS 16

struct Material
{
    float4 Albedo;
    float3 Fresnel;
    float Shininess;
};

struct Light
{
    int lightType; // 0: Directional, 1: Point, 2: Spot
    float3 Strength;
    float FalloffStart;
    float3 Position;
    float FalloffEnd;
    float3 Direction;
    float SpotPower;
    float3 lightPadding;
};

cbuffer cbObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTextureTransform;
}

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4 gAmbientLight;
    float3 gEyePosW;
    int gLightCount;
    Light gLights[MAXLIGHTS];
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 gFogPadding;
}

cbuffer cbMaterial : register(b2)
{
    float4 gAlbedo;
    float3 gFresnel;
    float gRoughness;
    int gTexture_On;
    int gNormal_On;
    float2 gTexPadding;
}

Texture2D gTexture_Diffuse : register(t0);
Texture2D gTexture_Normal : register(t1);
TextureCube gCube_Skybox : register(t2);

SamplerState gSampler : register(s0);

// 노말맵 샘플 -> 월드 스페이스
float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
    // [0, 1] -> [-1, 1]
    float3 normalT = 2.0f * normalMapSample - 1.0f;
    
    // TBN Matrix
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);
    
    float3x3 TBN = float3x3(T, B, N);
    
    float3 bumpedNormalW = mul(normalT, TBN);

    return bumpedNormalW;
}
#endif