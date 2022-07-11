// NoisePreview.cpp : implementation file
//

#include "stdafx.h"
#include "EffectTool.h"
#include "NoisePreview.h"
#include ".\noisepreview.h"


// CNoisePreview

IMPLEMENT_DYNAMIC(CNoisePreview, CWnd)
CNoisePreview::CNoisePreview()
{
}

CNoisePreview::~CNoisePreview()
{
}


BEGIN_MESSAGE_MAP(CNoisePreview, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()



// CNoisePreview message handlers


void CNoisePreview::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	RECT rect;
	GetClientRect(&rect);
	CRgn rgn;
	rgn.CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
	dc.SelectClipRgn(&rgn);
	dc.SelectStockObject(WHITE_BRUSH);
	dc.Rectangle(&rect);
	dc.MoveTo(rect.left,(rect.bottom-rect.top)/2);
	dc.LineTo(rect.right,(rect.bottom-rect.top)/2);

	dc.MoveTo((rect.right-rect.left)/2,rect.top);
	dc.LineTo((rect.right-rect.left)/2,rect.bottom);

	// Draw Noise
	CPen pen(PS_SOLID,1,RGB(255,0,0));
	CPen* pOldPen = dc.SelectObject(&pen);

	for(int i=0; i<m_points.size(); i++)
	{
		int y = m_points[i]*50;
		if (i==0)
			dc.MoveTo(i,(rect.bottom-rect.top)/2-y);
		else
			dc.LineTo(i,(rect.bottom-rect.top)/2-y);
	}
	dc.SelectObject(pOldPen);

}
