//=============================================================================
//// Shader uses position and texture
//=============================================================================
SamplerState samPoint
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Mirror;
    AddressV = Mirror;
};

SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;// or Mirror or Clamp or Border
	AddressV = Wrap;// or Mirror or Clamp or Border
};

Texture2D gTexture;

bool gActivated = true;

/// Create Depth Stencil State (ENABLE DEPTH WRITING)
DepthStencilState EnabledDepth
{
	DepthEnable = TRUE;
	DepthWriteMask = ALL;
};
/// Create Rasterizer State (Backface culling) 


RasterizerState BackfaceCulling
{
	FillMode = SOLID;
	CullMode = BACK;
};
//IN/OUT STRUCTS
//--------------
struct VS_INPUT
{
    float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;

};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD1;
};


//VERTEX SHADER
//-------------
PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output = (PS_INPUT)0;
	// Set the Position
	output.Position = float4(input.Position, 1.0f);
	// Set the TexCoord
	output.TexCoord = input.TexCoord;
	return output;
}


//PIXEL SHADER
//------------
float4 PS(PS_INPUT input): SV_Target
{
	float width,height;
	gTexture.GetDimensions(width, height);
	float2 res = float2(width, height);

	
	float2 uv = input.TexCoord;

	float3 TopLeft = gTexture.Sample(samPoint, uv + float2(-1, -1) / res);
	float3 TopMiddle = gTexture.Sample(samPoint, uv + float2(0, -1) / res);
	float3 TopRight = gTexture.Sample(samPoint, uv + float2(1, -1) / res);

	float3 MiddleLeft = gTexture.Sample(samPoint, uv + float2(-1, 0) / res);
	float3 MiddleRight = gTexture.Sample(samPoint, uv + float2(1, 0) / res);

	float3 BottomLeft = gTexture.Sample(samPoint, uv + float2(-1, 1) / res);
	float3 BottomMiddle = gTexture.Sample(samPoint, uv + float2(0, 1) / res);
	float3 BottomRight = gTexture.Sample(samPoint, uv + float2(1, 1) / res);

	float x = TopLeft + 2.0 * MiddleLeft + BottomLeft - TopRight - 2.0 * MiddleRight - BottomRight;
	float y = -TopLeft - 2.0 * TopMiddle - TopRight + BottomLeft + 2.0 * BottomMiddle + BottomRight;
	float edgeIntensity = sqrt((x * x) + (y * y));

	float4 color = gTexture.Sample(samPoint, input.TexCoord);
	
	float4 edgeColor = lerp(
		float4(0.0, 0.0, 0.0, 1.0),
		color,
		1.0 - edgeIntensity);


	
	

	return edgeColor;
}