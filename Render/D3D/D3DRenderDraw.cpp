#include "StdAfxRD.h"
#include "Render\D3D\D3DRender.h"
#include "Render\Src\cCamera.h"
#include "Render\src\FT_Font.h"
#include "UnicodeConverter.h"
#include "VisGenericDefine.h"

void cD3DRender::SetRenderTarget(cTexture* target,IDirect3DSurface9* pZBuffer)
{
	IDirect3DSurface9* lpDDSurface = 0;
	RDCALL((target->GetDDSurface(0))->GetSurfaceLevel(0, &lpDDSurface));
	SetRenderTarget(lpDDSurface, pZBuffer);

	lpDDSurface->Release();
}

void cD3DRender::SetRenderTarget(IDirect3DSurface9* target,IDirect3DSurface9* pZBuffer)
{
	for( int nPasses=0; nPasses<nSupportTexture; nPasses++ ) 
		D3DDevice_->SetTexture( nPasses, CurrentTexture[nPasses]=0 );

	IDirect3DSurface9* lpDDSurface=0;
	RDCALL(D3DDevice_->SetRenderTarget(0,target));
	RDCALL(D3DDevice_->SetDepthStencilSurface(pZBuffer));

	if(!pZBuffer)
	{
		SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
		SetRenderState( D3DRS_ZWRITEENABLE, FALSE ); 
	}
	else{
		SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
		SetRenderState( D3DRS_ZWRITEENABLE, TRUE); 
	}
}

void cD3DRender::SetRenderTarget1(cTexture* target1)
{
	if(!(DeviceCaps.NumSimultaneousRTs>=2))
		return;
	if(target1){
		IDirect3DSurface9* lpDDSurface=0;
		RDCALL((target1->GetDDSurface(0))->GetSurfaceLevel(0,&lpDDSurface));
		RDCALL(D3DDevice_->SetRenderTarget(1,lpDDSurface));
		lpDDSurface->Release();
	}
	else
		RDCALL(D3DDevice_->SetRenderTarget(1,0));
}


void cD3DRender::RestoreRenderTarget()
{
	RDCALL(D3DDevice_->SetRenderTarget(0,backBuffer_));
	if(DeviceCaps.NumSimultaneousRTs>=2)
		RDCALL(D3DDevice_->SetRenderTarget(1,0));
	RDCALL(D3DDevice_->SetDepthStencilSurface(zBuffer_));
	SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	SetRenderState( D3DRS_ZWRITEENABLE, TRUE ); 
}

void cD3DRender::setCamera(Camera *camera)
{
	if((camera_=camera) == 0 || D3DDevice_ == 0) 
		return;

	if(camera_->GetRenderSurface() || camera_->GetRenderTarget()){
		IDirect3DSurface9* pZBuffer=camera_->GetZBuffer();
		if(camera_->GetRenderSurface())
			SetRenderTarget(camera_->GetRenderSurface(),pZBuffer);
		else
			SetRenderTarget(camera_->GetRenderTarget(),pZBuffer);

		DWORD color=0;
		if(camera->getAttribute(ATTRCAMERA_SHADOWMAP))
			color=D3DCOLOR_RGBA(0,0,0,255);//test TSM
			//color=D3DCOLOR_RGBA(255,255,255,255);//Для Radeon 9700
		else
			color=camera->GetFoneColor().RGBA();
		if(camera->getAttribute(ATTRCAMERA_REFLECTION))
			color&=~0xff000000;

		if(!camera->getAttribute(ATTRCAMERA_NOCLEARTARGET)){
			if(!pZBuffer){
				RDCALL(D3DDevice_->Clear(0,0,D3DCLEAR_TARGET,color,1,0));
			}
			else{
				RDCALL(D3DDevice_->Clear(0,0,D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 
					camera->getAttribute(ATTRCAMERA_ZINVERT) ? 0 : 1, 0));
			}
		}
	}
	else
		RestoreRenderTarget();

	SetDrawTransform(camera);

	{//Немного не к месту, зато быстро по скорости, для отражений.
		SetTexture(5,camera->GetZTexture());
		SetSamplerData(5,sampler_clamp_linear);
	}
}

