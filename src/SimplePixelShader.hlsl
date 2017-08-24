struct PixelShaderInput
{
	float4 color: COLOR;
};

float4 ps_main(PixelShaderInput input): SV_TARGET
{
	return input.color;
}