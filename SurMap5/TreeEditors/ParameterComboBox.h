#ifndef __PARAMETER_COMBO_BOX_H_INCLUDED__
#define __PARAMETER_COMBO_BOX_H_INCLUDED__
#include "..\Render\inc\Umath.h"
#include "..\Units\Parameters.h"
#include "TreeComboBox.h"

class CParameterComboBox : public CTreeComboBox {
public:
    ParameterValue& selectedValue();
    BOOL Create(ParameterValueReference reference, DWORD style, const CRect& rect, CWnd* parent, UINT id);

	void hideDropDown(bool byLostFocus = false);

	void onItemSelected(HTREEITEM item);
private:
	ParameterValueReference initialValue_;
	StaticMap<HTREEITEM, ParameterValue*> values_;
};

#endif