void cD3DRender::SetDrawTransform(Camera *camera)
{
	camera_=camera;
	RDCALL(D3DDevice_->SetTransform(D3DTS_PROJECTION, camera->matProj));
	RDCALL(D3DDevice_->SetTransform(D3DTS_VIEW, camera->matView));

	Vect2f rendersize=camera->GetRenderSize();
	xassert(camera->vp.X>=0 && camera->vp.X<=rendersize.x);
	xassert(camera->vp.Y>=0 && camera->vp.Y<=rendersize.y);
	xassert(camera->vp.X+camera->vp.Width>=0 && camera->vp.X+camera->vp.Width<=rendersize.x);
	xassert(camera->vp.Y+camera->vp.Height>=0 && camera->vp.Y+camera->vp.Height<=rendersize.y);

	RDCALL(D3DDevice_->SetViewport((D3DVIEWPORT9*)&camera->vp));
	if(camera->getAttribute(ATTRCAMERA_REFLECTION)==0)
		SetRenderState(D3DRS_CULLMODE,CurrentCullMode=D3DCULL_CW);	// прямое изображение
	else
		SetRenderState(D3DRS_CULLMODE,CurrentCullMode=D3DCULL_CCW);	// отраженное изображение
}

void cD3DRender::drawCircle(const Vect3f& vc, float radius, Color4c color)
{
	float segment_length = 3;
	int N = round(2*M_PI*radius/segment_length);
	if(N < 10)
		N = 10;
	float dphi = 2*M_PI/N;
	Vect3f v0 = vc + Vect3f(radius,0,0);
	for(float phi = dphi;phi < 2*M_PI + dphi/2; phi += dphi){
		Vect3f v1 = vc + Vect3f(cos(phi), sin(phi),0)*radius;
		DrawLine(v0, v1,color);
		v0 = v1;
	}
}

void cD3DRender::DrawBound(const MatXf &Matrix,Vect3f &min,Vect3f &max,bool wireframe,Color4c diffuse)
{ 
	xassert(camera_);
	DWORD d3d_wireframe=GetRenderState(D3DRS_FILLMODE);
	DWORD d3d_zwrite=GetRenderState(RS_ZWRITEENABLE);
	SetRenderState( RS_ZWRITEENABLE, FALSE );
	if(wireframe)
		SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);
	SetNoMaterial(ALPHA_BLEND,Matrix);

	cQuadBuffer<sVertexXYZDT1>* buf=GetQuadBufferXYZDT1();
	sVertexXYZDT1 p,*v;
	p.diffuse=diffuse;
	p.GetTexel().set(0,0);
	buf->BeginDraw(Matrix);

	v=buf->Get();//bottom
	p.pos.set(min.x,min.y,min.z);v[0]=p;
	p.pos.set(max.x,min.y,min.z);v[1]=p;
	p.pos.set(min.x,max.y,min.z);v[2]=p;
	p.pos.set(max.x,max.y,min.z);v[3]=p;

	v=buf->Get();//top
	p.pos.set(min.x,min.y,max.z);v[0]=p;
	p.pos.set(min.x,max.y,max.z);v[1]=p;
	p.pos.set(max.x,min.y,max.z);v[2]=p;
	p.pos.set(max.x,max.y,max.z);v[3]=p;

	v=buf->Get();//right
	p.pos.set(max.x,min.y,min.z);v[0]=p;
	p.pos.set(max.x,min.y,max.z);v[1]=p;
	p.pos.set(max.x,max.y,min.z);v[2]=p;
	p.pos.set(max.x,max.y,max.z);v[3]=p;

	v=buf->Get();//left
	p.pos.set(min.x,min.y,min.z);v[0]=p;
	p.pos.set(min.x,max.y,min.z);v[1]=p;
	p.pos.set(min.x,min.y,max.z);v[2]=p;
	p.pos.set(min.x,max.y,max.z);v[3]=p;

	v=buf->Get();//rear
	p.pos.set(min.x,min.y,min.z);v[0]=p;
	p.pos.set(min.x,min.y,max.z);v[1]=p;
	p.pos.set(max.x,min.y,min.z);v[2]=p;
	p.pos.set(max.x,min.y,max.z);v[3]=p;

	v=buf->Get();//front
	p.pos.set(min.x,max.y,min.z);v[0]=p;
	p.pos.set(max.x,max.y,min.z);v[1]=p;
	p.pos.set(min.x,max.y,max.z);v[2]=p;
	p.pos.set(max.x,max.y,max.z);v[3]=p;
	buf->EndDraw();
	
	SetRenderState( D3DRS_FILLMODE,d3d_wireframe);
	SetRenderState( RS_ZWRITEENABLE, d3d_zwrite );
}

