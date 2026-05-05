struct _MatrixStorage_float4x4_ColMajorstd140_0
{
    @align(16) data_0 : array<vec4<f32>, i32(4)>,
};

struct Camera_std140_0
{
    @align(16) view_matrix_0 : _MatrixStorage_float4x4_ColMajorstd140_0,
    @align(16) projection_matrix_0 : _MatrixStorage_float4x4_ColMajorstd140_0,
};

@binding(0) @group(1) var<uniform> camera_0 : Camera_std140_0;
@binding(0) @group(2) var pbr_tex_albedo_0 : texture_2d<f32>;

@binding(1) @group(2) var pbr_smp_albedo_0 : sampler;

struct VertexOutput_0
{
    @builtin(position) sv_position_0 : vec4<f32>,
    @location(0) uv_0 : vec2<f32>,
};

struct vertexInput_0
{
    @location(0) pos_0 : vec3<f32>,
    @location(1) normal_0 : vec3<f32>,
    @location(2) uv_1 : vec2<f32>,
    @location(3) tex_id_0 : u32,
};

@vertex
fn vertMain( _S1 : vertexInput_0, @builtin(vertex_index) vid_0 : u32) -> VertexOutput_0
{
    var output_0 : VertexOutput_0;
    output_0.sv_position_0 = ((((((vec4<f32>(_S1.pos_0, 1.0f)) * (mat4x4<f32>(camera_0.view_matrix_0.data_0[i32(0)][i32(0)], camera_0.view_matrix_0.data_0[i32(1)][i32(0)], camera_0.view_matrix_0.data_0[i32(2)][i32(0)], camera_0.view_matrix_0.data_0[i32(3)][i32(0)], camera_0.view_matrix_0.data_0[i32(0)][i32(1)], camera_0.view_matrix_0.data_0[i32(1)][i32(1)], camera_0.view_matrix_0.data_0[i32(2)][i32(1)], camera_0.view_matrix_0.data_0[i32(3)][i32(1)], camera_0.view_matrix_0.data_0[i32(0)][i32(2)], camera_0.view_matrix_0.data_0[i32(1)][i32(2)], camera_0.view_matrix_0.data_0[i32(2)][i32(2)], camera_0.view_matrix_0.data_0[i32(3)][i32(2)], camera_0.view_matrix_0.data_0[i32(0)][i32(3)], camera_0.view_matrix_0.data_0[i32(1)][i32(3)], camera_0.view_matrix_0.data_0[i32(2)][i32(3)], camera_0.view_matrix_0.data_0[i32(3)][i32(3)]))))) * (mat4x4<f32>(camera_0.projection_matrix_0.data_0[i32(0)][i32(0)], camera_0.projection_matrix_0.data_0[i32(1)][i32(0)], camera_0.projection_matrix_0.data_0[i32(2)][i32(0)], camera_0.projection_matrix_0.data_0[i32(3)][i32(0)], camera_0.projection_matrix_0.data_0[i32(0)][i32(1)], camera_0.projection_matrix_0.data_0[i32(1)][i32(1)], camera_0.projection_matrix_0.data_0[i32(2)][i32(1)], camera_0.projection_matrix_0.data_0[i32(3)][i32(1)], camera_0.projection_matrix_0.data_0[i32(0)][i32(2)], camera_0.projection_matrix_0.data_0[i32(1)][i32(2)], camera_0.projection_matrix_0.data_0[i32(2)][i32(2)], camera_0.projection_matrix_0.data_0[i32(3)][i32(2)], camera_0.projection_matrix_0.data_0[i32(0)][i32(3)], camera_0.projection_matrix_0.data_0[i32(1)][i32(3)], camera_0.projection_matrix_0.data_0[i32(2)][i32(3)], camera_0.projection_matrix_0.data_0[i32(3)][i32(3)]))));
    output_0.uv_0 = _S1.uv_1;
    return output_0;
}

struct FragmentOutput_0
{
    @location(0) color_0 : vec4<f32>,
    @location(1) depth_data_0 : vec4<f32>,
};

struct pixelInput_0
{
    @location(0) uv_2 : vec2<f32>,
};

@fragment
fn fragMain( _S2 : pixelInput_0, @builtin(position) sv_position_1 : vec4<f32>) -> FragmentOutput_0
{
    var out_frag_0 : FragmentOutput_0;
    out_frag_0.color_0 = vec4<f32>((textureSample((pbr_tex_albedo_0), (pbr_smp_albedo_0), (_S2.uv_2))).xyz, 1.0f);
    out_frag_0.depth_data_0 = vec4<f32>(sv_position_1.z, 0.0f, 0.0f, 1.0f);
    return out_frag_0;
}

