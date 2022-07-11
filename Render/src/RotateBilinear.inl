void BikeRotateBilinearAndApply(BYTE* buffer,int dx,int dy,int pitch,
								BYTE* in_buffer,int in_dx, int in_dy,int in_pitch,
								int in_offsetx,int in_offsety,float angle,bool center)
{
	int shift=8;
	int mul=1<<shift;
	int ix=round(cos(angle)*mul);
	int iy=round(sin(angle)*mul);
	in_offsetx*=mul;
	in_offsety*=mul;
	if(center)
	{
		in_offsetx-=(ix*in_dx/2+iy*in_dy/2);
		in_offsety-=(-iy*in_dx/2+ix*in_dy/2);
	}

	//xx=( x*ix+y*iy>>shift)+in_offsetx;Прямая
	//yy=(-x*iy+y*ix>>shift)+in_offsety;
	int offx=-(in_offsetx*ix-in_offsety*iy)>>shift;
	int offy=-(in_offsetx*iy+in_offsety*ix)>>shift;
	//x=(xx*ix-yy*iy+offx)>>shift
	//y=(xx*iy+yy*ix+offx)>>shift

	int minx,miny,maxx,maxy;
	{
		int x[4]={0,in_dx,   0,in_dx};
		int y[4]={0,    0,in_dy,in_dy};

		for(int i=0;i<4;i++)
		{
			int xx=(( x[i]*ix+y[i]*iy+in_offsetx)>>shift);
			int yy=((-x[i]*iy+y[i]*ix+in_offsety)>>shift);
			if(i==0)
			{
				minx=maxx=xx;
				miny=maxy=yy;
			}else
			{
				minx=min(minx,xx);
				maxx=max(maxx,xx);
				miny=min(miny,yy);
				maxy=max(maxy,yy);
			}
		}

		minx=max(minx,0);
		maxx=min(maxx+1,dx);
		miny=max(miny,0);
		maxy=min(maxy+1,dy);
	}


	for(int yy=miny;yy<maxy;yy++)
	for(int xx=minx;xx<maxx;xx++)
	{
		int x=xx*ix-yy*iy+offx;
		int y=xx*iy+yy*ix+offy;
		sColor4c& out=*(sColor4c*)(buffer+xx*4+yy*pitch);
		
		if(0)
		{//point filter
			x=x>>shift;y=y>>shift;
			if(x>=0 && x<in_dx && y>=0 && y<in_dy)
			{
				sColor4c& in=*(sColor4c*)(in_buffer+x*4+y*in_pitch);
				out=in;
			}
		}else
		{
			int cx=x&255,cy=y&255;
			x=x>>shift;y=y>>shift;
			if(x>=-1 && x<in_dx && y>=-1 && y<in_dy)
			{
				sColor4c p00,p01,p10,p11;

#define GET(x,y) *(sColor4c*)(in_buffer+(x)*4+(y)*in_pitch)
#define IS_CLAMP(x,y) !((x)>=0 && (x)<in_dx && (y)>=0 && y<in_dy)

				if(x==-1 || x==in_dx-1 || y==-1 || y==in_dy-1)
				{
					p00=GET(max(x,0)        ,max(y,0));        if(IS_CLAMP(x  ,y))  p00.a=0;
					p01=GET(min(x+1,in_dx-1),max(y,0));        if(IS_CLAMP(x+1,y))  p01.a=0;
					p10=GET(max(x,0)        ,min(y+1,in_dy-1));if(IS_CLAMP(x  ,y+1))p10.a=0;
					p11=GET(min(x+1,in_dx-1),min(y+1,in_dy-1));if(IS_CLAMP(x+1,y+1))p11.a=0;

				}else
				{
					p00=GET(x,y);
					p01=GET(x+1,y);
					p10=GET(x,y+1);
					p11=GET(x+1,y+1);
				}

#ifdef BILIEAR_AND_ALPHABLEND
				sColor4c c;
#else
				sColor4c& c=out;
#endif
				c.r=bilinear(p00.r,p01.r,p10.r,p11.r,cx,cy);
				c.g=bilinear(p00.g,p01.g,p10.g,p11.g,cx,cy);
				c.b=bilinear(p00.b,p01.b,p10.b,p11.b,cx,cy);
				c.a=bilinear(p00.a,p01.a,p10.a,p11.a,cx,cy);
#ifdef BILIEAR_AND_ALPHABLEND
				out.r=ByteInterpolate(out.r,c.r,c.a);
				out.g=ByteInterpolate(out.g,c.g,c.a);
				out.b=ByteInterpolate(out.b,c.b,c.a);
				out.a=min(int(c.a)+int(out.a),255);
#endif
			}
#undef IS_CLAMP
#undef GET
		}
	}
}
