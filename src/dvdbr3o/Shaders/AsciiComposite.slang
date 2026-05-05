struct AsciiConfig {
	int4 metrics0;
};

struct AsciiCells {
	StructuredBuffer<float4> cells;
};

ParameterBlock<AsciiConfig> config;
ParameterBlock<AsciiCells> cell_data;

struct GlyphAtlas {
	Texture2D atlas;
	SamplerState atlas_sampler;
};
ParameterBlock<GlyphAtlas> glyphs;

struct DepthTexture {
	Texture2D depth_data;
};
ParameterBlock<DepthTexture> depth_texture;

struct FragmentOutput {
	float4 color : SV_Target;
	float depth : SV_Depth;
};

[shader("vertex")]
float4 vertMain(uint vid : SV_VertexID) : SV_Position {
	float2 positions[3] = {
		float2(-1.0, -3.0),
		float2(-1.0, 1.0),
		float2(3.0, 1.0),
	};
	return float4(positions[vid], 0.0, 1.0);
}

[shader("fragment")]
FragmentOutput fragMain(float4 sv_pos : SV_Position) {
	const int viewport_width = config.metrics0.x;
	const int viewport_height = config.metrics0.y;
	const int cell_width = config.metrics0.z;
	const int cell_height = config.metrics0.w;
	const int cell_cols = (viewport_width + cell_width - 1) / cell_width;

	const int2 pixel = int2(int(sv_pos.x), int(sv_pos.y));
	if (pixel.x < 0 || pixel.y < 0 || pixel.x >= viewport_width || pixel.y >= viewport_height)
		discard;

	const int2 cell = int2(pixel.x / cell_width, pixel.y / cell_height);
	const float4 cell_info = cell_data.cells[cell.y * cell_cols + cell.x];
	if (cell_info.z < 0.5)
		discard;

	const float2 local_uv = float2(
		float(pixel.x % cell_width) / max(float(cell_width), 1.0),
		float(pixel.y % cell_height) / max(float(cell_height), 1.0)
	);
	const float glyph_index = cell_info.x;
	const int glyph_count = 10;
	const float2 atlas_uv = float2(
		(glyph_index + local_uv.x) / glyph_count,
		local_uv.y
	);
	const float4 glyph = glyphs.atlas.Sample(glyphs.atlas_sampler, atlas_uv);
	if (glyph.a < 0.35)
		discard;

	FragmentOutput output;
	output.color = float4(1.0.xxx, glyph.a);
	output.depth = depth_texture.depth_data.Load(int3(pixel, 0)).r;
	return output;
}
