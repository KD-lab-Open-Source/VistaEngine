#include "StdAfx.h"
#include "ColorRampCtrl.h"
#include "mfc\MemDC.h"

IMPLEMENT_DYNAMIC(CColorRampCtrl, CWnd)

BEGIN_MESSAGE_MAP(CColorRampCtrl, CWnd)
    ON_WM_PAINT()
    ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


/*
void HBSColor::set(sColor4c color)
{
    float brightness;
    float hue;
    float saturation;
    unsigned char minval = min(color.r, min(color.g, color.b));
    unsigned char maxval = max(color.r, max(color.g, color.b));
    float mdiff  = float(maxval) - float(minval);
    float msum   = float(maxval) + float(minval);

    brightness = msum / 510.0f;
    //brightness = round(msum / 255.0f);

    if(maxval == minval){
        saturation = 0.0f;
        hue = 0.0f;
    }
    else{
        float rnorm = (maxval - color.r  ) / mdiff;      
        float gnorm = (maxval - color.g) / mdiff;
        float bnorm = (maxval - color.b ) / mdiff;   

        saturation = mdiff / msum;
        saturation = (brightness <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

        if(color.r == maxval)
            hue = 60.0f * (6.0f + bnorm - gnorm);
        if (color.g  == maxval)
            hue = 60.0f * (2.0f + rnorm - bnorm);
        if (color.b  == maxval)
            hue = 60.0f * (4.0f + gnorm - rnorm);
        if (hue > 360.0f)
            hue = hue - 360.0f;
    }
    this->hue = hue;
    this->brightness = brightness;
    this->saturation = saturation;
}
*/

void HBSColor::set(const sColor4f& color)
{
    float minval = min(color.r, min(color.g, color.b));
    float maxval = max(color.r, max(color.g, color.b));
    float mdiff = float(maxval) - float(minval);
    float msum = float(maxval) + float(minval);

    brightness = msum / 2.0f;
    if(maxval == minval){
        saturation = 0.0f;
        hue = 0.0f;
    }
    else{
        float rnorm = (maxval - color.r) / mdiff;      
        float gnorm = (maxval - color.g) / mdiff;
        float bnorm = (maxval - color.b) / mdiff;   


        if(color.r == maxval)
            hue = 60.0f * (6.0f + bnorm - gnorm);
        if(color.g  == maxval)
            hue = 60.0f * (2.0f + rnorm - bnorm);
        if(color.b  == maxval)
            hue = 60.0f * (4.0f + gnorm - rnorm);
        if(hue > 360.0f)
            hue = hue - 360.0f;

        saturation = mdiff / maxval;
		brightness = maxval;
        //brightness = mdiff / msum;
	}
}

float toRGBAux(float rm1, float rm2, float rh)
{
    if(rh > 360.0f)
        rh -= 360.0f;
    else if(rh <   0.0f)
        rh += 360.0f;
   
    if(rh < 60.0f)
        rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;   
    else if(rh < 180.0f)
        rm1 = rm2;
    else if(rh < 240.0f)
        rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;      
                     
    return rm1;
}

void HBSColor::toRGB(sColor4f& result) const
{
    /*
    sColor4c color;
    if(saturation == 0){
        unsigned char c = round(brightness * 255.0);
        color.set(c, c, c, 255);
    }
    else{
      float rm1, rm2;
      if(brightness <= 0.5f)
          rm2 = brightness + brightness * saturation; 
      else
          rm2 = brightness + saturation - brightness * saturation;
      rm1 = 2.0f * brightness - rm2;   

      color.r = toRGBAux(rm1, rm2, hue + 120.0f);   
      color.g = toRGBAux(rm1, rm2, hue);
      color.b = toRGBAux(rm1, rm2, hue - 120.0f);
    }
    */

    if(saturation == 0){
		result.set(brightness, brightness, brightness, 1.0f);
    }
    else{
      result.r = toRGBAux(0.0f, 1.0f, hue + 120.0f);   
      result.g = toRGBAux(0.0f, 1.0f, hue);
      result.b = toRGBAux(0.0f, 1.0f, hue - 120.0f);
	  result.a = 1.0f;
      
	  result *= brightness;
	  result.interpolate(result, sColor4f(brightness, brightness, brightness), 1.0f - saturation);
    }
}

CColorRampCtrl::CColorRampCtrl()
: hlsColor_(0.0f, 0.5f, 1.0f)
, mouseArea_(0)
{
    WNDCLASS wndclass;
    HINSTANCE hInst = AfxGetInstanceHandle();

    if(!::GetClassInfo(hInst, className(), &wndclass)){
        wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|WS_TABSTOP;
        wndclass.lpfnWndProc	= ::DefWindowProc;
        wndclass.cbClsExtra		= 0;
        wndclass.cbWndExtra		= 0;
        wndclass.hInstance		= hInst;
        wndclass.hIcon			= 0;
        wndclass.hCursor		= AfxGetApp()->LoadStandardCursor(IDC_ARROW);
        wndclass.hbrBackground	= reinterpret_cast<HBRUSH>(COLOR_WINDOW);
        wndclass.lpszMenuName	= 0;
        wndclass.lpszClassName	= className();

        if(!AfxRegisterClass(&wndclass))
            AfxThrowResourceException();
    }
}

