#include "stdafx.h"
#include ".\FormationEditor.h"

#include "AttributeSquad.h"

#include "..\Util\mfc\MemDC.h"

#include <vector>

const float ZoomTable [] = { 8.0f, 4.0f, 3.0f, 
					         2.0f, 1.0f, 0.5f, 0.25f };

enum { MaxZoom = sizeof (ZoomTable) / sizeof (ZoomTable [0]) };

// CFormationEditor

IMPLEMENT_DYNAMIC(CFormationEditor, CWnd)

CFormationEditor::CFormationEditor()
: view_offset_ (Vect2f::ZERO)
, view_size_ (Vect2f::ZERO)
, zoom_index_ (4)
, scrolling_ (false)
, moving_ (false)
, selected_cell_ (-1)
, click_point_ (Vect2f::ZERO)
, pattern_(*new FormationPattern)
, showTypeNames_(false)
{
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if( !::GetClassInfo (hInst, className(), &wndclass) )
	{
		wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|WS_TABSTOP;
		wndclass.lpfnWndProc	= ::DefWindowProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hInst;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= AfxGetApp ()->LoadStandardCursor (IDC_ARROW);
		wndclass.hbrBackground	= reinterpret_cast<HBRUSH> (COLOR_WINDOW);
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= className();

		if (!AfxRegisterClass (&wndclass))
			AfxThrowResourceException ();
	}
}

CFormationEditor::~CFormationEditor()
{
	delete &pattern_;
}


BEGIN_MESSAGE_MAP(CFormationEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_DESTROY()

	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()

	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()

	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()

	ON_WM_KEYDOWN()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_POPUP_DELETE, OnFormationCellDelete)
END_MESSAGE_MAP()


Vect2f CFormationEditor::coordsToScreen (const Vect2f& pos) const
{
	return (pos + view_offset_) * viewScale () + Vect2f (view_size_) * 0.5f;
}

Vect2f CFormationEditor::coordsFromScreen (const Vect2f& pos) const
{
	return (pos - Vect2f (view_size_) * 0.5f) / viewScale () - view_offset_;
}

void CFormationEditor::drawCircle (CDC& dc, const Vect2f& pos, float radius, const sColor4c& color, int outline_width)
{
	Vect2f center = coordsToScreen (pos);
	float rad = radius * viewScale();

	CBrush brush (RGB (color.r, color.g, color.b));
	CPen pen (PS_SOLID, outline_width, RGB (0, 0, 0));

	CPen* old_pen = dc.SelectObject (&pen);
	CBrush* old_brush = dc.SelectObject (&brush);
	dc.Ellipse (round (center.x - rad), round (center.y - rad), round (center.x + rad), round (center.y + rad));
	
	dc.SelectObject (old_pen);
	dc.SelectObject (old_brush);
}

void CFormationEditor::drawLine (CDC& dc, const Vect2f& _start, const Vect2f& _end, const sColor4c& _color)
{
	Vect2i start = coordsToScreen (_start);
	Vect2i end = coordsToScreen (_end);
	CPen pen (PS_SOLID, 1, RGB (_color.r, _color.g, _color.b));
	CPen* old_pen = dc.SelectObject (&pen);
	dc.MoveTo (start.x, start.y);
	dc.LineTo (end.x, end.y);
	dc.SelectObject (old_pen);
}

