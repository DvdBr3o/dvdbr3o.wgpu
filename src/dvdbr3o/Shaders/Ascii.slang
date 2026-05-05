import Uniform;
import Vertex;

struct VertexOutput {
    float4 sv_position : SV_Position;
	float2 uv;
};

struct FragmentOutput {
	float4 color : SV_Target0;
	float4 depth_data : SV_Target1;
};

ParameterBlock<Global> global;
ParameterBlock<Camera> camera;
ParameterBlock<PbrMaterial> pbr;

[shader("vertex")]
VertexOutput vertMain(uint vid : SV_VertexID , VertexInput input) {
    VertexOutput output;
    output.sv_position = mul(camera.projection_matrix, mul(camera.view_matrix, float4(input.pos, 1.)));
	output.uv	 = input.uv;
    return output;
}

[shader("fragment")]
FragmentOutput fragMain(VertexOutput in_vert) {
	const float4 albedo = pbr.tex_albedo.Sample(pbr.smp_albedo, in_vert.uv);
	FragmentOutput out_frag;
	out_frag.color = float4(albedo.rgb, 1.0);
	out_frag.depth_data = float4(in_vert.sv_position.z, 0.0, 0.0, 1.0);
	return out_frag;
}
