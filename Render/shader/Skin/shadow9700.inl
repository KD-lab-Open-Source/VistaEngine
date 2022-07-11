///////////////////////////Radeon shadow//////////////////
#define ccx 0.0005
static float4 sh_c4[2]={
	//          x     y    y     x
		float4(-ccx, -ccx, ccx, ccx),
		float4(-ccx, ccx, -ccx, ccx)
};
//float4 vShade;//=float4(0.4,1.0);//shade.x=0.4, shade.y=1.0
//static float4 vShadeFX=float4(0.4,0.4,0.4,0.6);

//*
float Shadow97002x2(sampler2D sh_sampler,float4 sh)
{
	float4 f4;
//*
	for(int i=0;i<2;i++)
	{
		f4[i*2]=tex2D(sh_sampler,sh.xy+sh_c4[i].xy).x-sh.z;
		f4[i*2+1]=tex2D(sh_sampler,sh.xy+sh_c4[i].wz).x-sh.z;
	}
/*/
	f4=tex2Dproj(sh_sampler,sh).x-sh.z;
/**/
	f4=(f4>=0)?float4(1,1,1,1):float4(0,0,0,0);
	return dot(f4,0.25);
}

static float4 sh_c16[8]={
	float4(-0.000692, -0.000868,-0.002347, 0.000450),
	float4(0.000773, -0.002042, -0.001592, 0.001880),
	float4(-0.001208, -0.001198, -0.000425, -0.000915),
	float4(-0.000050, 0.000105, -0.000753, 0.001719),
	float4(-0.001855, -0.000004, 0.001140, -0.001212),
	float4(0.000684, 0.000273, 0.000177, 0.000647),
	float4(-0.001448, 0.002095, 0.000811, 0.000421),
	float4(0.000542, 0.001491, 0.000537, 0.002367)
};

float Shadow97004x4(sampler2D sh_sampler,float3 sh)
{
	float sum=0;
	for(int j=0;j<4;j++)
	{
		float4 f4;
		for(int i=0;i<2;i++)
		{
			f4[i*2]=tex2D(sh_sampler,sh.xy+sh_c16[i+j*2].xy).x-sh.z;
			f4[i*2+1]=tex2D(sh_sampler,sh.xy+sh_c16[i+j*2].wz).x-sh.z;
		}

		f4=(f4>=0)?float4(1,1,1,1):float4(0,0,0,0);
		sum+=dot(f4,0.0625);
	}
	return sum;
}

float Shadow97003x4Unit(sampler2D sh_sampler,float3 sh)
{
	float sum=0;
	for(int j=0;j<3;j++)
	{
		float4 f4;
		for(int i=0;i<2;i++)
		{
			f4[i*2]=tex2D(sh_sampler,sh.xy+sh_c16[i+j*2].xy).x-sh.z;
			f4[i*2+1]=tex2D(sh_sampler,sh.xy+sh_c16[i+j*2].wz).x-sh.z;
		}

		f4=(f4>=0)?float4(1,1,1,1):float4(0,0,0,0);
		sum+=dot(f4,0.0833);
	}
	return sum;
}

/*/
float Shadow97002x2(sampler2D sh_sampler,float3 sh)
{
	float4 f4;
	for(int i=0;i<2;i++)
	{
		f4[i*2]=tex2D(sh_sampler,sh.xy+sh_c4[i].xy)-sh.z;
		f4[i*2+1]=tex2D(sh_sampler,sh.xy+sh_c4[i].wz)-sh.z;
	}
	f4=(f4>=0)?float4(vShade.y,vShade.y,vShade.y,vShade.y):float4(vShade.x,vShade.x,vShade.x,vShade.x);
	return dot(f4,0.25);
}

float Shadow97004x4(sampler2D sh_sampler,float4 sh)
{
	const float SHADOW_SIZE = 512.f;

    float shadow = 0;
    for (int y=-2; y<2; y++)
        for (int x=-2; x<2; x++)
        {
            float4 coord = sh;
            coord.xy += (float2(x,y)/SHADOW_SIZE) * sh.w;
            shadow += tex2Dproj( sh_sampler, coord )-sh.z;
        }
        
    shadow= smoothstep(2, 58, shadow);    
	return shadow;
}

/**/

