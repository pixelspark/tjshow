/** TJShow shader effect (C) Tommy van der Vorst, 2008-2009
This file is part of the TJShow software and is covered by copyright and license regulations. 
Please do not modify this file, unless you know what you are doing!
**/
int gp : SasGlobal < int3 SasVersion = {1,0,0}; >;

texture SourceTexture;
const float Gamma = 0.5f;
const float Alpha = 0.5f;
const float Blur = 0.0f;
const float Vignet = 0.0f;
const float Mosaic = 0.0f;
const float Saturation = 1.0f;
const float3 KeyingColor = {1.0f, 1.0f, 1.0f};
const float KeyingTolerance = 1.0f;
const bool KeyingEnabled = true;

sampler2D SourceColor = sampler_state {
    Texture = <SourceTexture>;
    AddressU = Border;
    AddressV = Border;
    MinFilter = Point;
    MagFilter = Anisotropic;
    MipFilter = Linear;
};

void GammaAdjust(inout float4 color) {
	color = pow(color, abs((Gamma*4.0f) + 1.0f));
}

void Mosaicing(inout float2 tex) {
	// The 10 limits the minimum amount of blocks on the screen to 10*10
	float d = Mosaic / 10;
	float c = d / 2;
	tex.x = tex.x - fmod(tex.x, d) + c;
	tex.y = tex.y - fmod(tex.y, d) + c;
}

void Keying(inout float4 color) {
    float3 pc = {color.r, color.g, color.b};
    float dist = pow(distance(pc, KeyingColor),2);
    if(dist < KeyingTolerance) {
        color.a = dist;
    }
}

void Vignetting(float2 tex, inout float4 color) {
	float2 center = {0.3f, 0.3f};
	color.a *= 1.0f - min(1.0f, (Vignet*distance(center, tex)*5.0f));
}

void Saturizing(inout float4 color) {
	float sat = (color.r + color.g + color.b)/3.0f;
	color.r = lerp(sat, color.r, Saturation);
	color.g = lerp(sat, color.g, Saturation);
	color.b = lerp(sat, color.b, Saturation);
}

float4 PostProcessor(float2 tex : TEXCOORD0): COLOR0 {   	
	// All methods that change the tx/ty coordinates go first
	if(Mosaic>0.0f) {
		Mosaicing(tex);
	}
	
	float4 color = tex2D(SourceColor, tex);
	
	// Color modification methods
	if(KeyingEnabled) {
		Keying(color);
	}
	
	if(Vignet > 0.0f) {
		Vignetting(tex, color);
	}
	
	if(Gamma>0.0f) {
		GammaAdjust(color);
	}
	
	if(Saturation < 1.0f) {
		Saturizing(color);
	}
	
	color.w *= Alpha;
	return color;
}

static const int BlurKernelSize = 8;
static const int BlurPasses = 5;

// Should sum to 1.0
static const float BlurWeights[BlurPasses] = {
    0.4f,
    0.3f,
    0.1f,
    0.1f,
    0.1f,
};

static const float2 BlurSamples[BlurKernelSize] = {
	{-0.707,-0.707},
	{0, -1},
	{0.707, -0.707},
	{1, 0},
	{0.707, 0.707},
	{0, 1},
	{-0.707, 0.707},
	{-1, 0},
};

float4 BlurPostProcessor(float2 tex :TEXCOORD0) :COLOR0 {
	float4 Color = 0;
	float BlurScale = Blur / 10.0f;
	static const float ColorScale = BlurKernelSize*BlurPasses;

	for(int p = 0; p < BlurPasses; p++) {
		float fraction = BlurWeights[p];
		
		for(int i = 0; i < BlurKernelSize; i++) { 
			float2 coord = tex.xy + (BlurSamples[i] * BlurScale * fraction);
			Color += tex2D(SourceColor, coord) / ColorScale;
		}
	}
    
    return Color;
}

technique Default {
	pass p0 {
		Lighting = true;
		AlphaOp[0] = MODULATE;
		AlphaArg1[0] = DIFFUSE;
		AlphaArg2[0] = TEXTURE;
		AlphaOp[1] = DISABLE;
	
		ZEnable = true;
		ZWriteEnable = false;
		ZFunc = LessEqual;  
		CullMode = None;
		SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
		AlphaBlendEnable = true;
	}
}

technique Advanced {
    pass p0 {
        VertexShader = null;
        
        ZEnable = true;
        ZWriteEnable = false;
        ZFunc = LessEqual;  
        CullMode = None;
        SrcBlend = SrcAlpha;
        DestBlend = InvSrcAlpha;
        AlphaBlendEnable = true;
        PixelShader = compile ps_2_0 PostProcessor();
    }
}
