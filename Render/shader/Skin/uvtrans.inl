#ifdef UVTRANS
	float3 vt0=float3(v.t0.x,v.t0.y,1);
	o.t0.x=dot(vt0,vUtrans);
	o.t0.y=dot(vt0,vVtrans);
#else
	o.t0=v.t0;
#endif
#ifdef SECOND_OPACITY_TEXTURE
	#if(SECOND_OPACITY_TEXTURE==2)
		// animated:
		#ifdef SECOND_UVTRANS
		float3 vt1=float3(v.t1.x,v.t1.y,1);
		o.t1.x=dot(vt1,vSecondUtrans);
		o.t1.y=dot(vt1,vSecondVtrans);
		#else
		o.t1 = v.t1;
		#endif
	#else	
		// animated:
		#ifdef SECOND_UVTRANS
			#ifndef UVTRANS
			float3 vt0=float3(v.t0.x,v.t0.y,1);
			#endif
		o.t1.x=dot(vt0,vSecondUtrans);
		o.t1.y=dot(vt0,vSecondVtrans);
		#else
		o.t1 = v.t0;
		#endif
	#endif
#endif
