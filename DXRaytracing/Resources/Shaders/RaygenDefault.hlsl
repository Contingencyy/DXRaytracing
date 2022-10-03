cbuffer ViewCB : register(b0)
{
	matrix ViewProjection;
	float4 ViewOriginAndTanHalfFovY;
	float2 Resolution;
};

struct DefaultRayPayload
{
	float3 Color;
};

struct BuiltinIntersectAttribs
{
	uint2 Barycentrics;
};

RWTexture2D<float4> output : register(u0);
RaytracingAccelerationStructure SceneBVH : register(t0);

[shader("raygeneration")]
void main()
{
	uint2 currentPixel = DispatchRaysIndex().xy;
	uint2 totalPixels = DispatchRaysDimensions().xy;

	float2 d = ((currentPixel + 0.5f) / Resolution.xy) * 2.0f - 1.0f;
	float aspectRatio = (Resolution.x / Resolution.y);

	float2 pixelCenter = (currentPixel + float2(0.5f, 0.5f)) / totalPixels;
	float2 ndc = float2(2.0f, -2.0f) * pixelCenter + float2(-1.0f, 1.0f);

	float3 camForward = float3(ViewProjection[0][2], ViewProjection[1][2], ViewProjection[2][2]);
	//float3 pixelRayDirection = ndc.x * camForward.x + ndc.y * camForward.y + camForward.z;
	
	RayDesc ray;
	ray.Origin = ViewOriginAndTanHalfFovY.xyz;
	ray.Direction = normalize((d.x * ViewProjection[0].xyz * ViewOriginAndTanHalfFovY.w * aspectRatio) -
		(d.y * ViewProjection[1].xyz * ViewOriginAndTanHalfFovY.w) + ViewProjection[2].xyz);
	//ray.Direction = normalize(float3(ndc.x, ndc.y, 1.0f));
	//ray.Direction = normalize(pixelRayDirection);
	ray.TMin = 0.0f;
	ray.TMax = 1e+38f;

	DefaultRayPayload payload = { float3(0.0f, 0.0f, 0.0f) };
	
	TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF,
		0, 0, 0, ray, payload);

	output[currentPixel] = float4(payload.Color, 1.0f);
}
