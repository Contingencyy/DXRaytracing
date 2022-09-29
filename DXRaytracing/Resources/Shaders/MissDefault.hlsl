struct DefaultRayPayload
{
	float3 Color;
};

[shader("miss")]
void main(inout DefaultRayPayload payload)
{
	payload.Color = float3(1.0f, 0.0f, 1.0f);
}
