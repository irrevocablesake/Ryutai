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

Sampler2D velocityField;
Sampler2D densityField;

static const float dt = 0.01;

[shader("fragment")]
float4 velocityPass(VSOutput input) : SV_Target {

    float2 uv = input.uv;
    float2 velocity = velocityField.Sample(uv).xy * 2.0 - 1.0;
    float2 prevUV = uv - dt * ( velocity  );
    prevUV = clamp(prevUV, 0.0, 1.0);

    float2 advected = (velocityField.Sample( prevUV).xy );

    return float4(advected, 0.0, 1.0);
}

[shader("fragment")]
float4 densityPass(VSOutput input) : SV_Target {

    float2 uv = input.uv;
    float2 velocity = velocityField.Sample(uv).xy * 2.0 - 1.0;
    float2 prevUV = uv - dt * (velocity);
    prevUV = clamp(prevUV, 0.0, 1.0);

    float2 advected = (densityField.Sample(prevUV).xy);

    return float4(advected, 0.0, 1.0);
}