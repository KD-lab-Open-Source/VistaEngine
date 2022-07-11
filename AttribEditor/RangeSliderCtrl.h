#ifndef __RANGE_SLIDER_CTRL_H_INCLUDED__
#define __RANGE_SLIDER_CTRL_H_INCLUDED__

#include "Range.h"
#include "TreeInterface.h"

#define RANGE_SLIDER_CTRL_CLASSNAME "SurMapRangeSliderCtrl"

class CRangeSliderCtrl: public CWnd
{
    DECLARE_MESSAGE_MAP()
public:
    CRangeSliderCtrl();
    virtual ~CRangeSliderCtrl () {}

    enum Type{
        TYPE_INTEGER,
        TYPE_FLOAT
    };

    BOOL Create(DWORD style, const CRect& rect, CWnd* parent_wnd, UINT id);

    CSliderCtrl* slider(){ return &slider_; }
    float value() const;

    virtual void SetFont(CFont* pFont);
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
private:
    Type type_;
    CSliderCtrl slider_;
    CEdit edit_;
};

#endif
