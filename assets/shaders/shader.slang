struct VSInput {
    uint vertexID : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

[shader("vertex")]
VSOutput main( VSInput input ) {
    VSOutput output;

    //float2 position[3] = {
        //float2( -1.0, -1.0 ),
        //float2( 3.0, -1.0  ),
        //float2( -1.0, 3.0 )
    //};

    float2 position[3] = {
        float2(0, -0.75),
        float2(0.75, 0.75),
        float2(-0.75, 0.75)
    };

    output.position = float4(position[input.vertexID], 0.0, 1.0);
    return output;
}

Sampler2D velocityFieldA;
Sampler2D velocityFieldB;

[shader("fragment")]
float4 main() : SV_Target {
    // float3 color = float3( 1.0, 0.0, 0.0 );
    float3 colorA = velocityFieldA.Sample(float2(0.0, 0.0)).rgb;
    float3 colorB = velocityFieldB.Sample(float2(0.0, 0.0)).rgb;

    float3 color = lerp( colorA, colorB, 0.5 );

    return float4( color, 1.0);
}