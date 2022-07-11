#include "StdAfx.h"
#include "DrawGraph.h"
#include "Render\D3D\D3DRender.h"

DrawGraph::DrawGraph()
{
	wxmin=0;
	wxmax=1;
	wymin=0;
	wymax=1;
	xmin=0;
	xmax=1;
	ymin=0;
	ymax=1;

	half_shift=false;
}

void DrawGraph::SetWindow(float xmin_,float xmax_,float ymin_, float ymax_)
{
	wxmin=xmin_;
	wxmax=xmax_;
	wymin=ymin_;
	wymax=ymax_;
}

void DrawGraph::SetArgumentRange(float xmin_,float xmax_,float ymin_,float ymax_)
{
	xmin=xmin_;
	xmax=xmax_;
	ymin=ymin_;
	ymax=ymax_;
}

void DrawGraph::Draw(DrawFunctor& f)
{
	Color4c color(255,255,255);
	float fxmin=gb_RenderDevice->GetSizeX()*wxmin;
	float fxrange=gb_RenderDevice->GetSizeX()*(wxmax-wxmin);
	int ixmin=round(fxmin);
	int ixmax=round(gb_RenderDevice->GetSizeX()*wxmax);

	int xold=0,yold=0;
	for(int ix=ixmin;ix<=ixmax;ix++)
	{
		float x=(ix-fxmin)/fxrange;
		if(half_shift)
		{
			x=fmod(x+0.5f,1);
		}
		x=x*(xmax-xmin)+xmin;

		float y=f.get(x,&color);

		float fy=(y-ymin)/(ymax-ymin);
		int iy;
		fy=fy*(wymax-wymin)+wymin;
		iy=round((1-fy)*gb_RenderDevice->GetSizeY());

		if(ix>ixmin)
			gb_RenderDevice->DrawLine(xold,yold,ix,iy,color);

		xold=ix;
		yold=iy;
	}
}

void DrawGraph::DrawXPosition(float x,Color4c color)
{
	if(half_shift)
	{
		x=fmod(x+0.5f,1);
	}
	float fxmin=gb_RenderDevice->GetSizeX()*wxmin;
	float fxrange=gb_RenderDevice->GetSizeX()*(wxmax-wxmin);
	float ix=x*fxrange+fxmin;
	gb_RenderDevice->DrawLine(ix,0,ix,gb_RenderDevice->GetSizeY(),color);
}
