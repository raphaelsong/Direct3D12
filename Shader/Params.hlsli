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
    float4x4 gShadowTransform;
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
Texture2D gTexture_ShadowMap : register(t3);

SamplerState gSampler : register(s0);
SamplerComparisonState gSampler_ShadowMap : register(s1);

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

//------------------------------------------------------------------------------------
// PCF for shadow mapping.
//-----------------------------------------------------------------------------------
float CalcShadowFactor(float4 shadowPosH)
{
	// Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

	// Depth in NDC space.
    float depth = shadowPosH.z;

    uint width, height, numMips;
    gTexture_ShadowMap.GetDimensions(0, width, height, numMips);

	// Texel size.
    float dx = 1.0f / (float) width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

	[unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += gTexture_ShadowMap.SampleCmpLevelZero(gSampler_ShadowMap,
			shadowPosH.xy + offsets[i], depth).r;
    }

    return percentLit / 9.0f;
}
#endif