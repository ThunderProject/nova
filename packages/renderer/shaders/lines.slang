struct LineInstance {
    float3 start;
    float width;
    float3 end;
    uint color;
}

struct LineConstants {
    float viewPortWidth;
    float viewPortHeight;

    uint lineCount;
    float aaRadius;
}

[[vk::binding(0, 0)]]
StructuredBuffer<LineInstance> gLines;

[[vk::push_constant]]
ConstantBuffer<LineConstants> gConstants;

struct MeshVertex {
    float4 position : SV_Position;

    noperspective float2 local : TEXCOORD0;

    nointerpolation float length : TEXCOORD1;
    nointerpolation float halfWidth : TEXCOORD2;
    nointerpolation float4 color : COLOR0;
};

struct MeshPrimitive
{
};

float4 unpackColor(uint packed) {
    float4 color;

    color.r = float((packed >> 0) & 0xffu);
    color.g = float((packed >> 8) & 0xffu);
    color.b = float((packed >> 16) & 0xffu);
    color.a = float((packed >> 24) & 0xffu);

    return color / 255.0;
}

float3 srgbToLinear(float3 v) {
    float3 lo = v / 12.92;
    float3 hi = pow((v + 0.055) / 1.055, 2.4);

    return select(v <= 0.04045, lo, hi);
}

[shader("mesh")]
[numthreads(32,1,1)]
[outputtopology("triangle")]
void line_mesh(
    uint lane : SV_GroupThreadID,
    uint group : SV_GroupID,
    out vertices MeshVertex vertices[128],
    out indices uint3 triangles[64]
) {
    uint lineIndex = group * 32u + lane;
    uint firstLine = group * 32u;
    uint remaining = 0u;

    if(firstLine < gConstants.lineCount) {
        remaining = gConstants.lineCount - firstLine;
    }

    uint groupLineCount = min(remaining, 32u);
    SetMeshOutputCounts(groupLineCount * 4u, groupLineCount * 2u);

    if(lane >= groupLineCount) {
        return;
    }

    LineInstance line = gLines[lineIndex];
    float2 viewport = float2(gConstants.viewPortWidth, gConstants.viewPortHeight);

    float2 startNdc = line.start.xy;
    float2 endNdc = line.end.xy;

    float2 startPx = (startNdc * 0.5 + 0.5) * viewport;
    float2 endPx = (endNdc * 0.5 + 0.5) * viewport;

    float2 delta = endPx - startPx;

    float lengthPx = length(delta);

    float2 tangent;

    if(lengthPx > 1e-5) {
        tangent = delta / lengthPx;
    }
    else {
        tangent = float2(1.0, 0.0);
    }

    float2 normal = float2(-tangent.y, tangent.x);
    float halfWidth = max(line.width * 0.5, 0.0);

    float expansion = halfWidth + gConstants.aaRadius;

    float2 p0 = startPx - tangent * expansion - normal * expansion;
    float2 p1 = startPx - tangent * expansion + normal * expansion;

    float2 p2 = endPx + tangent * expansion - normal * expansion;
    float2 p3 = endPx + tangent * expansion + normal * expansion;

    float2 ndcScale = 2.0 / viewport;

    p0 = p0 * ndcScale - 1.0;
    p1 = p1 * ndcScale - 1.0;
    p2 = p2 * ndcScale - 1.0;
    p3 = p3 * ndcScale - 1.0;

    float4 color = unpackColor(line.color);

    color.rgb = srgbToLinear(color.rgb);

    uint vertexBase = lane * 4u;
    uint primitiveBase = lane * 2u;

    MeshVertex output;

    output.length = lengthPx;
    output.halfWidth = halfWidth;
    output.color = color;
    output.position = float4(p0, line.start.z, 1.0);
    output.local = float2(-expansion, -expansion);

    vertices[vertexBase + 0u] = output;

    output.position = float4(p1, line.start.z, 1.0);
    output.local = float2(-expansion, expansion);

    vertices[vertexBase + 1u] = output;

    output.position = float4(p2, line.end.z, 1.0);
    output.local = float2(lengthPx + expansion, -expansion);

    vertices[vertexBase + 2u] = output;

    output.position = float4(p3, line.end.z, 1.0);
    output.local = float2(lengthPx + expansion, expansion);

    vertices[vertexBase + 3u] = output;

    triangles[primitiveBase + 0u] = uint3(
        vertexBase + 0u,
        vertexBase + 1u,
        vertexBase + 2u
    );

    triangles[primitiveBase + 1u] = uint3(
        vertexBase + 2u,
        vertexBase + 1u,
        vertexBase + 3u
    );
}

[shader("fragment")]
float4 line_fragment(MeshVertex input) : SV_Target0 
{
    float x = clamp(input.local.x, 0.0, input.length);
    float2 delta = float2(input.local.x - x, input.local.y);

    float signedDistance = length(delta) - input.halfWidth;
    float derivative = max(fwidth(signedDistance), 1e-4);
    float coverage = saturate(0.5 - signedDistance / derivative);

    float alpha = input.color.a * coverage;

    return float4(input.color.rgb * alpha, alpha);
}
