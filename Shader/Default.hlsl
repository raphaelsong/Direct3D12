#ifndef _DEFAULT_HLSL_
#define _DEFAULT_HLSL_

#include "Params.hlsli"
#include "LightingUtil.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Uv : TEXCOORD;
    float3 TangentU : TANGENT;
#ifdef SKINNED
    float3 BoneWeights : WEIGHTS;
    uint4 BoneIndices : BONEINDICES;
#endif
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float4 ShadowPosH : POSITION0;
    float3 PosW : POSITION1;
    float3 NormalW : NORMAL;
    float2 Uv : TEXCOORD;
    float3 TangentW : TANGENT;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
#ifdef SKINNED
    float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    weights[0] = vin.BoneWeights.x;
    weights[1] = vin.BoneWeights.y;
    weights[2] = vin.BoneWeights.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];
    
    float3 posL = float3(0.0f, 0.0f, 0.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);
    float3 tangentL = float3(0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < 4; ++i)
    {
        posL += weights[i] * mul(float4(vin.PosL, 1.0f), gBoneTransform[vin.BoneIndices[i]]).xyz;
        normalL += weights[i] * mul(float4(vin.NormalL, 0.0f), gBoneTransform[vin.BoneIndices[i]]).xyz;
        tangentL += weights[i] * mul(float4(vin.TangentU, 0.0f), gBoneTransform[vin.BoneIndices[i]]).xyz;
    }
    
    vin.PosL = posL;
    vin.NormalL = normalL;
    vin.TangentU = tangentL;
#endif
    
        float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
    vout.PosW = posW.xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    
    float4 Uv = mul(float4(vin.Uv, 0.0f, 1.0f), gTextureTransform);
    vout.Uv = Uv.xy;
    
    vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    
    vout.ShadowPosH = mul(posW, gShadowTransform);
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 diffuseAlbedo = gAlbedo;
    
    if(gTexture_On)
    {
        diffuseAlbedo = gTexture_Diffuse.Sample(gSampler, pin.Uv) * gAlbedo;
    }
    
#ifdef ALPHA_TESTED
    clip(diffuseAlbedo.a - 0.1f);
#endif 
    
    pin.NormalW = normalize(pin.NormalW);
    
    float4 normalMapSample;
    float3 bumpedNormalW = pin.NormalW;
    if(gNormal_On)
    {
        normalMapSample = gTexture_Normal.Sample(gSampler, pin.Uv);
        bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
    }    
    
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    
    float4 ambient = gAmbientLight * diffuseAlbedo;
    
    const float shininess = 1.0f - gRoughness;
    
    Material mat = { diffuseAlbedo, gFresnel, shininess };
    
    // Shadow Map
    float shadowFactor = 1.0f;
    shadowFactor = CalcShadowFactor(pin.ShadowPosH);
    
    float4 directLight = ComputeLighting(gLights, gLightCount, mat, pin.PosW, bumpedNormalW, toEyeW, shadowFactor);
    
    float4 litColor = ambient + directLight;
    
    float3 r = reflect(-toEyeW, pin.NormalW);
    float4 rColor = gCube_Skybox.Sample(gSampler, r);
    float3 fresnalFactor = SchlickFresnel(gFresnel, pin.NormalW, r);
    litColor.rgb += shininess * fresnalFactor * rColor.rgb;
    
#ifdef FOG
    float distToEye = length(gEyePosW - pin.PosW);
    float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
    litColor = lerp(litColor, gFogColor, fogAmount);
#endif 
    
    litColor.a = diffuseAlbedo.a;
    return litColor;
}

#endif