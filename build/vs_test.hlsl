#pragma pack_matrix( row_major )

SamplerState s1 : register(s1);
Texture2D t2 : register(t1);

struct ModelViewProjection
{
    matrix MVP;
};

struct ClipMapData
{
    float tex_dim;//x and y
    float level;
    float grid_offset;//0 - 15 4 x 4 grid 
    float dummy;
    //level of detail of the clipmap //used to sample at higher rates at the closer level of details ///means we need to have the outer level be Zero and inner level the maximum.
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);
ConstantBuffer<ModelViewProjection> WorldProjectionCB : register(b1);
ConstantBuffer<ClipMapData> ClipMapDataCB : register(b2);

struct VertexPosColor
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float2 UV : TEXCOORD;
};
 
struct VertexShaderOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 UV : TEXCOORD;    
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

//float4 world_p = mul(float4(IN.Position,1.0f),ModelViewProjectionCB.MVP);
float4 world_p = mul(float4(IN.Position,1.0f),WorldProjectionCB.MVP);
OUT.Position = world_p;
OUT.Color = IN.Color;
OUT.UV = IN.UV;
    return OUT;
}