struct AsciiConfig_std140_0
{
    @align(16) metrics0_0 : vec4<i32>,
};

@binding(0) @group(0) var<uniform> config_0 : AsciiConfig_std140_0;
@binding(0) @group(1) var<storage, read> cell_data_cells_0 : array<vec4<f32>>;

@binding(0) @group(2) var glyphs_atlas_0 : texture_2d<f32>;

@binding(1) @group(2) var glyphs_atlas_sampler_0 : sampler;

@binding(0) @group(3) var depth_texture_depth_data_0 : texture_2d<f32>;

struct vertexOutput_0
{
    @builtin(position) output_0 : vec4<f32>,
};

@vertex
fn vertMain(@builtin(vertex_index) vid_0 : u32) -> vertexOutput_0
{
    var positions_0 : array<vec2<f32>, i32(3)> = array<vec2<f32>, i32(3)>( vec2<f32>(-1.0f, -3.0f), vec2<f32>(-1.0f, 1.0f), vec2<f32>(3.0f, 1.0f) );
    var _S1 : vertexOutput_0 = vertexOutput_0( vec4<f32>(positions_0[vid_0], 0.0f, 1.0f) );
    return _S1;
}

struct FragmentOutput_0
{
    @location(0) color_0 : vec4<f32>,
    @builtin(frag_depth) depth_0 : f32,
};

@fragment
fn fragMain(@builtin(position) sv_pos_0 : vec4<f32>) -> FragmentOutput_0
{
    var viewport_width_0 : i32 = config_0.metrics0_0.x;
    var viewport_height_0 : i32 = config_0.metrics0_0.y;
    var cell_width_0 : i32 = config_0.metrics0_0.z;
    var cell_height_0 : i32 = config_0.metrics0_0.w;
    var cell_cols_0 : i32 = (viewport_width_0 + cell_width_0 - i32(1)) / cell_width_0;
    var _S2 : i32 = i32(sv_pos_0.x);
    var _S3 : i32 = i32(sv_pos_0.y);
    var pixel_0 : vec2<i32> = vec2<i32>(_S2, _S3);
    var _S4 : bool;
    if(_S2 < i32(0))
    {
        _S4 = true;
    }
    else
    {
        _S4 = _S3 < i32(0);
    }
    if(_S4)
    {
        _S4 = true;
    }
    else
    {
        _S4 = _S2 >= viewport_width_0;
    }
    if(_S4)
    {
        _S4 = true;
    }
    else
    {
        _S4 = _S3 >= viewport_height_0;
    }
    if(_S4)
    {
        discard;
    }
    var _S5 : i32 = _S2 / cell_width_0;
    var _S6 : i32 = _S3 / cell_height_0;
    var cell_info_0 : vec4<f32> = cell_data_cells_0[_S6 * cell_cols_0 + _S5];
    if((cell_info_0.z) < 0.5f)
    {
        discard;
    }
    var _S7 : i32 = _S2 % cell_width_0;
    var _S8 : f32 = f32(_S7) / max(f32(cell_width_0), 1.0f);
    var _S9 : i32 = _S3 % cell_height_0;
    var _S10 : f32 = (textureSample((glyphs_atlas_0), (glyphs_atlas_sampler_0), (vec2<f32>((cell_info_0.x + _S8) / 10.0f, f32(_S9) / max(f32(cell_height_0), 1.0f))))).w;
    if(_S10 < 0.34999999403953552f)
    {
        discard;
    }
    var output_1 : FragmentOutput_0;
    output_1.color_0 = vec4<f32>(vec3<f32>(1.0f), _S10);
    var _S11 : vec3<i32> = vec3<i32>(pixel_0, i32(0));
    output_1.depth_0 = (textureLoad((depth_texture_depth_data_0), ((_S11)).xy, ((_S11)).z)).x;
    return output_1;
}

