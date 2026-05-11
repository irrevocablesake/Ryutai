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
    return output;
}

[[vk::binding(0, 0)]]
Sampler2D fieldA;

[[vk::binding(1, 0)]]
Sampler2D fieldB;

static const float dt = 0.6;
static const float texelX = 1.0 / 1920.0f;
static const float texelY = 1.0 / 1200.0f;
static const float2 texelSize = float2(texelX, texelY);

[shader("fragment")]
float2 velocityPass(VSOutput input) : SV_Target {

    float2 uv = input.uv;
    float2 velocity = fieldA.Sample(uv).xy;
    float2 prevUV = uv - dt * ( velocity  ) * texelSize;

    float2 advected = (fieldA.Sample(prevUV).xy);

    return (advected);
}

[shader("fragment")]
float4 densityPass(VSOutput input) : SV_Target {

    float2 uv = input.uv;
    float2 velocity = fieldA.Sample(uv).xy;
    float2 prevUV = uv - dt * (velocity)*texelSize;
    prevUV = clamp(prevUV, 0.0, 1.0);

    float2 advected = (fieldB.Sample(prevUV).xy);

    return float4(advected, 0.0, 1.0);
}

[shader("fragment")]
float divergencePass(VSOutput input) : SV_Target {
    float2 left = fieldA.Sample(input.uv + float2(-texelX, 0.0)).xy;
    float2 right = fieldA.Sample(input.uv + float2(texelX, 0.0)).xy;
    float2 down = fieldA.Sample(input.uv + float2(0.0, -texelY)).xy;
    float2 up = fieldA.Sample(input.uv + float2(0.0, texelY)).xy;

    float divX = (right.x - left.x) ;
    float divY = (up.y - down.y);

    float divergence = (divX + divY);

    return ( divergence);
}

[shader("fragment")]
float pressurePass(VSOutput input) : SV_Target {

    float2 uv = input.uv;

    float left = fieldA.Sample(input.uv + float2(-texelX, 0.0)).x;
    float right = fieldA.Sample(input.uv + float2(texelX, 0.0)).x;
    float down = fieldA.Sample(input.uv + float2(0.0, -texelY)).x;
    float up = fieldA.Sample(input.uv + float2(0.0, texelY)).x;

    float divergence = fieldB.Sample(uv).x;
    float pressure = (left + right + down + up - divergence) * 0.25;

    return ( pressure);
}

[shader("fragment")]
float2 projectionPass(VSOutput input) : SV_Target {

    float2 uv = input.uv;

    float pL = fieldA.Sample(uv - float2(texelX, 0.0)).x;
    float pR = fieldA.Sample(uv + float2(texelX, 0.0)).x;
    float pD = fieldA.Sample(uv - float2(0.0, texelY)).x;
    float pU = fieldA.Sample(uv + float2(0.0, texelY)).x;

    float2 gradP = float2(pR - pL, pU - pD) * 0.5;

    float2 velocity = fieldB.Sample(uv).xy;

    velocity -= gradP;

    return (velocity);
}