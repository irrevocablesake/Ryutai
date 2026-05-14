struct VSInput {
    uint vertexID : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

[shader("vertex")]
VSOutput main( VSInput input ) {
    VSOutput output;

    float2 position[3] = {
        float2( -1.0, -1.0 ),
        float2( 3.0, -1.0  ),
        float2( -1.0, 3.0 )
    };

    float2 uvs[3] = {
        float2(0.0, 0.0),
        float2(2.0, 0.0),
        float2(0.0, 2.0)
    };

    output.position = float4(position[input.vertexID], 0.0, 1.0);
     output.uv = uvs[input.vertexID];
    //output.uv = output.position.xy * 0.5 + 0.5;
    return output;
}

[[vk::binding(0, 0)]]
Sampler2D fieldA;

[[vk::binding(0, 1)]]
Sampler2D fieldB;

static float dt = 0.016;
static float2 texelSize = float2(1.0 / 1920.0, 1.0 / 1200.0);

struct SplatData {
    float2 point;
    float padding[2];
    float3 color;
    float radius;
};

[[vk::push_constant]]
SplatData data;

[shader("fragment")]
float4 splatShader(VSOutput input) : SV_Target {
    float3 color = data.color;
    float2 point = data.point;
    float radius = data.radius;
    float aspectRatio = 1.6;

    float2 p = input.uv - point.xy;
    p.x *= aspectRatio;

    float3 splat = exp(-dot(p, p) / radius) * color;
    float3 base = fieldA.Sample( input.uv ).xyz;

    return float4( base + splat, 1.0 );
}

[shader("fragment")]
float4 diverganceShader(VSOutput input) : SV_Target {

    float2 vL = input.uv + float2(-texelSize.x, 0.0);
    float2 vR = input.uv + float2(texelSize.x, 0.0);
    float2 vT = input.uv + float2(0.0, texelSize.y);
    float2 vB = input.uv + float2(0.0, -texelSize.y);

    float L = fieldA.Sample(vL).x;
    float R = fieldA.Sample(vR).x;
    float T = fieldA.Sample(vT).y;
    float B = fieldA.Sample(vB).y;

    float2 C = fieldA.Sample(input.uv).xy;
    if (vL.x < 0.0) { L = -C.x; }
    if (vR.x > 1.0) { R = -C.x; }
    if (vT.y > 1.0) { T = -C.y; }
    if (vB.y < 0.0) { B = -C.y; }

    float div = 0.5 * (R - L + T - B);
    return float4(div, 0.0, 0.0, 1.0);
}

[shader("fragment")]
float4 pressureShader(VSOutput input) : SV_Target {

    float2 vL = input.uv + float2(-texelSize.x, 0.0);
    float2 vR = input.uv + float2(texelSize.x, 0.0);
    float2 vT = input.uv + float2(0.0, texelSize.y);
    float2 vB = input.uv + float2(0.0, -texelSize.y);

    float L = fieldA.Sample(vL).x;
    float R = fieldA.Sample(vR).x;
    float T = fieldA.Sample(vT).x;
    float B = fieldA.Sample(vB).x;

    float C = fieldA.Sample(input.uv).x;

    float divergence = fieldB.Sample(input.uv).x;
    float pressure = (L + R + B + T - divergence) * 0.25;

    return float4( pressure, 0.0, 0.0, 1.0 );
}

[shader("fragment")]
float4 gradientShader(VSOutput input) : SV_Target {

    float2 vL = input.uv + float2(-texelSize.x, 0.0);
    float2 vR = input.uv + float2(texelSize.x, 0.0);
    float2 vT = input.uv + float2(0.0, texelSize.y);
    float2 vB = input.uv + float2(0.0, -texelSize.y);

    float L = fieldA.Sample(vL).x;
    float R = fieldA.Sample(vR).x;
    float T = fieldA.Sample(vT).x;
    float B = fieldA.Sample(vB).x;

    float2 velocity = fieldB.Sample(input.uv).xy;
    velocity.xy -= float2(R - L, T - B);

    return float4( velocity, 0.0, 1.0 );
}

[shader("fragment")]
float4 advectionShader(VSOutput input) : SV_Target {

    float2 coord = input.uv - dt * fieldA.Sample(input.uv).xy;
    float4 result = fieldB.Sample(coord);

    float decay = 1.0 + 0.01 * dt;

    float4 value = (result / decay);

    return value;
}

[shader("fragment")]
float4 displayShader(VSOutput input) : SV_Target {
    float3 c = fieldA.Sample(input.uv).xyz;
    float a = fmax(c.x, max(c.y, c.z));

    return float4( c,a );
}