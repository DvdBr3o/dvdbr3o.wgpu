import Uniform;
import Vertex;

struct Material {
	Texture2D asciis;
};

struct VertexOutput {
    float4 sv_position : SV_Position;
    float3 color;
	float2 uv;
};

ParameterBlock<Global> global;
ParameterBlock<Camera> camera;
ParameterBlock<Material> material;

[shader("vertex")]
VertexOutput vertMain(uint vid : SV_VertexID , VertexInput input) {
    VertexOutput output;
    output.sv_position = mul(camera.projection_matrix, mul(camera.view_matrix, float4(input.pos, 1.)));
	output.color = float3(.4, .8, .9);
	output.uv	 = input.uv;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput in_vert) : SV_Target {
	return float4(0., 0., 0., 1.);
}
