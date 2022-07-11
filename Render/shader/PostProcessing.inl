void PSMonochrome::Select( float time_)
{
	SetVector(fTime,&D3DXVECTOR4(time_,time_,time_,time_));
	float mtime=1-time_;
	SetVector(fTimeInv,&D3DXVECTOR4(mtime,mtime,mtime,mtime));
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}

void PSMonochrome::RestoreShader()
{
	LoadShader("PostProcessing\\monochrome.psl");
}
void PSMonochrome::GetHandle()
{
	//__super::GetHandle();
	VAR_HANDLE(fTime);
	VAR_HANDLE(fTimeInv);
}
void PSColorDodge::RestoreShader()
{
	LoadShader("PostProcessing\\colordodge.psl");
}
void PSColorBright::Select(float luminance)
{
	SetVector(Luminance,&D3DXVECTOR4(luminance,luminance,luminance,luminance));
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSColorBright::RestoreShader()
{
	LoadShader("PostProcessing\\colorbright.psl");
}
void PSColorBright::GetHandle()
{
	VAR_HANDLE(Luminance);
}
void PSCombine::Select(sColor4f _color)
{
	SetVector(addColor,&D3DXVECTOR4(_color.r,_color.g,_color.b,_color.a));
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSCombine::RestoreShader()
{
	LoadShader("PostProcessing\\colorcombine.psl");
}
void PSCombine::GetHandle()
{
	VAR_HANDLE(addColor);
}
void PSBloomHorizontal::Select(float texWidth, float texHeight, float scale_)
{
	float texelKernel[13][4] = {
		{ -6.f/texWidth, 0, 0.002216f,0},
		{ -5.f/texWidth, 0, 0.008764f,0},
		{ -4.f/texWidth, 0, 0.026995f,0},
		{ -3.f/texWidth, 0, 0.064759f,0},
		{ -2.f/texWidth, 0, 0.120985f,0},
		{ -1.f/texWidth, 0, 0.176033f,0},
		{  0.f/texWidth, 0, 0.199471f,0},
		{  1.f/texWidth, 0, 0.176033f,0},
		{  2.f/texWidth, 0, 0.120985f,0},
		{  3.f/texWidth, 0, 0.064759f,0},
		{  4.f/texWidth, 0, 0.026995f,0},
		{  5.f/texWidth, 0, 0.008764f,0},
		{  6.f/texWidth, 0, 0.002216f,0},
	};
	SetVector(PixelKernel,(D3DXVECTOR4*)(&texelKernel),13);
	SetVector(BloomScale,&D3DXVECTOR4(scale_,scale_,scale_,scale_));
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSBloomHorizontal::RestoreShader()
{
	LoadShader("PostProcessing\\bloom.psl");
}
void PSBloomHorizontal::GetHandle()
{
	VAR_HANDLE(PixelKernel);
	VAR_HANDLE(BloomScale);
}
void PSBloomVertical::Select(float texWidth, float texHeight, float scale_)
{
	float texelKernel[13][4] = {
		{ 0,-6.f/texHeight, 0.002216f,0},
		{ 0,-5.f/texHeight, 0.008764f,0},
		{ 0,-4.f/texHeight, 0.026995f,0},
		{ 0,-3.f/texHeight, 0.064759f,0},
		{ 0,-2.f/texHeight, 0.120985f,0},
		{ 0,-1.f/texHeight, 0.176033f,0},
		{ 0, 0.f/texHeight, 0.199471f,0},
		{ 0, 1.f/texHeight, 0.176033f,0},
		{ 0, 2.f/texHeight, 0.120985f,0},
		{ 0, 3.f/texHeight, 0.064759f,0},
		{ 0, 4.f/texHeight, 0.026995f,0},
		{ 0, 5.f/texHeight, 0.008764f,0},
		{ 0, 6.f/texHeight, 0.002216f,0},
	};
	SetVector(PixelKernel,(D3DXVECTOR4*)(&texelKernel),13);
	SetVector(BloomScale,&D3DXVECTOR4(scale_,scale_,scale_,scale_));
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSBloomVertical::RestoreShader()
{
	LoadShader("PostProcessing\\bloom.psl");
}
void PSBloomVertical::GetHandle()
{
	VAR_HANDLE(PixelKernel);
	VAR_HANDLE(BloomScale);
}
void PSUnderWater::Select(float shift_,float scale_,sColor4f& color_)
{
	SetVector(shift,&D3DXVECTOR4(shift_,shift_,shift_,shift_));
	SetVector(scale,&D3DXVECTOR4(scale_*0.05f,scale_,scale_,scale_));
	SetVector(color,&D3DXVECTOR4(color_.r,color_.g,color_.b,color_.a));
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSUnderWater::RestoreShader()
{
	LoadShader("PostProcessing\\underwater.psl");
}
void PSUnderWater::GetHandle()
{
	VAR_HANDLE(shift);
	VAR_HANDLE(scale);
	VAR_HANDLE(color);
}
void PSBlurMap::RestoreShader()
{
	LoadShader("PostProcessing\\BlurMap.psl");
}
void PSBlurMap::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(dofParams);
}
void PSBlurMap::Select(Vect2f& params)
{
	SetVector(dofParams,&D3DXVECTOR4(params.x,params.y,0,0));
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSDOFCombine::Select()
{
	float dx = 1.0f / (float)gb_RenderDevice3D->GetSizeX();
	float dy = 1.0f / (float)gb_RenderDevice3D->GetSizeY();

	D3DXVECTOR4 v[12];
	v[0]  = D3DXVECTOR4(-0.326212f * dx, -0.405805f * dy, 0.0f, 0.0f);
	v[1]  = D3DXVECTOR4(-0.840144f * dx, -0.07358f * dy, 0.0f, 0.0f);
	v[2]  = D3DXVECTOR4(-0.695914f * dx, 0.457137f * dy, 0.0f, 0.0f);
	v[3]  = D3DXVECTOR4(-0.203345f * dx, 0.620716f * dy, 0.0f, 0.0f);
	v[4]  = D3DXVECTOR4(0.96234f * dx, -0.194983f * dy, 0.0f, 0.0f);
	v[5]  = D3DXVECTOR4(0.473434f * dx, -0.480026f * dy, 0.0f, 0.0f);
	v[6]  = D3DXVECTOR4(0.519456f * dx, 0.767022f * dy, 0.0f, 0.0f);
	v[7]  = D3DXVECTOR4(0.185461f * dx, -0.893124f * dy, 0.0f, 0.0f);
	v[8]  = D3DXVECTOR4(0.507431f * dx, 0.064425f * dy, 0.0f, 0.0f);
	v[9]  = D3DXVECTOR4(0.89642f * dx, 0.412458f * dy, 0.0f, 0.0f);
	v[10] = D3DXVECTOR4(-0.32194f * dx, -0.932615f * dy, 0.0f, 0.0f);
	v[11] = D3DXVECTOR4(-0.791559f * dx, -0.597705f * dy, 0.0f, 0.0f);
	SetVector(filterTaps,(D3DXVECTOR4*)v,12);
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSDOFCombine::RestoreShader()
{
	LoadShader("PostProcessing\\DOFCombine.psl");
}
void PSDOFCombine::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE(dofParams);
	VAR_HANDLE(filterTaps);
	VAR_HANDLE(maxCoC);
}
void PSDOFCombine::SetDofParams(Vect2f& params, float power)
{
	SetVector(maxCoC,&D3DXVECTOR4(power,0,0,0));
	SetVector(dofParams,&D3DXVECTOR4(params.x,params.y,0,0));
}
//void PSFiledDistort::Select(float shift_)
//{
//	SetVector(shift,&D3DXVECTOR4(shift_,shift_,shift_,shift_));
//	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
//}
//void PSFiledDistort::RestoreShader()
//{
//	LoadShader("PostProcessing\\FieldDistrot.psl");
//}
//void PSFiledDistort::GetHandle()
//{
//	VAR_HANDLE(shift);
//}
void PSFont::RestoreShader()
{
	LoadShader("font.psl");
}

void PSDown4::Select(float texWidth, float texHeight)
{
	float w = 1.f/texWidth;
	float h = 1.f/texHeight;
	float DownFilter[16][4] =
	{
		{ 1.5*w,  -1.5*h,0,0 },
		{ 1.5*w,  -0.5*h,0,0 },
		{ 1.5*w,   0.5*h,0,0 },
		{ 1.5*w,   1.5*h,0,0 },

		{ 0.5*w,  -1.5*h,0,0 },
		{ 0.5*w,  -0.5*h,0,0 },
		{ 0.5*w,   0.5*h,0,0 },
		{ 0.5*w,   1.5*h,0,0 },

		{-0.5*w,  -1.5*h,0,0 },
		{-0.5*w,  -0.5*h,0,0 },
		{-0.5*w,   0.5*h,0,0 },
		{-0.5*w,   1.5*h,0,0 },

		{-1.5*w,  -1.5*h,0,0 },
		{-1.5*w,  -0.5*h,0,0 },
		{-1.5*w,   0.5*h,0,0 },
		{-1.5*w,   1.5*h,0,0 },
	};
	SetVector(TexelCoordsDownFilter,(D3DXVECTOR4*)(&DownFilter),16);
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSDown4::RestoreShader()
{
	LoadShader("PostProcessing\\Down4.psl");
}

void PSDown4::GetHandle()
{
	VAR_HANDLE(TexelCoordsDownFilter);
}

void PSOverdraw::Select()
{
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSOverdraw::RestoreShader()
{
	LoadShader("overdraw.psl");
}
void PSOverdrawColor::Select()
{
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSOverdrawColor::RestoreShader()
{
	LoadShader("overdraw_color.psl");
}
void PSOverdrawCalc::Select(float texWidth, float texHeight)
{
	float PixelSum_[16][4] = {
		{0/texWidth,0/texHeight,0,0},
		{1/texWidth,0/texHeight,0,0},
		{2/texWidth,0/texHeight,0,0},
		{3/texWidth,0/texHeight,0,0},

		{0/texWidth,1/texHeight,0,0},
		{1/texWidth,1/texHeight,0,0},
		{2/texWidth,1/texHeight,0,0},
		{3/texWidth,1/texHeight,0,0},

		{0/texWidth,2/texHeight,0,0},
		{1/texWidth,2/texHeight,0,0},
		{2/texWidth,2/texHeight,0,0},
		{3/texWidth,2/texHeight,0,0},

		{0/texWidth,3/texHeight,0,0},
		{1/texWidth,3/texHeight,0,0},
		{2/texWidth,3/texHeight,0,0},
		{3/texWidth,3/texHeight,0,0},

	};
	
	SetVector(PixelSum,(D3DXVECTOR4*)(&PixelSum_),16);
	gb_RenderDevice3D->SetPixelShader(shader->GetPixelShader());
}
void PSOverdrawCalc::RestoreShader()
{
	LoadShader("overdraw_calc.psl");
}
void PSOverdrawCalc::GetHandle()
{
	VAR_HANDLE(PixelSum);
}

void PSMirage::Select()
{
	__super::Select();
}
void PSMirage::RestoreShader()
{
	LoadShader("PostProcessing\\mirage.psl");
}