void ChangeTextColorW(const wchar_t*& str, Color4c& diffuse)
{
	while(*str == L'&'){
		++str;
		if(*str == L'&')
			return;
		DWORD s = 0;
		int i = 0;
		for(; i < 6; ++i, ++str)
			if(wchar_t k = *str){
				if(k >= L'0' && k <= '9')
					s = (s << 4) + (k-'0');
				else if(k >= L'A' && k <= 'F')
					s = (s << 4) + (k-'A'+10);
				else if(k >= L'a' && k <= L'f')
					s = (s << 4) + (k - L'a' + 10);
				else
					break;
			}
			else
				break;

		if(i > 5){
			diffuse.RGBA() &= 0xFF000000;
			diffuse.RGBA() |= s;
		}
		else
			return;
	}
}

void cD3DRender::OutText(int _x, int _y, const char *outtext, const Color4f& color, ALIGN_TEXT align, eBlendMode blend_mode, Vect2f scale)
{
	if(!CurrentFont){
		xassert(CurrentFont != 0);
		return;
	}

	SetTexture(0, const_cast<cTexture*>(CurrentFont->texture()));
	SetTextureBase(1, 0);
	SetTextureBase(2, 0);
	SetBlendStateAlphaRef(blend_mode);

	SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);

	psFont->Select();

	if(fabsf(scale.x - 1.0f) < 0.01f)
		scale.x = 1.f;
	if(fabsf(scale.y - 1.0f) < 0.01f)
		scale.y = 1.f;
	
	bool scaled = (scale.x != 1.f || scale.y != 1.f);
	if(scaled)
		SetSamplerData(0, sampler_clamp_linear);
	else
		SetSamplerData(0, sampler_clamp_point);
	
	wstring out(a2w(outtext));
	Color4c diffuse(color);

	float txWidth = float(CurrentFont->texture()->GetWidth());
	float txHeight = float(CurrentFont->texture()->GetHeight());

	QuadBufferXYZWDT1.BeginDraw();

	float y = float(_y);
	for(const wchar_t* str = out.c_str(); *str; ++str, y += scale.y * float(CurrentFont->lineHeight())){
		float x = float(_x);
		if(align >= 0){
			float width = scale.x * float(CurrentFont->lineWidth(str));
			if(align == 0)
				x -= width / 2.f;
			else
				x -= width;
		}
		x = float(round(x));
		for(; *str != L'\n'; ++str){
			ChangeTextColorW(str, diffuse);
			
			wchar_t symbol = *str;
			if(symbol == L'\n')
				break;
			if(!symbol)
				goto LABEL_DRAW;
			if(symbol < 32)
				continue;

			const FT::OneChar& one = CurrentFont->getChar(symbol);

			sVertexXYZWDT1* v = QuadBufferXYZWDT1.Get();
			sVertexXYZWDT1 &v1 = v[1], &v2 = v[0], &v3 = v[2], &v4 = v[3];

			if(scaled){
				v1.x = v4.x = float(round(x + scale.x * float(one.su - 1))) - 0.5f;
				v3.x = v2.x = v1.x + float(round(scale.x * float(one.du + 1)));

				v1.y = v2.y = float(round(y + scale.y * float(one.sv - 1))) - 0.5f;
				v3.y = v4.y = v1.y + float(round(scale.y * float(one.dv + 1)));

				v1.u1() = v4.u1() = float(one.u - 1) / txWidth;
				v3.u1() = v2.u1() = v1.u1() + float(one.du + 1) / txWidth;

				v1.v1() = v2.v1() = float(one.v - 1) / txHeight;
				v3.v1() = v4.v1() = v1.v1() + float(one.dv + 1) / txHeight;

				x += scale.x * float(one.advance);
			}
			else {
				v1.x = v4.x = x + float(one.su) - 0.5f;
				v3.x = v2.x = v1.x + float(one.du);

				v1.y = v2.y = y + float(one.sv) - 0.5f;
				v3.y = v4.y = v1.y + float(one.dv);

				v1.u1() = v4.u1() = float(one.u) / txWidth;
				v3.u1() = v2.u1() = v1.u1() + float(one.du) / txWidth;

				v1.v1() = v2.v1() = float(one.v) / txHeight;
				v3.v1() = v4.v1() = v1.v1() + float(one.dv) / txHeight;

				x += float(one.advance);
			}

			v1.diffuse = v2.diffuse = v3.diffuse = v4.diffuse = diffuse;

			v1.z = v2.z = v3.z = v4.z = 0.001f;
			v1.w = v2.w = v3.w = v4.w = 0.001f;

		}
	}
