cbuffer ViewCB : register(b0)
{
	matrix ViewProjAtOrigin;
	float4 ViewOriginAndTanHalfFovY;
	float2 Resolution;
};

struct DefaultRayPayload
{
	float3 Color;
};

RWTexture2D<float4> output : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

[shader("raygeneration")]
void main()
{
	uint2 pixelIndex = DispatchRaysIndex().xy;
	float2 xy = pixelIndex + 0.5f;
	float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0f - 1.0f;

	screenPos.y = -screenPos.y;

	float4 world = mul(float4(screenPos, 0.0f, 1.0f), ViewProjAtOrigin);
	float aspectRatio = Resolution.x / Resolution.y;

	RayDesc ray;
	ray.Origin = ViewOriginAndTanHalfFovY.xyz;
	//ray.Direction = normalize(world.xyz);
	ray.Direction = normalize(float3(world.x * aspectRatio, world.y, world.z));
	ray.TMin = 0.0f;
	ray.TMax = 1e+38f;

	DefaultRayPayload payload = { float3(0.0f, 0.0f, 0.0f) };
	
	TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF,
		0, 0, 0, ray, payload);
	
	output[pixelIndex] = float4(payload.Color, 1.0f);

	//output[pixelIndex] = world;
	//output[pixelIndex] = float4(pixelIndex.x / Resolution.x, pixelIndex.y / Resolution.y, 0.0f, 1.0f);
	//output[pixelIndex] = float4(abs(screenPos.xy), 0.0f, 1.0f);
	//output[pixelIndex] = float4(abs(ray.Direction), 1.0f);
}
