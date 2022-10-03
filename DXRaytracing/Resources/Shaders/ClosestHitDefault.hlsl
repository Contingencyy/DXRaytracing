struct DefaultRayPayload
{
	float3 Color;
};

struct BuiltinIntersectAttribs
{
	uint2 Barycentrics;
};

[shader("closesthit")]
void main(inout DefaultRayPayload payload, BuiltinIntersectAttribs attribs)
{
	payload.Color = float3(attribs.Barycentrics.x, attribs.Barycentrics.y, 0.0f);
}