LABEL_DRAW:

	QuadBufferXYZWDT1.EndDraw();
}

int cD3DRender::OutTextLine(int x, int y, const FT::Font& font, const wchar_t *textline, const wchar_t* end, const Color4c& color, eBlendMode blend_mode, int xRangeMin, int xRangeMax)
{
	xassert(font.texture());

	SetTexture(0, const_cast<cTexture*>(font.texture()));
	SetTextureBase(1, 0);
	SetTextureBase(2, 0);
	SetBlendStateAlphaRef(blend_mode);

	SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);

	psFont->Select();

	SetSamplerData(0, sampler_clamp_point);

	float txWidth = float(font.texture()->GetWidth());
	float txHeight = float(font.texture()->GetHeight());
	
	QuadBufferXYZWDT1.BeginDraw();

	int prev_rh = 0;
	int prev_right = x;
	for(const wchar_t* str = textline; str != end; ++str)
	{
		wchar_t symbol = *str;

		if(symbol < 32)
			continue;
		
		const FT::OneChar& one = font.getChar(symbol);

		// сдвиг позиции рисования по X для отрисовки следующего символа
		int advance = one.advance;

		// субпиксельный контроль сдвига
		if(prev_rh - one.lh >= 32)
			--advance;
		else if(prev_rh - one.lh < -32)
			++advance;
		prev_rh = one.rh;

		// реальная правая граница символа
		int right = x + max(advance, one.su + one.du);

		// не находимся ли левее левой границы рисования
		if(xRangeMin >= 0 && x < xRangeMin){
			prev_right = right;
			x += advance;
			continue;
		}
		
		// не уперлись ли в правую границу рисования
		if(xRangeMax >= 0 && right > xRangeMax)
			break;

		sVertexXYZWDT1* v = QuadBufferXYZWDT1.Get();
		sVertexXYZWDT1 &v1 = v[1], &v2 = v[0], &v3 = v[2], &v4 = v[3];

		// т.к. в текстуре пустоты вокруг символов сжаты - их нужно учитывать отдельно
		v1.x = v4.x = float(x + one.su) - 0.5f;
		v3.x = v2.x = v1.x + float(one.du);

		v1.y = v2.y = float(y + one.sv) - 0.5f;
		v3.y = v4.y = v1.y + float(one.dv);
		
		// выводим пиксель-в-пиксель из текстуры на экран
		v1.u1() = v4.u1() = float(one.u) / txWidth;
		v3.u1() = v2.u1() = v1.u1() + float(one.du) / txWidth;

		v1.v1() = v2.v1() = float(one.v) / txHeight;
		v3.v1() = v4.v1() = v1.v1() + float(one.dv) / txHeight;

		v1.diffuse = v2.diffuse = v3.diffuse = v4.diffuse = color;

		v1.z = v2.z = v3.z = v4.z = 0.001f;
		v1.w = v2.w = v3.w = v4.w = 0.001f;

		prev_right = right;
		x += advance;
	}

	QuadBufferXYZWDT1.EndDraw();
	return prev_right;
}
