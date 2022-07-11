void PSMonochrome::Select( float time_)
{
	setVectorPS(fTime, Vect4f(time_,time_,time_,time_));
	float mtime=1-time_;
	setVectorPS(fTimeInv, Vect4f(mtime,mtime,mtime,mtime));
	
	__super::Select();
}

void PSMonochrome::RestoreShader()
{
	LoadShaderPS("PostProcessing\\monochrome.psl");
}

void PSMonochrome::GetHandle()
{
	//__super::GetHandle();
	VAR_HANDLE_PS(fTime);
	VAR_HANDLE_PS(fTimeInv);
}

void PSColorDodge::RestoreShader()
{
	LoadShaderPS("PostProcessing\\colordodge.psl");
}

void PSColorBright::Select(float luminance)
{
	setVectorPS(Luminance, Vect4f(luminance,luminance,luminance,luminance));
	__super::Select();
}

void PSColorBright::RestoreShader()
{
	LoadShaderPS("PostProcessing\\colorbright.psl");
}

void PSColorBright::GetHandle()
{
	VAR_HANDLE_PS(Luminance);
}

void PSCombine::Select(Color4f _color)
{
	setVectorPS(addColor, Vect4f(_color.r,_color.g,_color.b,_color.a));
	__super::Select();
}

void PSCombine::RestoreShader()
{
	LoadShaderPS("PostProcessing\\colorcombine.psl");
}

void PSCombine::GetHandle()
{
	VAR_HANDLE_PS(addColor);
}

void PSBloomHorizontal::Select(float texWidth, float texHeight, float scale_)
{
	static const Vect4f texelKernel[13] = {
		Vect4f(-6.f/texWidth, 0, 0.002216f, 0),
		Vect4f(-5.f/texWidth, 0, 0.008764f, 0),
		Vect4f(-4.f/texWidth, 0, 0.026995f, 0),
		Vect4f(-3.f/texWidth, 0, 0.064759f, 0),
		Vect4f(-2.f/texWidth, 0, 0.120985f, 0),
		Vect4f(-1.f/texWidth, 0, 0.176033f, 0),
		Vect4f(0.f/texWidth, 0, 0.199471f, 0),
		Vect4f(1.f/texWidth, 0, 0.176033f, 0),
		Vect4f(2.f/texWidth, 0, 0.120985f, 0),
		Vect4f(3.f/texWidth, 0, 0.064759f, 0),
		Vect4f(4.f/texWidth, 0, 0.026995f, 0),
		Vect4f(5.f/texWidth, 0, 0.008764f, 0),
		Vect4f(6.f/texWidth, 0, 0.002216f, 0),
	};
	setVectorPS(PixelKernel, texelKernel, 13);
	setVectorPS(BloomScale, Vect4f(scale_,scale_,scale_,scale_));
	__super::Select();
}

void PSBloomHorizontal::RestoreShader()
{
	LoadShaderPS("PostProcessing\\bloom.psl");
}

void PSBloomHorizontal::GetHandle()
{
	VAR_HANDLE_PS(PixelKernel);
	VAR_HANDLE_PS(BloomScale);
}

void PSBloomVertical::Select(float texWidth, float texHeight, float scale_)
{
	static const Vect4f texelKernel[13] = {
		Vect4f(0,-6.f/texHeight, 0.002216f, 0),
		Vect4f(0,-5.f/texHeight, 0.008764f, 0),
		Vect4f(0,-4.f/texHeight, 0.026995f, 0),
		Vect4f(0,-3.f/texHeight, 0.064759f, 0),
		Vect4f(0,-2.f/texHeight, 0.120985f, 0),
		Vect4f(0,-1.f/texHeight, 0.176033f, 0),
		Vect4f(0, 0.f/texHeight, 0.199471f, 0),
		Vect4f(0, 1.f/texHeight, 0.176033f, 0),
		Vect4f(0, 2.f/texHeight, 0.120985f, 0),
		Vect4f(0, 3.f/texHeight, 0.064759f, 0),
		Vect4f(0, 4.f/texHeight, 0.026995f, 0),
		Vect4f(0, 5.f/texHeight, 0.008764f, 0),
		Vect4f(0, 6.f/texHeight, 0.002216f, 0),
	};
	setVectorPS(PixelKernel, texelKernel,13);
	setVectorPS(BloomScale, Vect4f(scale_,scale_,scale_,scale_));

	__super::Select();
}
void PSBloomVertical::RestoreShader()
{
	LoadShaderPS("PostProcessing\\bloom.psl");
}

void PSBloomVertical::GetHandle()
{
	VAR_HANDLE_PS(PixelKernel);
	VAR_HANDLE_PS(BloomScale);
}

void PSUnderWater::Select(float shift_,float scale_,Color4f& color_)
{
	setVectorPS(shift, Vect4f(shift_,shift_,shift_,shift_));
	setVectorPS(scale, Vect4f(scale_*0.05f,scale_,scale_,scale_));
	setVectorPS(color, Vect4f(color_.r,color_.g,color_.b,color_.a));

	__super::Select();
}

void PSUnderWater::RestoreShader()
{
	LoadShaderPS("PostProcessing\\underwater.psl");
}

void PSUnderWater::GetHandle()
{
	VAR_HANDLE_PS(shift);
	VAR_HANDLE_PS(scale);
	VAR_HANDLE_PS(color);
}

void PSBlurMap::RestoreShader()
{
	LoadShaderPS("PostProcessing\\BlurMap.psl");
}

void PSBlurMap::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_PS(dofParams);
}

void PSBlurMap::Select(Vect2f& params)
{
	setVectorPS(dofParams, Vect4f(params.x,params.y,0,0));
	
	__super::Select();
}