void CFormationEditor::redraw (CDC& dc)
{
	CRect rt;
	GetClientRect (&rt);
	view_size_.set (rt.Width(), rt.Height());
	dc.FillSolidRect (&rt, RGB (255, 255, 255));

	sColor4c color1 (216, 216, 216);
	sColor4c color2 (128, 128, 128);
	for (float x = -200.0f; x <= 200.0f + FLT_COMPARE_TOLERANCE; x+= 10.0f) {
		drawLine (dc, Vect2f (x, -200.0f), Vect2f (x, 200.0f), round (x) % 100 == 0 ? color2 : color1);
		drawLine (dc, Vect2f (-200.0f, x), Vect2f (200.0f, x), round (x) % 100 == 0 ? color2 : color1);
	}
	
	FormationPattern::Cells::const_iterator it;
	int index = 0;
	CFont* old_font = dc.SelectObject (&m_fntPosition);
	FOR_EACH (pattern_.cells(), it) {
		const FormationPattern::Cell& cell = *it;

		Vect2f pos = cell;
		if (GetAsyncKeyState (VK_CONTROL) && index == selected_cell_) {
			float grid_size = 10.0f;
			pos.set (round (cell.x / grid_size) * grid_size,
			 	     round (cell.y / grid_size) * grid_size);
		}

		drawCircle (dc, pos, cell.type->radius(), cell.type->color(), index == selected_cell_ ? 3 : 1);
		Vect2f scr_pos = coordsToScreen (pos);
		CString str;
		if(showTypeNames_)
			str.Format ( " %i (%s) ", index + 1, cell.type->c_str());
		else
			str.Format ( " %i ", index + 1);
		CRect rt;
		dc.DrawText (str, &rt, DT_CALCRECT);
		dc.FillSolidRect (round(scr_pos.x) - rt.Width() / 2 - 2, round(scr_pos.y) - rt.Height() / 2 - 1,
						  rt.Width() + 4, rt.Height() + 2, RGB (0, 0, 0));
		dc.SetBkMode (OPAQUE);
		dc.SetBkColor (RGB (255, 255, 255));
		dc.TextOut (round(scr_pos.x) - rt.Width() / 2, round(scr_pos.y) - rt.Height() / 2, str);
		//dc.DrawText(str, &rt, 0);


		++index;
	}
    
	index = 0;
	FOR_EACH (pattern_.cells(), it) {
		const FormationPattern::Cell& cell = *it;

		Vect2f pos = cell;
		Vect2f scr_pos = coordsToScreen (pos);
		CString str;
		if(showTypeNames_)
			str.Format ( " %i (%s) ", index + 1, cell.type->c_str());
		else
			str.Format ( " %i ", index + 1);
		CRect rt;
		dc.DrawText (str, &rt, DT_CALCRECT);
		dc.FillSolidRect (round(scr_pos.x) - rt.Width() / 2 - 2, round(scr_pos.y) - rt.Height() / 2 - 1,
						  rt.Width() + 4, rt.Height() + 2, RGB (0, 0, 0));
		dc.SetBkMode (OPAQUE);
		dc.SetBkColor (RGB (255, 255, 255));
		dc.TextOut (round(scr_pos.x) - rt.Width() / 2, round(scr_pos.y) - rt.Height() / 2, str);

		++index;
	}

	dc.SelectObject (old_font);
}

void CFormationEditor::OnPaint()
{
	CPaintDC paintDC(this);
	CMemDC dc (&paintDC);

    redraw (dc);
}

void CFormationEditor::OnSize(UINT nType, int cx, int cy)
{
	view_size_.set (cx, cy);
	CWnd::OnSize(nType, cx, cy);
}

void CFormationEditor::OnDestroy()
{
	CWnd::OnDestroy();
}

void CFormationEditor::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetFocus ();

	click_point_ = coordsFromScreen (Vect2f (point.x, point.y));
	
	CWnd::OnRButtonDown(nFlags, point);
}


void CFormationEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	Vect2f delta = coordsFromScreen (Vect2f (point.x, point.y)) - click_point_;
	if (nFlags & MK_RBUTTON) {
		scrolling_ = true;
		view_offset_ += delta;
		Invalidate ();
	}
	if (moving_ && selected_cell_ >= 0) {
		pattern_.cells_ [selected_cell_] += delta;
		Invalidate ();
	}
	click_point_ = coordsFromScreen (Vect2f (point.x, point.y));
	CWnd::OnMouseMove(nFlags, point);
}

void CFormationEditor::selectCellUnderPoint (const Vect2f& v) 
{
	selected_cell_ = -1;

	FormationPattern::Cells::const_iterator it;
	int index = 0;
	FOR_EACH (pattern_.cells(), it) {
		const FormationPattern::Cell& cell = *it;
		float dist = cell.distance (v);
		if (dist <= cell.type->radius()) {
			selected_cell_ = index;
		}
		++index;
	}
}

namespace{
inline void FillComboMenu (const char* comboList, CMenu* parentMenu, UINT firstMenuID) {
	ComboStrings::iterator it;
	ComboStrings strings;
	splitComboList(strings, comboList);
	unsigned int uPos = 0;
	FOR_EACH(strings, it)
		parentMenu->AppendMenu (MF_STRING, firstMenuID + uPos++, it->c_str ());

	parentMenu->RemoveMenu (firstMenuID, MF_BYCOMMAND);
}
}

void CFormationEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	click_point_ = coordsFromScreen (Vect2f (point.x, point.y));

	if (scrolling_) {
		scrolling_ = false;
	} else {
		selectCellUnderPoint (coordsFromScreen (Vect2f (point.x, point.y)));

		if (selected_cell_ == -1) {
			CMenu menu;
			menu.LoadMenu (IDR_FORMATION_MENU);

			CPoint pt;
			GetCursorPos (&pt);
			const char* comboList = UnitFormationTypes::instance().comboList();
			FillComboMenu (comboList, menu.GetSubMenu(0)->GetSubMenu (0), IDM_TYPE_FIRST_ITEM);

			menu.GetSubMenu (0)->TrackPopupMenu (0, pt.x, pt.y, this, 0);
		} else {
			int index = indexInComboListString (UnitFormationTypes::instance().comboList(), pattern_.cells_[selected_cell_].type->c_str());

			Invalidate ();

			CMenu menu;
			menu.LoadMenu (IDR_FORMATION_CELL_MENU);

			CPoint pt;
			GetCursorPos (&pt);
			const char* comboList = UnitFormationTypes::instance().comboList();
			FillComboMenu (comboList, menu.GetSubMenu(0)->GetSubMenu (0), IDM_TYPE_FIRST_ITEM);
			menu.GetSubMenu (0)->GetSubMenu (0)->CheckMenuItem (index, MF_CHECKED | MF_BYPOSITION);

			menu.GetSubMenu (0)->TrackPopupMenu (0, pt.x, pt.y, this, 0);
		}
	}

    CWnd::OnRButtonUp(nFlags, point);
}

BOOL CFormationEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	SetFocus ();

	if (zDelta < 0) {
		zoom_index_ = min (MaxZoom - 1, zoom_index_ + 1);
	} else {
		zoom_index_ = max (0, zoom_index_ - 1);
	}

	Invalidate (FALSE);

	return CWnd::OnMouseWheel(nFlags, zDelta, point);
}

void CFormationEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus ();

	click_point_ = coordsFromScreen (Vect2f (point.x, point.y));

	selectCellUnderPoint (coordsFromScreen (Vect2f (point.x, point.y)));
	if (selected_cell_ != -1) {
		moving_ = true;
	}

	Invalidate ();

	CWnd::OnLButtonDown(nFlags, point);
}

void CFormationEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonUp(nFlags, point);
	if (moving_) {
		if (selected_cell_ >= 0) {
			FormationPattern::Cell& cell = pattern_.cells_ [selected_cell_];
			float grid_size = 10.0f;
			if (nFlags & MK_CONTROL) {
				cell.set (round (cell.x / grid_size) * grid_size, round (cell.y / grid_size) * grid_size);
			}
		}
		moving_ = false;
	}
	click_point_ = coordsFromScreen (Vect2f (point.x, point.y));
}

void CFormationEditor::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CFormationEditor::OnEraseBkgnd(CDC* pDC)
{
    //return CWnd::OnEraseBkgnd(pDC);
	return FALSE;
}

float CFormationEditor::viewScale () const
{
	return ZoomTable [zoom_index_];
}


BOOL CFormationEditor::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam >= IDM_TYPE_FIRST_ITEM &&
		wParam < IDM_TYPE_FIRST_ITEM + UnitFormationTypes::instance().strings().size())
	{
		int index = wParam - IDM_TYPE_FIRST_ITEM;
		if (selected_cell_ != -1)  {
			FormationPattern::Cell& cell = pattern_.cells_ [selected_cell_];
			cell.type = UnitFormationTypeReference ((UnitFormationTypes::instance().strings().begin() + index)->c_str());
		} else {
			FormationPattern::Cell cell;
			cell.set (click_point_.x, click_point_.y);
			cell.type = UnitFormationTypeReference ((UnitFormationTypes::instance().strings().begin() + index)->c_str());
			pattern_.cells_.push_back (cell);
			selected_cell_ = pattern_.cells_.size () - 1;
		}

		Invalidate ();
	}

	return CWnd::OnCommand(wParam, lParam);
}

void CFormationEditor::PreSubclassWindow()
{
	m_fntPosition.CreateFont (12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
							  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Verdana");

	CWnd::PreSubclassWindow();
}

void CFormationEditor::OnFormationCellDelete()
{
	xassert (selected_cell_ >= 0);
	if (selected_cell_ >= 0) {
		pattern_.cells_.erase (pattern_.cells_.begin () + selected_cell_);
		selected_cell_ = -1;
		Invalidate ();
	}
}

void CFormationEditor::setPatternName(const char* name)
{
	pattern_.setName(name); 
}

void CFormationEditor::setPattern(const FormationPattern& pattern)
{
	pattern_ = pattern;
}
