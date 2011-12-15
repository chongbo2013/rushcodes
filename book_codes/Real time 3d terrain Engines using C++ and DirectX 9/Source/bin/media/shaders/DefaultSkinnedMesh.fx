float4 lhtDir = {0.0f, 0.0f, -1.0f, 1.0f};    //light Direction 
float4 lightDiffuse = {0.2f, 0.2f, 0.2f, 1.0f}; // Light Diffuse
float4 MaterialAmbient : MATERIALAMBIENT = {0.1f, 0.1f, 0.1f, 1.0f};
float4 MaterialDiffuse : MATERIALDIFFUSE = {0.8f, 0.8f, 0.8f, 1.0f};

// Matrix Pallette
static const int MAX_MATRICES = 26;
float4x3    mWorldMatrixArray[MAX_MATRICES] : WORLDMATRIXARRAY;
float4x4    mViewProj : VIEWPROJECTION;

///////////////////////////////////////////////////////
struct VS_INPUT
{
    float4  Pos             : POSITION;
    float4  BlendWeights    : BLENDWEIGHT;
    float4  BlendIndices    : BLENDINDICES;
};

struct VS_OUTPUT
{
    float4  Pos     : POSITION;
};


VS_OUTPUT VShade(VS_INPUT i, uniform int NumBones)
{
    VS_OUTPUT   o;
    float3      Pos = 0.0f;
    float       LastWeight = 0.0f;
     
    // Compensate for lack of UBYTE4 on Geforce3
    int4 IndexVector = D3DCOLORtoUBYTE4(i.BlendIndices);

    // cast the vectors to arrays for use in the for loop below
    float BlendWeightsArray[4] = (float[4])i.BlendWeights;
    int   IndexArray[4]        = (int[4])IndexVector;
    
    // calculate the pos/normal using the "normal" weights 
    //        and accumulate the weights to calculate the last weight
    for (int iBone = 0; iBone < NumBones-1; iBone++)
    {
        LastWeight = LastWeight + BlendWeightsArray[iBone];
        
        Pos += mul(i.Pos, mWorldMatrixArray[IndexArray[iBone]]) * BlendWeightsArray[iBone];
    }
    LastWeight = 1.0f - LastWeight; 

    // Now that we have the calculated weight, add in the final influence
    Pos += (mul(i.Pos, mWorldMatrixArray[IndexArray[NumBones-1]]) * LastWeight);
    
    // transform position from world space into view and then projection space
    o.Pos = mul(float4(Pos.xyz, 1.0f), mViewProj);

    return o;
}

int CurNumBones = 2;
VertexShader vsArray[4] = { compile vs_1_1 VShade(1), 
                            compile vs_1_1 VShade(2),
                            compile vs_1_1 VShade(3),
                            compile vs_1_1 VShade(4)
                          };


//////////////////////////////////////
// Techniques specs follow
//////////////////////////////////////
technique t0
{
	pass p0
	{
		//FILLMODE = WIREFRAME;
		CULLMODE = CCW;
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;

		AlphaBlendEnable = false;

		VertexShader = (vsArray[CurNumBones]);
	}
}

