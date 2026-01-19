#ifndef _TREE_HLSL_
#define _TREE_HLSL_

#include "Params.hlsli"
#include "LightingUtil.hlsl"

Texture2DArray gTexture_Tree : register(t0);

struct VertexIn
{
    float3 PosL : POSITION;
    float2 SizeW : SIZE;
};

struct VertexOut
{
    float3 CenterW : POSITION;
    float2 SizeW : SIZE;
    float4 PosH : SV_Position;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    vout.CenterW = mul(vin.PosL, (float3x3) gWorld);
    vout.SizeW = vin.SizeW;
    vout.PosH = mul(float4(vout.CenterW, 1.0f), gViewProj);

    return vout;
}

struct GeoOut
{
    float4 PosH : SV_Position;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Uv : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};

[maxvertexcount(4)]
void GS( point VertexOut gin[1],
         uint primID : SV_PrimitiveID,
         inout TriangleStream<GeoOut> triStream)
{
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = gEyePosW - gin[0].CenterW;
    look.y = 0.0f;
    look = normalize(look);
    float3 right = cross(up, look);
    
    float halfWidth = 0.5f * gin[0].SizeW.x;
    float halfHeight = 0.5f * gin[0].SizeW.y;
    
    float4 v[4];
    v[0] = float4(gin[0].CenterW + halfWidth * right - halfHeight * up, 1.0f);
    v[1] = float4(gin[0].CenterW + halfWidth * right + halfHeight * up, 1.0f);
    v[2] = float4(gin[0].CenterW - halfWidth * right - halfHeight * up, 1.0f);
    v[3] = float4(gin[0].CenterW - halfWidth * right + halfHeight * up, 1.0f);
    
    float2 texC[4];
    texC[0] = float2(0.0f, 1.0f);
    texC[1] = float2(0.0f, 0.0f);
    texC[2] = float2(1.0f, 1.0f);
    texC[3] = float2(1.0f, 0.0f);

    GeoOut gout;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        gout.PosH = mul(v[i], gViewProj);
        gout.PosW = v[i].xyz;
        gout.NormalW = look;
        gout.Uv = texC[i];
        gout.PrimID = primID;
        
        triStream.Append(gout);
    }
}

float4 PS(GeoOut pin) : SV_Target
{
    float4 diffuseAlbedo = gAlbedo;
    
    float3 uvw = float3(pin.Uv, pin.PrimID % 3);
    diffuseAlbedo = gTexture_Tree.Sample(gSampler, uvw) * gAlbedo;
    
#ifdef ALPHA_TESTED
    clip(diffuseAlbedo.a - 0.1f);
#endif 
    
    pin.NormalW = normalize(pin.NormalW);
    
    float3 bumpedNormalW = pin.NormalW;
    
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    
    float4 ambient = gAmbientLight * diffuseAlbedo;
    
    const float shininess = 1.0f - gRoughness;
    
    Material mat = { diffuseAlbedo, gFresnel, shininess };
    
    float4 directLight = ComputeLighting(gLights, gLightCount, mat, pin.PosW, bumpedNormalW, toEyeW);
    
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