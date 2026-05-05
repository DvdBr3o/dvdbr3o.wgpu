import Uniform;

struct AsciiConfig {
	int4 metrics0;
};

struct AsciiSource {
	Texture2D ascii_lit;
};

struct AsciiCells {
	RWStructuredBuffer<float4> cells;
};

ParameterBlock<AsciiConfig> config;
ParameterBlock<AsciiSource> source_data;
ParameterBlock<AsciiCells> output_data;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 dispatch_id : SV_DispatchThreadID) {
	const int cell_x = int(dispatch_id.x);
	const int cell_y = int(dispatch_id.y);
	const int viewport_width = config.metrics0.x;
	const int viewport_height = config.metrics0.y;
	const int cell_width = config.metrics0.z;
	const int cell_height = config.metrics0.w;
	const int cell_cols = (viewport_width + cell_width - 1) / cell_width;
	const int cell_rows = (viewport_height + cell_height - 1) / cell_height;
	if (cell_x >= cell_cols || cell_y >= cell_rows)
		return;

	float brightness_sum = 0.0;
	float alpha_sum = 0.0;
	int sample_count = 0;

	const int start_x = cell_x * cell_width;
	const int start_y = cell_y * cell_height;
	const int end_x = min(start_x + cell_width, viewport_width);
	const int end_y = min(start_y + cell_height, viewport_height);
	if (end_x - start_x != cell_width || end_y - start_y != cell_height) {
		output_data.cells[cell_y * cell_cols + cell_x] = 0.0.xxxx;
		return;
	}

	for (int y = start_y; y < end_y; ++y) {
		for (int x = start_x; x < end_x; ++x) {
			const float4 sample = source_data.ascii_lit.Load(int3(x, y, 0));
			const float brightness = dot(sample.rgb, float3(0.2126, 0.7152, 0.0722));
			brightness_sum += brightness * sample.a;
			alpha_sum += sample.a;
			sample_count += 1;
		}
	}

	float brightness = 0.0;
	float active = 0.0;
	const float coverage = sample_count > 0 ? alpha_sum / sample_count : 0.0;
	if (alpha_sum > 0.0001 && coverage > 0.98) {
		brightness = brightness_sum / alpha_sum;
		active = 1.0;
	}

	const int glyph_count = 10;
	const float glyph_index = round(saturate(brightness) * (glyph_count - 1));
	output_data.cells[cell_y * cell_cols + cell_x] =
		float4(glyph_index, saturate(brightness), active, coverage);
}
