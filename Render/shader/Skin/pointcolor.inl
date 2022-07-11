#ifdef POINT_LIGHT
	{
		float3 dpos0=vPointPos0-world_pos;
		float3 dpos1=vPointPos1-world_pos;
		float2 r12;
		r12.x=dot(dpos0,dpos0);
		r12.y=dot(dpos1,dpos1);
		r12.xy=max(r12.xy*vLightAttenuation.xy+vLightAttenuation.zw,0);
		o.specular.rgb+=vPointColor0*r12.x;
		o.specular.rgb+=vPointColor1*r12.y;
	}
#endif

//attenuation
// color=r2*att2+att0
// r2=0 color=c, att0=c
// r2=r2 color=0, att2=-c/r2
// как правило c=1