BOOL CColorRampCtrl::Create(DWORD style, const RECT& rect, CWnd* parent, UINT id)
{
	if(CWnd::CreateEx(WS_EX_CLIENTEDGE, className(), "", style, rect, parent, id, 0)){
	    createRampBitmap();
		return TRUE;
	}
	else
		return FALSE;
}

void CColorRampCtrl::setColor(const sColor4f& color)
{
    color_ = color;
	hlsColor_.set(color);
	if(::IsWindow(GetSafeHwnd())){
		createRampBitmap();
        Invalidate(FALSE);			
	}
}


bool convertToDFB(HBITMAP& hBitmap)
{
  bool converted = false;
  BITMAP stBitmap;

  if(GetObject(hBitmap, sizeof(stBitmap), &stBitmap) && stBitmap.bmBits){
      // that is a DIB. Now we attempt to create
      // a DFB with the same sizes, and with the pixel
      // format of the display (to omit conversions
      // every time we draw it).
      HDC hScreen = GetDC(NULL);
      ASSERT(hScreen);

      HBITMAP hDfb = CreateCompatibleBitmap(hScreen, stBitmap.bmWidth, stBitmap.bmHeight);
      if(hDfb){
          // now let's ensure what we've created is a DIB.
          if(GetObject(hDfb, sizeof(stBitmap), &stBitmap) && !stBitmap.bmBits){
              // ok, we're lucky. Now we have
              // to transfer the image to the DFB.
              HDC hMemSrc = CreateCompatibleDC(NULL);
              if(hMemSrc){
                  HGDIOBJ hOldSrc = SelectObject(hMemSrc, hBitmap);
                  if(hOldSrc){
                      HDC hMemDst = CreateCompatibleDC(NULL);
                      if(hMemDst){
                          HGDIOBJ hOldDst = SelectObject(hMemDst, hDfb);
                          if(hOldDst){
                              if(BitBlt(hMemDst, 0, 0, stBitmap.bmWidth, stBitmap.bmHeight, hMemSrc, 0, 0, SRCCOPY))
                                  converted = true; // success

                              VERIFY(SelectObject(hMemDst, hOldDst));
                          }
                          VERIFY(DeleteDC(hMemDst));
                      }
                      VERIFY(SelectObject(hMemSrc, hOldSrc));
                  }
                  VERIFY(DeleteDC(hMemSrc));
              }
          }

          if (converted){
              VERIFY(DeleteObject(hBitmap)); // it's no longer needed
              hBitmap = hDfb;
          }
          else
              VERIFY(DeleteObject(hDfb));
      }
      ReleaseDC(0, hScreen);
  }
  return converted;
}

void CColorRampCtrl::createRampBitmap()
{
    const int width = 256;
    const int height = 256;
    DWORD* pixels = 0;

    HBSColor hlsColor = hlsColor_;
    hlsColor.saturation = 1.0f;
    hlsColor.brightness = 1.0f;
	sColor4f colorf(1.0f, 1.0f, 1.0f, 1.0f);
	hlsColor.toRGB(colorf);
    sColor4c color(colorf);

	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = 0;

	HBITMAP bitmap = CreateDIBSection(::GetDC(0), &bi, DIB_RGB_COLORS, (void**)&pixels, 0, 0);
	if(pixels)
		for(int j = 0; j < height; ++j)
			for(int i = 0; i < width; ++i){
				/*
				sColor4f cf;
				HBSColor(hlsColor.hue, float(j) / height, float(i) / width).toRGB(cf);
                sColor4c c(cf);
                pixels[j * width + i] = ((c.r << 16) + (c.g << 8) + (c.b));
										 */
                pixels[j * width + i] = ((((((color.r * j) * i >> 8) + (j * (255 - i))) & 0xFFFFFF00) << 8) +
                                         ((((color.g * j) * i >> 8) + (j * (255 - i))) & 0xFFFFFF00) +
                                         (((((color.b * j) * i >> 8) + (j * (255 - i))) >> 8)));
			}

	if(!bitmap)
		delete[] pixels;

    VERIFY(convertToDFB(bitmap));
	if(static_cast<HBITMAP>(rampBitmap_))
		rampBitmap_.DeleteObject();
    rampBitmap_.Attach(bitmap);

    //VERIFY(rampBitmap_.CreateBitmap(width, height, 3, 32, pixels));
}

