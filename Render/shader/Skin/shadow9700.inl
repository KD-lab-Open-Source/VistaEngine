///////////////////////////Radeon shadow//////////////////
#define ccx 0.0005
static float4 sh_c4[2]={
	//          x     y    y     x
		float4(-ccx, -ccx, ccx, ccx),
		float4(-ccx, ccx, -ccx, ccx)
};

float Shadow97002x2(sampler2D sh_sampler,float4 sh)
{
	float4 f4;
	for(int i=0;i<2;i++){
		f4[i*2]=tex2D(sh_sampler,sh.xy+sh_c4[i].xy).x-sh.z;
		f4[i*2+1]=tex2D(sh_sampler,sh.xy+sh_c4[i].wz).x-sh.z;
	}
	f4=(f4>=0)?float4(1,1,1,1):float4(0,0,0,0);
	return dot(f4,0.25);
}

float Shadow9700(sampler2D sh_sampler,float4 sh)
{
	float4 shadow = sh;
	shadow.xy /= shadow.w;	
	//shadow.z -= 0.001; // bias нельзя передавать через матрицу из за TSM
#ifdef FILTER_SHADOW
	return Shadow97002x2(sh_sampler, shadow);
#else
	return tex2D(sh_sampler,shadow).x - shadow.z > 0 ? 1 : 0;
#endif
}


///////////////////////GeforceFX shadow//////////////////////////

float3 ShadowFX2x2(sampler sh_sampler,float4 sh)
{
	float4 offs=fx_offset*sh.w;
	float3 o=tex2Dproj(sh_sampler,sh-offs);
	o+=tex2Dproj(sh_sampler,sh+offs);
	o+=tex2Dproj(sh_sampler,sh-offs.zxyw);
	o+=tex2Dproj(sh_sampler,sh+offs.zxyw);
	o*=0.25;
	return o;
}

float3 ShadowFX(sampler sh_sampler,float4 sh)
{
#ifdef FILTER_SHADOW
	return ShadowFX2x2(sh_sampler,sh); 
#else
	return tex2Dproj(sh_sampler,sh);
#endif
}

void Shadow(inout float3 ot,sampler sh_sampler,float4 sh, float k)
{
	float3 mul;

#ifdef SHADOW_9700
	mul=Shadow9700(sh_sampler,sh);
#endif
#ifdef SHADOW_FX
	mul=ShadowFX(sh_sampler,sh);
#endif
	mul *= k;
	mul=vShade.rgb*(1-mul)+mul;

	ot.rgb*=mul;
}
