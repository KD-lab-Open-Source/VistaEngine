#ifndef __ATTRIB_EDITOR_CTRL_H_INCLUDED__
#define __ATTRIB_EDITOR_CTRL_H_INCLUDED__

#include "XTL/sigslot.h"
#include "Serialization/Serializer.h"
#include "Serialization/ComboStrings.h"

namespace kdw{
	class Win32Proxy;
	class PropertyTree;
	class PropertyRow;
};

class Serializer;
class Archive;

class CAttribEditorCtrl : public CWnd, public sigslot::has_slots{
public:
	typedef kdw::PropertyRow* ItemType;
	enum{
		COLUMN_NAME,
		COLUMN_VALUE
	};
	enum{
		EXPAND_ALL      = 1 << 0,
        HIDE_ROOT_NODE  = 1 << 1,
        NO_DIGEST       = 1 << 2,
        COMPACT         = 1 << 3,
        AUTO_SIZE       = 1 << 4,
        DEEP_EXPAND     = 1 << 5,
        NO_HEADER       = 1 << 6,
        DISABLE_MENU    = 1 << 6,
	};

	CAttribEditorCtrl();

	static const char* className() { return "SurMapAttribEditorCtrl"; }
	BOOL Create(DWORD style, const CRect& rect, CWnd* parentWnd = 0, UINT id = 0);
	int OnCreate(LPCREATESTRUCT createStruct);
	afx_msg void OnSize(UINT type, int cx, int cy);
	virtual int initControl();

	void attachSerializer(Serializer& s);
	void detachData();

	void loadExpandState(XBuffer& file);
    void saveExpandState(XBuffer& file);
	void serialize(Archive& ar);

	void mixIn(Serializer& holder);
	void showMix();

    void setStyle(int newStyle);
    int  style() { return style_; }

	bool getItemPath(ComboStrings& result, ItemType item);
	bool selectItemByPath(const ComboStrings& path, bool resetSelection = true);

	virtual void beforeResave() {}
	virtual void onChanged(){}

	kdw::PropertyTree& tree() { return *propertyTree_; }

	DECLARE_MESSAGE_MAP()
protected:
	void onBeforeApply();

	kdw::Win32Proxy* proxy_;
	kdw::PropertyTree* propertyTree_;
	Serializers sers_;
	int style_;
};

#endif