void CColorRampCtrl::redraw(CDC& paintDC)
{
	CMemDC dc(&paintDC);

    CRect rect;
    GetClientRect(&rect);
    int width = rect.Width();
    int height = rect.Height();


    CDC tempDC;
    tempDC.CreateCompatibleDC(&dc);
    tempDC.SelectObject(rampBitmap_);

	int ramp_width = width - HUE_WIDTH - BORDER_WIDTH;
	int ramp_height = height;

	dc.SetStretchBltMode(HALFTONE);
    dc.StretchBlt(0, 0, ramp_width, ramp_height, &tempDC, 0, 0, 255, 255, SRCCOPY);
    
    // Курсор в Saturation/Brightness области
    int posY = round((1.0f - hlsColor_.brightness) * float(ramp_height));
    int posX = round((hlsColor_.saturation) * float(ramp_width));
	sColor4f colorf(1.0f, 1.0f, 1.0f, 1.0f);
	hlsColor_.toRGB(colorf);
    sColor4c color(colorf);

    dc.FillSolidRect(posX - 4, posY - 4, 9, 9, RGB(0, 0, 0));
    dc.FillSolidRect(posX - 3, posY - 3, 7, 7, RGB(color.r, color.g, color.b));

    dc.FillSolidRect(ramp_width, 0, BORDER_WIDTH, height, RGB(0, 0, 0));

	for(int i = 0; i < rect.Height(); ++i){
		float hue = 360.0f * float(i) / float(rect.Height());
		sColor4f colorf;
		HBSColor(hue, 1.0f, 1.0f).toRGB(colorf);
        sColor4c color(colorf);
		dc.FillSolidRect(width - HUE_WIDTH, i, width, 1, RGB(color.r, color.g, color.b));
	}

    // Курсор на Hue полоске
	int pos = round(hlsColor_.hue / 360.0f * rect.Height());
		
	HBSColor(hlsColor_.hue, 1.0f, 1.0f).toRGB(colorf);
    color = sColor4c(colorf);
	dc.FillSolidRect(width - HUE_WIDTH - 2, pos - 2, HUE_WIDTH + 4, 5, RGB(0, 0, 0));
	dc.FillSolidRect(width - HUE_WIDTH - 1, pos - 1, HUE_WIDTH + 2, 3, RGB(color.r, color.g, color.b));


    tempDC.DeleteDC();
}

void CColorRampCtrl::OnPaint()
{
    CPaintDC paintDC(this);
	redraw(paintDC);
}

void CColorRampCtrl::handleMouse(CPoint point)
{
	CRect rect;
	GetClientRect(&rect);
	int cx = rect.Width() - BORDER_WIDTH - HUE_WIDTH;
	int cy = rect.Height();

    float brightness = clamp(float(cy - point.y) / cy, 0.0f, 1.0f);
    float saturation = clamp(float(point.x) / cx, 0.0f, 1.0f);
	hlsColor_.brightness = brightness;
	hlsColor_.saturation = saturation;
	hlsColor_.toRGB(color_);
	if(signalColorChanged_)
		signalColorChanged_();
	mouseArea_ = 1;
	SetCapture();
	//Invalidate(FALSE);
	redraw(*GetDC());
}

void CColorRampCtrl::handleMouseHue(CPoint point)
{
	CRect rect;
	GetClientRect(&rect);
	int cx = rect.Width() - BORDER_WIDTH - HUE_WIDTH;
	int cy = rect.Height();

	float hue = clamp(float(point.y) * 360.0f / float(cy), 0.0f, 360.0f);
	if(fabs(hlsColor_.hue - hue) > FLT_COMPARE_TOLERANCE){
		hlsColor_.hue = hue;
		createRampBitmap();
		hlsColor_.toRGB(color_);
		if(signalColorChanged_)
			signalColorChanged_();
		redraw(*GetDC());
	}
	mouseArea_ = 0;
	SetCapture();
}

void CColorRampCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetClientRect(&rect);
	int cx = rect.Width() - BORDER_WIDTH - HUE_WIDTH;
	int cy = rect.Height();

	if(point.x > cx)
		handleMouseHue(point);
	else
		handleMouse(point);
    
	CWnd::OnLButtonDown(nFlags, point);
}

void CColorRampCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if(nFlags & MK_LBUTTON && GetCapture() == this){
		CRect rect;
		GetClientRect(&rect);
		int cx = rect.Width() - BORDER_WIDTH - HUE_WIDTH;
		int cy = rect.Height();

		if(mouseArea_ == 0)
			handleMouseHue(point);
		else
			handleMouse(point);
	}
    CWnd::OnMouseMove(nFlags, point);
}


void CColorRampCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	if(GetCapture() == this){
		ReleaseCapture();
	}
	CWnd::OnLButtonUp(nFlags, point);
}

void CColorRampCtrl::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

}

BOOL CColorRampCtrl::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

BOOL CColorRampCtrl::OnSetCursor(CWnd* wnd, UINT hitTest, UINT message)
{
	HCURSOR cursor = ::LoadCursor(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDC_CROSS));
	::SetCursor(cursor);
	return FALSE;
}
