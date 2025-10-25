struct VSInput {
    float2 inPosition;
    float2 uv;
    float3 inColor;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
ConstantBuffer<UniformBuffer> ubo;

struct VSOutput
{
    float4 pos : SV_Position;
    float3 color;
    float2 uv;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.inPosition, 0.0, 1.0))));
    output.color = input.inColor;

    float2 fragScale = float2(ubo.model[0][0], ubo.model[1][1]);
    float2 uvOffset = fragScale *0.5;

    output.uv = uvOffset;


    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {

     float3 c =  float3(0.0,0.0,0.0);

     if(vertIn.pos.x < vertIn.uv.x)
     {
        if(vertIn.pos.y > vertIn.uv.y)
        {
            c= float3(1.0,1.0,1.0);
        }
        else
        {
            c= float3(1.0,1.0,0.0);
        }
     }
     else if(vertIn.pos.y < vertIn.uv.y)
     {
        c= float3(0.0,1.0,1.0);
     }



    return float4(c, 1.0);
}