void PSDOFCombine::Select()
{
	float dx = 1.0f / (float)gb_RenderDevice3D->GetSizeX();
	float dy = 1.0f / (float)gb_RenderDevice3D->GetSizeY();

	static Vect4f v[12] = {
		Vect4f(-0.326212f * dx, -0.405805f * dy, 0.0f, 0.0f),
		Vect4f(-0.840144f * dx, -0.07358f * dy, 0.0f, 0.0f),
		Vect4f(-0.695914f * dx, 0.457137f * dy, 0.0f, 0.0f),
		Vect4f(-0.203345f * dx, 0.620716f * dy, 0.0f, 0.0f),
		Vect4f(0.96234f * dx, -0.194983f * dy, 0.0f, 0.0f),
		Vect4f(0.473434f * dx, -0.480026f * dy, 0.0f, 0.0f),
		Vect4f(0.519456f * dx, 0.767022f * dy, 0.0f, 0.0f),
		Vect4f(0.185461f * dx, -0.893124f * dy, 0.0f, 0.0f),
		Vect4f(0.507431f * dx, 0.064425f * dy, 0.0f, 0.0f),
		Vect4f(0.89642f * dx, 0.412458f * dy, 0.0f, 0.0f),
		Vect4f(-0.32194f * dx, -0.932615f * dy, 0.0f, 0.0f),
		Vect4f(-0.791559f * dx, -0.597705f * dy, 0.0f, 0.0f)
	};
	setVectorPS(filterTaps,v,12);
	
	__super::Select();
}

void PSDOFCombine::RestoreShader()
{
	LoadShaderPS("PostProcessing\\DOFCombine.psl");
}

void PSDOFCombine::GetHandle()
{
	__super::GetHandle();
	VAR_HANDLE_PS(dofParams);
	VAR_HANDLE_PS(filterTaps);
	VAR_HANDLE_PS(maxCoC);
}

void PSDOFCombine::SetDofParams(Vect2f& params, float power)
{
	setVectorPS(maxCoC, Vect4f(power,0,0,0));
	setVectorPS(dofParams, Vect4f(params.x,params.y,0,0));
}

void PSFont::RestoreShader()
{
	LoadShaderPS("font.psl");
}

void PSDown4::Select(float texWidth, float texHeight)
{
	float w = 1.f/texWidth;
	float h = 1.f/texHeight;
	static const Vect4f DownFilter[16] =
	{
		Vect4f(1.5*w,  -1.5*h, 0, 0),
		Vect4f(1.5*w,  -0.5*h, 0, 0),
		Vect4f(1.5*w,   0.5*h, 0, 0),
		Vect4f(1.5*w,   1.5*h, 0, 0),

		Vect4f(0.5*w,  -1.5*h, 0, 0),
		Vect4f(0.5*w,  -0.5*h, 0, 0),
		Vect4f(0.5*w,   0.5*h, 0, 0),
		Vect4f(0.5*w,   1.5*h, 0, 0),

		Vect4f(0.5*w,  -1.5*h, 0, 0),
		Vect4f(0.5*w,  -0.5*h, 0, 0),
		Vect4f(0.5*w,   0.5*h, 0, 0),
		Vect4f(0.5*w,   1.5*h, 0, 0),

		Vect4f(1.5*w,  -1.5*h, 0, 0),
		Vect4f(1.5*w,  -0.5*h, 0, 0),
		Vect4f(1.5*w,   0.5*h, 0, 0),
		Vect4f(1.5*w,   1.5*h, 0, 0)
	};
	setVectorPS(TexelCoordsDownFilter, DownFilter, 16);
	
	__super::Select();
}

void PSDown4::RestoreShader()
{
	LoadShaderPS("PostProcessing\\Down4.psl");
}

void PSDown4::GetHandle()
{
	VAR_HANDLE_PS(TexelCoordsDownFilter);
}

void PSOverdraw::RestoreShader()
{
	LoadShaderPS("overdraw.psl");
}

void PSOverdrawColor::RestoreShader()
{
	LoadShaderPS("overdraw_color.psl");
}

void PSOverdrawCalc::Select(float texWidth, float texHeight)
{
	static const Vect4f PixelSum_[16] = {
		Vect4f(0/texWidth,0/texHeight,0,0),
		Vect4f(1/texWidth,0/texHeight,0,0),
		Vect4f(2/texWidth,0/texHeight,0,0),
		Vect4f(3/texWidth,0/texHeight,0,0),

		Vect4f(0/texWidth,1/texHeight,0,0),
		Vect4f(1/texWidth,1/texHeight,0,0),
		Vect4f(2/texWidth,1/texHeight,0,0),
		Vect4f(3/texWidth,1/texHeight,0,0),

		Vect4f(0/texWidth,2/texHeight,0,0),
		Vect4f(1/texWidth,2/texHeight,0,0),
		Vect4f(2/texWidth,2/texHeight,0,0),
		Vect4f(3/texWidth,2/texHeight,0,0),

		Vect4f(0/texWidth,3/texHeight,0,0),
		Vect4f(1/texWidth,3/texHeight,0,0),
		Vect4f(2/texWidth,3/texHeight,0,0),
		Vect4f(3/texWidth,3/texHeight,0,0)
	};
	
	setVectorPS(PixelSum, PixelSum_, 16);
	
	__super::Select();
}

void PSOverdrawCalc::RestoreShader()
{
	LoadShaderPS("overdraw_calc.psl");
}

void PSOverdrawCalc::GetHandle()
{
	VAR_HANDLE_PS(PixelSum);
}

void PSMirage::Select()
{
	__super::Select();
}

void PSMirage::RestoreShader()
{
	LoadShaderPS("PostProcessing\\mirage.psl");
}

