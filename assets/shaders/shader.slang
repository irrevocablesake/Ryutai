struct VSInput {
    uint vertexID : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput main( VSInput input ) {
    VSOutput output;

    float2 position[3] = {
        float2( -1.0, -1.0 ),
        float2( 3.0, -1.0  ),
        float2( -1.0, 3.0 )
    };

    //float2 position[3] = {
        //float2(0, -0.75),
        //float2(0.75, 0.75),
        //float2(-0.75, 0.75)
    //};

    output.position = float4(position[input.vertexID], 0.0, 1.0);
    return output;
}

Sampler2D velocityField;

[shader("fragment")]
float4 main() : SV_Target {
    float3 color = velocityField.Sample(float2(0.5, 0.0)).rgb;

    return float4( color, 1.0);
}