
Texture2D t1 : register(t0);
Texture2D t2 : register(t1);

SamplerState s1 : register(s0);

struct PixelShaderInput
{
    float4 Position : SV_Position;
	float2 UV : TEXCOORD;
    float4 h : COLOR;
};

float4 main( PixelShaderInput IN ) : SV_Target
{
   float4 color = float4(1,1,1,1);
   return color;
}