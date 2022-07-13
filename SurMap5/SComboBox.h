#ifndef __S_COMBO_BOX_H_INCLUDED__
#define __S_COMBO_BOX_H_INCLUDED__

class CSComboBox : public CExtComboBox
{
	DECLARE_DYNAMIC(CSComboBox)

public:
	CSComboBox();
	virtual ~CSComboBox();

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
	virtual void MeasureItem(LPMEASUREITEMSTRUCT /*lpMeasureItemStruct*/);
	virtual void DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct);
};

#endif
