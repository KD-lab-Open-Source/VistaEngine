// based on the control by Mark Jackson <mark@mjsoft.continue.uk> and James R. Twine.
#ifndef __ATTRIB_COLOR_COMBO_BOX_H_INCLUDED__
#define __ATTRIB_COLOR_COMBO_BOX_H_INCLUDED__
#include "AttribComboBox.h"
void DDX_ColourPickerCB( CDataExchange *pDX, int nIDC, COLORREF& prgbColor );


class CAttribColorComboBox : public CComboBox
{
// Construction
public:
	CAttribColorComboBox();
	virtual	~CAttribColorComboBox();

// Attributes
private:
	CString m_strColourName;

private:
	void Initialise();

public:
	COLORREF GetSelectedColourValue();

	void AddColour( CString strName, COLORREF crColour );
// Overrides
	protected:
	virtual void PreSubclassWindow();
	virtual void DrawItem( LPDRAWITEMSTRUCT lpDrawItemStruct );
protected:
	afx_msg void OnCbnCloseup();
	afx_msg void OnCbnSetfocus();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};
#endif
