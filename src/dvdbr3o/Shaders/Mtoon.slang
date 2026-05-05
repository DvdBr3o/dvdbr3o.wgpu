module Pipeline;

import Uniform;
import Vertex;

public struct VertexOutput {
    public float4 sv_position : SV_Position;
    public float3 color;
	public float2 uv;
	public float3 normal;
	public float3 world_pos;
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
	output.normal = normalize(input.normal);
	output.world_pos = input.pos;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput in_vert) : SV_Target {
	const float3 albedo = pbr.tex_albedo.Sample(pbr.smp_albedo, in_vert.uv).rgb;
	const float3 light_vec = global.point_light_position_intensity.xyz - in_vert.world_pos;
	const float distance_sq = max(dot(light_vec, light_vec), 0.01);
	const float3 light_dir = normalize(light_vec);
	const float ndotl = dot(normalize(in_vert.normal), light_dir);
	const float light_distance_attenuation =
		saturate(global.point_light_position_intensity.w / (1.0 + 1.1 * distance_sq));
	const float3 light_color = global.point_light_color_ambient.rgb;

	const float shading_shift = -0.02;
	const float shading_toony = 0.95;
	const float shade_mix = smoothstep(
		-1.0 + shading_toony + shading_shift,
		1.0 - shading_toony + shading_shift,
		ndotl
	);

	const float3 shade_color = albedo * 0.94;
	const float3 lit_color = albedo * (0.96 + 0.04 * light_color);
	const float3 toon_color = lerp(shade_color, lit_color, shade_mix);
	const float light_blend = 0.35 + 0.65 * light_distance_attenuation;
	const float3 ambient = albedo * global.point_light_color_ambient.a;
	return float4(saturate(ambient + toon_color * light_blend), 1.0);
}
