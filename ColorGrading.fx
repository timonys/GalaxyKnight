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
Texture2D gLUT;

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
	float4 pixel = gTexture.Sample(samLinear,input.TexCoord);

	float cell = pixel.b * 15.0f;

	float cell_l = floor(cell);
	float cell_h = ceil(cell);

	float half_pixel_x = 0.5 / 256.0f;
	float half_pixel_y = 0.5 / 16.0;

	float x_offset = half_pixel_x + pixel.r / 16.0f * (15.0f / 16.0f);
	float y_offset = half_pixel_y + pixel.g * (15.0f / 16.0f);

	float2 LUT_position_l = float2(cell_l / 16.0f + x_offset, y_offset);
	float2 LUT_position_h = float2(cell_h / 16.0f + x_offset, y_offset);
	
	float4 color_graded_l = gLUT.Sample(samLinear, LUT_position_l);
	float4 color_graded_h = gLUT.Sample(samLinear, LUT_position_h);

	float4 color = lerp(color_graded_l, color_graded_h, frac(cell));

	return color;
}