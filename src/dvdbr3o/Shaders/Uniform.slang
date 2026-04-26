module Uniform;

public struct Viewport {
	public int32_t width;
	public int32_t height;
};

public struct Global {
	public float time;
	public float delta_time;
	public float	frame;
	public Viewport viewport;
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

