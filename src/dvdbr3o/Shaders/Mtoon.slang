module Pipeline;

import Uniform;
import Vertex;

public struct VertexOutput {
    public float4 sv_position : SV_Position;
    public float3 color;
	public float2 uv;
};

public ParameterBlock<Global> global;
public ParameterBlock<Camera> camera;
public ParameterBlock<PbrMaterial> pbr;

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
	return pbr.tex_albedo.Sample(pbr.smp_albedo, in_vert.uv);
}