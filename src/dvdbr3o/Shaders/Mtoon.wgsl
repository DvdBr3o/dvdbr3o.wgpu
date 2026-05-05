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

struct Viewport_std140_0
{
    @align(16) width_0 : i32,
    @align(4) height_0 : i32,
};

struct Global_std140_0
{
    @align(16) time_0 : f32,
    @align(4) delta_time_0 : f32,
    @align(8) frame_0 : f32,
    @align(4) _padding0_0 : f32,
    @align(16) viewport_0 : Viewport_std140_0,
    @align(16) _padding1_0 : i32,
    @align(4) _padding2_0 : i32,
    @align(16) point_light_position_intensity_0 : vec4<f32>,
    @align(16) point_light_color_ambient_0 : vec4<f32>,
};

@binding(0) @group(0) var<uniform> global_0 : Global_std140_0;
struct VertexOutput_0
{
    @builtin(position) sv_position_0 : vec4<f32>,
    @location(0) color_0 : vec3<f32>,
    @location(1) uv_0 : vec2<f32>,
    @location(2) normal_0 : vec3<f32>,
    @location(3) world_pos_0 : vec3<f32>,
};

struct vertexInput_0
{
    @location(0) pos_0 : vec3<f32>,
    @location(1) normal_1 : vec3<f32>,
    @location(2) uv_1 : vec2<f32>,
    @location(3) tex_id_0 : u32,
};

@vertex
fn vertMain( _S1 : vertexInput_0, @builtin(vertex_index) vid_0 : u32) -> VertexOutput_0
{
    var output_0 : VertexOutput_0;
    output_0.sv_position_0 = ((((((vec4<f32>(_S1.pos_0, 1.0f)) * (mat4x4<f32>(camera_0.view_matrix_0.data_0[i32(0)][i32(0)], camera_0.view_matrix_0.data_0[i32(1)][i32(0)], camera_0.view_matrix_0.data_0[i32(2)][i32(0)], camera_0.view_matrix_0.data_0[i32(3)][i32(0)], camera_0.view_matrix_0.data_0[i32(0)][i32(1)], camera_0.view_matrix_0.data_0[i32(1)][i32(1)], camera_0.view_matrix_0.data_0[i32(2)][i32(1)], camera_0.view_matrix_0.data_0[i32(3)][i32(1)], camera_0.view_matrix_0.data_0[i32(0)][i32(2)], camera_0.view_matrix_0.data_0[i32(1)][i32(2)], camera_0.view_matrix_0.data_0[i32(2)][i32(2)], camera_0.view_matrix_0.data_0[i32(3)][i32(2)], camera_0.view_matrix_0.data_0[i32(0)][i32(3)], camera_0.view_matrix_0.data_0[i32(1)][i32(3)], camera_0.view_matrix_0.data_0[i32(2)][i32(3)], camera_0.view_matrix_0.data_0[i32(3)][i32(3)]))))) * (mat4x4<f32>(camera_0.projection_matrix_0.data_0[i32(0)][i32(0)], camera_0.projection_matrix_0.data_0[i32(1)][i32(0)], camera_0.projection_matrix_0.data_0[i32(2)][i32(0)], camera_0.projection_matrix_0.data_0[i32(3)][i32(0)], camera_0.projection_matrix_0.data_0[i32(0)][i32(1)], camera_0.projection_matrix_0.data_0[i32(1)][i32(1)], camera_0.projection_matrix_0.data_0[i32(2)][i32(1)], camera_0.projection_matrix_0.data_0[i32(3)][i32(1)], camera_0.projection_matrix_0.data_0[i32(0)][i32(2)], camera_0.projection_matrix_0.data_0[i32(1)][i32(2)], camera_0.projection_matrix_0.data_0[i32(2)][i32(2)], camera_0.projection_matrix_0.data_0[i32(3)][i32(2)], camera_0.projection_matrix_0.data_0[i32(0)][i32(3)], camera_0.projection_matrix_0.data_0[i32(1)][i32(3)], camera_0.projection_matrix_0.data_0[i32(2)][i32(3)], camera_0.projection_matrix_0.data_0[i32(3)][i32(3)]))));
    output_0.color_0 = vec3<f32>(0.40000000596046448f, 0.80000001192092896f, 0.89999997615814209f);
    output_0.uv_0 = _S1.uv_1;
    output_0.normal_0 = normalize(_S1.normal_1);
    output_0.world_pos_0 = _S1.pos_0;
    return output_0;
}

struct pixelOutput_0
{
    @location(0) output_1 : vec4<f32>,
};

struct pixelInput_0
{
    @location(0) color_1 : vec3<f32>,
    @location(1) uv_2 : vec2<f32>,
    @location(2) normal_2 : vec3<f32>,
    @location(3) world_pos_1 : vec3<f32>,
};

@fragment
fn fragMain( _S2 : pixelInput_0, @builtin(position) sv_position_1 : vec4<f32>) -> pixelOutput_0
{
    var albedo_0 : vec3<f32> = (textureSample((pbr_tex_albedo_0), (pbr_smp_albedo_0), (_S2.uv_2))).xyz;
    var light_vec_0 : vec3<f32> = global_0.point_light_position_intensity_0.xyz - _S2.world_pos_1;
    var _S3 : pixelOutput_0 = pixelOutput_0( vec4<f32>(saturate(albedo_0 * vec3<f32>(global_0.point_light_color_ambient_0.w) + mix(albedo_0 * vec3<f32>(0.93999999761581421f), albedo_0 * (vec3<f32>(0.95999997854232788f) + vec3<f32>(0.03999999910593033f) * global_0.point_light_color_ambient_0.xyz), vec3<f32>(smoothstep(-0.07000000774860382f, 0.03000001236796379f, dot(normalize(_S2.normal_2), normalize(light_vec_0))))) * vec3<f32>((0.34999999403953552f + 0.64999997615814209f * saturate(global_0.point_light_position_intensity_0.w / (1.0f + 1.10000002384185791f * max(dot(light_vec_0, light_vec_0), 0.00999999977648258f)))))), 1.0f) );
    return _S3;
}