float Shadow9700(sampler2D sh_sampler,float4 sh)
{
#ifdef c2x2
	if(c2x2==3)
		return Shadow97003x4Unit(sh_sampler,sh);
	else
		return Shadow97002x2(sh_sampler,sh);
#else
	#ifdef ZREFLECTION
		return Shadow97003x4Unit(sh_sampler,sh);
	#else
		return Shadow97004x4(sh_sampler,sh);
	#endif
#endif
}


///////////////////////GeforceFX shadow//////////////////////////
//#define FX_OFFSET  0.0009765
//#define FX_OFFSET  0.0004882
//#define FX_OFFSET2 0.0004882
//static float4 fx_offset=float4(0.0004882,0,0,0);
//static float4 fx_offset=float4(0.0009765,0,0,0);

float3 ShadowFX2x2(sampler sh_sampler,float4 sh)
{
	float4 o=tex2Dproj(sh_sampler,sh);
//	return o*vShade.rgb+vShade.a;
	return o;
}

float3 ShadowFX4x4(sampler sh_sampler,float4 sh)
{

	float4 offs=fx_offset*sh.w;
	float3 o=
	   tex2Dproj(sh_sampler,sh-offs);
	o+=tex2Dproj(sh_sampler,sh+offs);
	o+=tex2Dproj(sh_sampler,sh-offs.zxyw);
	o+=tex2Dproj(sh_sampler,sh+offs.zxyw);
	o*=0.25;
//	return o*vShade.rgb+vShade.a;
	return o;
}
//*
float3 ShadowFX2x2Unit(sampler sh_sampler,float4 sh)
{
	float3 o=
	   tex2Dproj(sh_sampler,sh);
	return o;
//	return o*vShade.rgb+vShade.a;
}

float3 ShadowFX3x4Unit(sampler sh_sampler,float4 sh)
{
	float4 offs=fx_offset*sh.w;
	float3 o=
	   tex2Dproj(sh_sampler,sh-offs);
	o+=tex2Dproj(sh_sampler,sh+offs);
	o+=tex2Dproj(sh_sampler,sh-offs.zxyw);
	o+=tex2Dproj(sh_sampler,sh+offs.zxyw);

	o*=0.25;
	return o;
//	return o*vShade.rgb+vShade.a;

}
/*/

float3 ShadowFX3x4Unit(sampler3D sh_sampler,float3 sh)
{

	float sum=0,sumw=0;

	float c=1.0f;
	float map_size=256;//1024;
	for(int y=-2;y<=2;y++)
	for(int x=-2;x<=2;x++)
	{
		float2 delta=frac(sh*map_size);
		float2 offset=float2(x,y);
		offset-=delta;
		float2 weight2=saturate(abs(offset)*c);
		float weight=(1-weight2.x)*(1-weight2.y);
		sumw+=weight;

//		offset-=float2(0.5,0.5);
		offset/=map_size;
		float o=tex3D(sh_sampler,sh+float3(offset.x,offset.y,0));
		sum+=o*weight;
	}

	sum/=sumw;

//	sum=tex3D(sh_sampler,sh);
//	return sum*vShade.rgb+vShade.a;
	return float3(frac(sh.x*map_size),frac(sh.y*map_size),0);
}
/**/
float3 ShadowFX(sampler sh_sampler,float4 sh)
{
#ifdef c2x2
	if(c2x2==3)
		return ShadowFX3x4Unit(sh_sampler,sh);
	else
	if(c2x2==2)
		return ShadowFX2x2Unit(sh_sampler,sh);
	else
		return ShadowFX2x2(sh_sampler,sh);
#else
	return ShadowFX4x4(sh_sampler,sh);
#endif
}
void Shadow(inout float3 ot,sampler sh_sampler,float4 sh)
{
	float3 mul;
#ifdef SHADOW_9700
	mul=Shadow9700(sh_sampler,sh);
#endif
#ifdef SHADOW_FX
	mul=ShadowFX(sh_sampler,sh);
#endif
	mul=vShade.rgb*(1-mul)+mul;

	ot.rgb*=mul;
}

