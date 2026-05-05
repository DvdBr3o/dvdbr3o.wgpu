struct AsciiConfig_std140_0
{
    @align(16) metrics0_0 : vec4<i32>,
};

@binding(0) @group(0) var<uniform> config_0 : AsciiConfig_std140_0;
@binding(0) @group(2) var<storage, read_write> output_data_cells_0 : array<vec4<f32>>;

@binding(0) @group(1) var source_data_ascii_lit_0 : texture_2d<f32>;

@compute
@workgroup_size(8, 8, 1)
fn computeMain(@builtin(global_invocation_id) dispatch_id_0 : vec3<u32>)
{
    var cell_x_0 : i32 = i32(dispatch_id_0.x);
    var cell_y_0 : i32 = i32(dispatch_id_0.y);
    var viewport_width_0 : i32 = config_0.metrics0_0.x;
    var viewport_height_0 : i32 = config_0.metrics0_0.y;
    var cell_width_0 : i32 = config_0.metrics0_0.z;
    var cell_height_0 : i32 = config_0.metrics0_0.w;
    var cell_cols_0 : i32 = (viewport_width_0 + cell_width_0 - i32(1)) / cell_width_0;
    var cell_rows_0 : i32 = (viewport_height_0 + cell_height_0 - i32(1)) / cell_height_0;
    var _S1 : bool;
    if(cell_x_0 >= cell_cols_0)
    {
        _S1 = true;
    }
    else
    {
        _S1 = cell_y_0 >= cell_rows_0;
    }
    if(_S1)
    {
        return;
    }
    var start_x_0 : i32 = cell_x_0 * cell_width_0;
    var start_y_0 : i32 = cell_y_0 * cell_height_0;
    var end_x_0 : i32 = min(start_x_0 + cell_width_0, viewport_width_0);
    var end_y_0 : i32 = min(start_y_0 + cell_height_0, viewport_height_0);
    if((end_x_0 - start_x_0) != cell_width_0)
    {
        _S1 = true;
    }
    else
    {
        _S1 = (end_y_0 - start_y_0) != cell_height_0;
    }
    if(_S1)
    {
        output_data_cells_0[cell_y_0 * cell_cols_0 + cell_x_0] = vec4<f32>(0.0f);
        return;
    }
    var y_0 : i32 = start_y_0;
    var brightness_sum_0 : f32 = 0.0f;
    var alpha_sum_0 : f32 = 0.0f;
    var sample_count_0 : i32 = i32(0);
    for(;;)
    {
        if(y_0 < end_y_0)
        {
        }
        else
        {
            break;
        }
        var x_0 : i32 = start_x_0;
        for(;;)
        {
            if(x_0 < end_x_0)
            {
            }
            else
            {
                break;
            }
            var _S2 : vec3<i32> = vec3<i32>(x_0, y_0, i32(0));
            var sample_0 : vec4<f32> = (textureLoad((source_data_ascii_lit_0), ((_S2)).xy, ((_S2)).z));
            var _S3 : f32 = sample_0.w;
            var brightness_sum_1 : f32 = brightness_sum_0 + dot(sample_0.xyz, vec3<f32>(0.2125999927520752f, 0.71520000696182251f, 0.07220000028610229f)) * _S3;
            var alpha_sum_1 : f32 = alpha_sum_0 + _S3;
            var sample_count_1 : i32 = sample_count_0 + i32(1);
            x_0 = x_0 + i32(1);
            brightness_sum_0 = brightness_sum_1;
            alpha_sum_0 = alpha_sum_1;
            sample_count_0 = sample_count_1;
        }
        y_0 = y_0 + i32(1);
    }
    var coverage_0 : f32;
    if(sample_count_0 > i32(0))
    {
        coverage_0 = alpha_sum_0 / f32(sample_count_0);
    }
    else
    {
        coverage_0 = 0.0f;
    }
    if(alpha_sum_0 > 0.00009999999747379f)
    {
        _S1 = coverage_0 > 0.98000001907348633f;
    }
    else
    {
        _S1 = false;
    }
    var brightness_0 : f32;
    var active_0 : f32;
    if(_S1)
    {
        brightness_0 = brightness_sum_0 / alpha_sum_0;
        active_0 = 1.0f;
    }
    else
    {
        brightness_0 = 0.0f;
        active_0 = 0.0f;
    }
    var _S4 : f32 = saturate(brightness_0);
    output_data_cells_0[cell_y_0 * cell_cols_0 + cell_x_0] = vec4<f32>(round(_S4 * 9.0f), _S4, active_0, coverage_0);
    return;
}

