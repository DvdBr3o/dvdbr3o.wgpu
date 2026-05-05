module Light;

interface ILight {

};

struct SpotLight : ILight {
	float3 position;
	float  intensity;
	float4 color;

    
}