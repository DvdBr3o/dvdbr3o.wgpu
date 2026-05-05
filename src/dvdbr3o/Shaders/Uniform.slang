module Uniform;

public struct Viewport {
	public int32_t width;
	public int32_t height;
};

public struct Global {
	public float time;
	public float delta_time;
	public float	frame;
	public float	_padding0;
	public Viewport viewport;
	public int32_t	_padding1;
	public int32_t	_padding2;
	public float4 point_light_position_intensity;
	public float4 point_light_color_ambient;
}

public struct Camera {
	// public float4x4 model_matrix; // TODO: model matrix
	public float4x4 view_matrix;
	public float4x4 projection_matrix;
}

public struct PbrMaterial {
	public Texture2D	tex_albedo;
	public SamplerState			  smp_albedo;
	// public float4 base_color;
	// public float  metallic;
	// public float  roughness;
	// public float  alpha_cutoff;
}

public struct ModelData {
	public float4x4 model_matrix;
}

