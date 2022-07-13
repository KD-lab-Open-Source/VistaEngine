#include "stdafx.h"
#include "..\Util\mfc\MemDC.h"
#include "EditArchive.h"

#include "AttribEditorInterface.h"
#include "..\TriggerEditor\TriggerExport.h"
#include "ConditionEditor.h"
#include "..\..\AttribEditor\AttribEditorCtrl.h" // для TreeNodeClipboard
#include "..\Util\Conditions.h"
#include "Dictionary.h"
using namespace std;

// ---------------------------------------------------------------------------

bool removeCondition(ConditionSwitcher* switcher, Condition* condition)
{
	xassert (switcher && condition);

	ConditionSwitcher::Conditions::iterator it = std::find(switcher->conditions.begin(), switcher->conditions.end(), condition);
	if(it != switcher->conditions.end()){
		Condition* oldValue = *it;
		switcher->conditions.erase(it);
        return true;
	}
	return false;
}

bool replaceCondition(ConditionSwitcher* switcher, Condition* from, Condition* to)
{
	xassert(switcher && from && to);

	ConditionSwitcher::Conditions::iterator it = std::find(switcher->conditions.begin(), switcher->conditions.end(), from);
	if(it != switcher->conditions.end()){
		Condition* oldValue = *it;
		*it = to;
        return true;
	}

	return false;
}

void addCondition(ConditionSwitcher* switcher, Condition* condition)
{
	switcher->conditions.push_back(condition);
}

void cleanUp(ShareHandle<Condition>& condition)
{
	if(ConditionSwitcher* switcher = dynamic_cast<ConditionSwitcher*>(&*condition)) {
		int count = switcher->conditions.size();
		if (count == 1) {
			ShareHandle<Condition> stored = condition;
			condition = switcher->conditions[0];
		} else {
			for(int i = 0; i < count; ++i)
				cleanUp(switcher->conditions[i]);
		}
	}
}

// ---------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(CConditionEditor, CWnd)

CConditionEditor::CConditionEditor()
: scrolling_(false)
, moving_(false)
, draging_(false)
, click_point_(Vect2f::ZERO)
, cursorPosition_(Vect2f::ZERO)
, newConditionIndex_ (0)
, selectedCondition_ (0)
, dragedCondition_ (0)
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

CConditionEditor::~CConditionEditor()
{
	//if(condition_.condition)
	//	condition_.condition->addRef();
}


BEGIN_MESSAGE_MAP(CConditionEditor, CWnd)
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
	ON_WM_QUERYDRAGICON()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SETCURSOR()
	ON_WM_CREATE()
END_MESSAGE_MAP()

float CConditionEditor::pixelWidth () const
{
	return 1.0f / view_.viewScale() ;
}

float CConditionEditor::pixelHeight () const
{
	return 1.0f / view_.viewScale() ;
}

void CConditionEditor::drawCircle (CDC& dc, const Vect2f& pos, float radius, const sColor4c& color, int outline_width)
{
	Vect2f center = view_.coordsToScreen (pos);
	float rad = radius * view_.viewScale();

	CBrush brush (RGB (color.r, color.g, color.b));
	CPen pen (PS_SOLID, outline_width, RGB (0, 0, 0));

	CPen* old_pen = dc.SelectObject (&pen);
	CBrush* old_brush = dc.SelectObject (&brush);
	dc.Ellipse (round (center.x - rad), round (center.y - rad), round (center.x + rad), round (center.y + rad));
	
	dc.SelectObject (old_pen);
	dc.SelectObject (old_brush);
}

void CConditionEditor::drawLine (CDC& dc, const Vect2f& _start, const Vect2f& _end, const sColor4c& _color)
{
	Vect2i start (view_.coordsToScreen (_start));
	Vect2i end (view_.coordsToScreen (_end));
	CPen pen (PS_SOLID, 1, RGB(_color.r, _color.g, _color.b));
	CPen* old_pen = dc.SelectObject (&pen);
	dc.MoveTo (start.x, start.y);
	dc.LineTo (end.x, end.y);
	dc.SelectObject (old_pen);
}


void CConditionEditor::fillRectangle (CDC& dc, const Rectf& rect, const sColor4c& color)
{
	Vect2i left_top (view_.coordsToScreen (Vect2f(rect.left(), rect.top())));
	Vect2i right_bottom (view_.coordsToScreen (Vect2f(rect.right(), rect.bottom())));
	CRect rt (left_top.x, left_top.y, right_bottom.x, right_bottom.y);
	dc.FillSolidRect (&rt, RGB(color.r, color.g, color.b));
}

void CConditionEditor::drawRectangle (CDC& dc, const Rectf& rect, const sColor4c& color)
{
	drawLine (dc, Vect2f(rect.left(), rect.top()), Vect2f(rect.right(), rect.top()), color);
	drawLine (dc, Vect2f(rect.left(), rect.bottom()), Vect2f(rect.right(), rect.bottom()), color);
	drawLine (dc, Vect2f(rect.left(), rect.top()), Vect2f(rect.left(), rect.bottom()), color);
	drawLine (dc, Vect2f(rect.right(), rect.top()), Vect2f(rect.right(), rect.bottom()), color);
}

void CConditionEditor::drawText (CDC& dc, const Rectf& rect, const char* text, TextAlign text_align)
{
	Vect2i p1 (view_.coordsToScreen (Vect2f(rect.left(), rect.top())));
	Vect2i p2 (view_.coordsToScreen (Vect2f(rect.right(), rect.bottom())));
	CRect rt(p1.x, p1.y, p2.x, p2.y);
	int flags = DT_VCENTER | DT_SINGLELINE;
	if (text_align == ALIGN_LEFT) {
		flags |= DT_LEFT;
	} else if (text_align == ALIGN_CENTER) {
		flags |= DT_CENTER;
	} else {
		flags |= DT_RIGHT;
	}
	dc.DrawText (text, &rt, flags);
}

void CConditionEditor::drawSlot (CDC& dc, const ConditionSlot& slot) 
{
	sColor4c border_color(0, 0, 100, 255);
	sColor4c fill_color(240, 240, 240, 255);
	
	Rectf rect(slot.rect());
	Rectf text_rect(slot.text_rect());
	Rectf not_rect (slot.not_rect());

	if (slot.condition() == selectedCondition_) {
		if (draging_) {
			fill_color = sColor4c(255, 200, 200, 255);
		} else {
			fill_color = sColor4c(200, 255, 200, 255);
		}
	}
	fillRectangle (dc, rect, fill_color);
	drawRectangle (dc, rect, border_color);

	float textHeight_ = ConditionSlot::TEXT_HEIGHT;
	int size = slot.height();
	float border = (rect.height() / float(size) - 2.0f * pixelWidth() - textHeight_)*0.5f;
	if (size > 1) {
		drawText (dc, text_rect, slot.name(), ALIGN_CENTER);
	} else {
		drawText (dc, text_rect, slot.name(), ALIGN_LEFT);
	}
	/*
	fillRectangle(dc, not_rect, slot.inversed_ ? sColor4c(255, 0, 0) : sColor4c(255, 255, 255));
	drawRectangle(dc, not_rect, sColor4c(0, 0, 0));
	*/
	drawCircle(dc, not_rect.center(), not_rect.width() * 0.5f, slot.inverted() ? sColor4c(255, 0, 0) : sColor4c(255, 255, 255), 1);
	if (slot.inverted()) {
		dc.SetTextColor(RGB(255, 255, 255));		
		drawText (dc, not_rect, "!", ALIGN_CENTER);
		dc.SetTextColor(RGB(0, 0, 0));		
	} else {

	}

	ConditionSlot::Slots::const_iterator it;
	int index = 0;
	FOR_EACH (slot.children(), it) {
		drawSlot (dc, *it);
		index += it->height();
	}

}

void CConditionEditor::redraw (CDC& dc)
{
	CRect rt;
	GetClientRect (&rt);
	view_.setSize (rt.Width(), rt.Height());
	dc.FillSolidRect (&rt, GetSysColor(COLOR_APPWORKSPACE));

	CFont* old_font = dc.SelectObject (&m_fntPosition);
	dc.SetBkMode (TRANSPARENT);
	currentSlots_.calculateRect(Vect2f::ZERO);
	drawSlot (dc, currentSlots_);
	dc.SelectObject (old_font);
}

void CConditionEditor::OnPaint()
{
	CPaintDC paintDC(this);
	CMemDC dc (&paintDC);

    redraw (dc);
}

void CConditionEditor::OnSize(UINT nType, int cx, int cy)
{
    view_.setSize (cx, cy);
	CWnd::OnSize(nType, cx, cy);
}

void CConditionEditor::OnDestroy()
{
	CWnd::OnDestroy();
}

void CConditionEditor::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetFocus ();

	click_point_ = view_.coordsFromScreen (Vect2f (point.x, point.y));
	
	CWnd::OnRButtonDown(nFlags, point);
}


namespace {

template<class Pred>
void forEachSlot (ConditionSlot& slot, Pred pred)
{
	pred (slot);
	ConditionSlot::Slots::iterator it;
	FOR_EACH(slot.children(), it)
		forEachSlot(*it, pred);
}

template<class Pred>
ConditionSlot* findSlot(ConditionSlot& slot, Pred pred){
	if(pred(slot))
		return &slot;

	ConditionSlot::Slots::iterator it;
	FOR_EACH(slot.children(), it){
		if (ConditionSlot* result = findSlot(*it, pred))
			return result;
	}
	return 0;
}

struct SlotWithCondition{
	SlotWithCondition(const Condition* condition)	: condition_(condition) 	{}
	bool operator()(const ConditionSlot& slot){
		return (slot.condition() == condition_);
	}
	const Condition* condition_;
};

struct SelectIfUnderPoint {
	SelectIfUnderPoint(Condition*& selection, const Vect2f& pt)
		: point_(pt)
		, selection_(selection)
	{}
	void operator()(ConditionSlot& slot){
		if(slot.rect().point_inside(point_)){
            selection_ = slot.condition();
		}
	}
	Vect2f point_;
	Condition*& selection_;
};

};

void CConditionEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	SetFocus();

	Vect2f mouse_point = view_.coordsFromScreen(Vect2f (point.x, point.y));
	Vect2f delta = mouse_point - click_point_;

	cursorPosition_ = mouse_point;

	if(draging_){
		trackMouse (mouse_point, true);
		if(canBeDropped(point))
			SetCursor(::LoadCursor(0, MAKEINTRESOURCE(IDC_HAND)));
		else
			SetCursor(::LoadCursor(0, MAKEINTRESOURCE(IDC_NO)));
		SetCapture ();
	}
	else{
		if((nFlags & MK_LBUTTON) && (mouse_point - click_point_).norm() >= 0.05){
			dragedCondition_ = selectedCondition_;
			trackMouse (mouse_point, dragedCondition_ != 0);
			SetCapture ();
		}
		else{
			trackMouse(mouse_point);
		}
	}
 	if(nFlags & MK_RBUTTON){
		scrolling_ = true;
		view_.addOffset(delta);
	}
	CWnd::OnMouseMove(nFlags, point);
}

void CConditionEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	click_point_ = view_.coordsFromScreen (Vect2f (point.x, point.y));

	if (scrolling_) {
		scrolling_ = false;
	} else {
	}

    CWnd::OnRButtonUp(nFlags, point);
}

BOOL CConditionEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	SetFocus();

	if(zDelta > 0){
        view_.zoom(true);
		createFont ();
	}
	else{
        view_.zoom(false);
		createFont ();
	}

	Invalidate(FALSE);
	return CWnd::OnMouseWheel(nFlags, zDelta, point);
}

void CConditionEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus ();

	click_point_ = view_.coordsFromScreen(Vect2f(point.x, point.y));

	if(selectedCondition_){
		ConditionSlot* slot = findSlot(currentSlots_, SlotWithCondition(selectedCondition_));
		if(slot->not_rect().point_inside (click_point_))
			slot->invert();		
	}

	Invalidate ();

	CWnd::OnLButtonDown(nFlags, point);
}

void CConditionEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonUp(nFlags, point);
	click_point_ = view_.coordsFromScreen (Vect2f (point.x, point.y));

	if (draging_) {
		if (dragedCondition_) {
			onDrop (point, dragedCondition_);
			dragedCondition_ = 0;
		} else {
			onDrop (point, newConditionIndex_);
		}

		draging_ = false;
	}
	if (GetCapture() == this) {
		ReleaseCapture ();
	}
}

ShareHandle<Condition> CConditionEditor::pasteCondition()
{
	TreeNodeClipboard& clipboard = attribEditorInterface().clipboard();

	if(!clipboard.empty()){
		ShareHandle<Condition> newCondition;

		Serializeable ser(newCondition);
		clipboard.get(ser, GetSafeHwnd());
		return newCondition;
	}
	else
		return 0;
}

void CConditionEditor::copyCondition(Condition* condition)
{
	// делаем копию condition, т.к. при сериализации на ShareHandleBase должна быть всего одна ссылка
	xassert(condition);
	EditOArchive tempOArchive;
	tempOArchive.serialize(*condition, "condition", 0);
	int index = indexInComboListString(triggerInterface().conditionComboList(), triggerInterface().conditionName(*condition));
	xassert(index >= 0);
	ShareHandle<Condition> conditionCopy = triggerInterface().createCondition(index);
	xassert(conditionCopy);
	EditIArchive tempIArchive(tempOArchive);
	tempIArchive.serialize(*conditionCopy, "condition", 0);

	Serializeable ser(conditionCopy);
	attribEditorInterface().clipboard().set(ser, GetSafeHwnd());
}

Condition* CConditionEditor::conditionUnderMouse()
{
	Condition* condition = 0;
	forEachSlot(currentSlots_, SelectIfUnderPoint(condition, cursorPosition_));
	return condition;
}

void CConditionEditor::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	GetAsyncKeyState(VK_CONTROL);
	bool controlPressed = GetAsyncKeyState(VK_CONTROL);

	Condition* condition = conditionUnderMouse();

	switch(nChar){
	case 'C':
		if(condition)
			copyCondition(condition);
		break;
	case 'V':
		if(ShareHandle<Condition> newCondition = pasteCondition()){
			if(condition_.condition){
				ConditionSlot* slot = findSlot(currentSlots_, SlotWithCondition(condition));
				if(slot){
					if(slot->parent()){
						ConditionSwitcher* switcher = safe_cast<ConditionSwitcher*>(slot->parent()->condition());
						replaceCondition(switcher, condition, newCondition);
					}
					else{
						condition_.condition = newCondition;
					}
				}
			}
			else{
				condition_.condition = newCondition;
			}
			cleanUp(condition_.condition);
			currentSlots_.create(condition_.condition);
			Invalidate(FALSE);
		}
		break;
	};
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CConditionEditor::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void CConditionEditor::createFont ()
{
	m_fntPosition.DeleteObject();
	m_fntPosition.CreateFont (round(ConditionSlot::TEXT_HEIGHT * view_.viewScale()), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
							  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma");
}


void CConditionEditor::PreSubclassWindow()
{
	createFont ();
	CWnd::PreSubclassWindow();
}

ConditionSlot::ConditionSlot()
	: color_(1.0f, 0.0f, 1.0f, 1.0f)
	, condition_(0)
{
}


inline const char* toConstChar(const char* name) {	return name; }
inline const char* toConstChar(const std::string& name) { return name.c_str(); }

// TODO: убрать копию
static void assignNodeValue(std::string& out, const TreeNode* node)
{
    if(node->editor())
        out = node->editor()->nodeValue();
    else{
        if(node->editType() == TreeNode::COMBO){
            out = TRANSLATE(node->value());
        }
		else if(node->editType() == TreeNode::POLYMORPHIC){
            out = TRANSLATE(node->value());
		}
        else if(node->editType() == TreeNode::COMBO_MULTI){
            ComboStrings values;
            splitComboList(values, node->value());
            ComboStrings::iterator it;
            FOR_EACH(values, it){
                *it = TRANSLATE(it->c_str());
            }
            joinComboList(out, values);
        }
        else
            out = node->value();
    }
}
AttribEditorInterface& attribEditorInterface();

void treeNodeDigest(std::string& digest, const TreeNode* root)
{
	xassert(root);
	TreeEditor* editor = attribEditorInterface().createTreeEditor(root->type());
	if(root->empty() || editor){
		if(editor){
			editor->onChange(*root);
			std::string value;
			assignNodeValue(value, root);
			digest += value;
			bool hideContent = editor->hideContent();
			editor->free();
			editor = 0;
			if(hideContent)
				return;
		}
		else {
			std::string value;
			assignNodeValue(value, root);
			digest += value;
			return;
		}
	}
	TreeNode::const_iterator it;
	
	bool firstItem = true;
	FOR_EACH(*root, it){
		const TreeNode* node = *it;		

		if(toConstChar(node->name())[0] == '&'){
			if(firstItem){
				digest += "{ ";
				firstItem = false;
			} 
			else{
				if(!digest.empty())
					digest += ", ";
			}
			treeNodeDigest(digest, node);

		}
	}
	if(!firstItem)
		digest += " }";
}

std::string conditionName(Condition* condition){
	std::string result = TRANSLATE(ClassCreatorFactory<Condition>::instance().nameAlt(typeid(*condition).name()));
	std::string digest;

	EditOArchive oa;
	condition->serialize(oa);
		
	treeNodeDigest(digest, oa.rootNode());
	if(!digest.empty()){
		result += " ";
		result += digest;
	}
	return result;
}

void ConditionSlot::create(Condition* condition, int offset, ConditionSlot* parent)
{
    condition_ = condition;
    offset_ = offset;
	parent_ = parent;
	children_.clear();

	if(condition){
		if(typeid(*condition) == typeid(ConditionSwitcher)){
			ConditionSwitcher* switcher = static_cast<ConditionSwitcher*>(condition);
			ConditionSwitcher::Conditions::iterator it;
			FOR_EACH(switcher->conditions, it){
				if(Condition* cond = *it){
					children_.push_back(ConditionSlot());
					children_.back().create(cond, offset + 1, this);
				}
			}
			name_ = TRANSLATE(getEnumNameAlt(switcher->type));
		}
		else{
			name_ = conditionName(condition).c_str();
		}

	}
	else{
		name_ = TRANSLATE("Выполняется всегда");
	}	
} 


float ConditionSlot::SLOT_HEIGHT = 25.0f;
float ConditionSlot::TEXT_HEIGHT = 12.0f;

int ConditionSlot::height() const
{
    int height = 0;
    Slots::const_iterator it;
    FOR_EACH (children_, it) {
        height += it->height();
    }
    return height ? height : 1;
}

int ConditionSlot::depth() const
{
	int depth = 0;
	ConditionSlot::Slots::const_iterator it;
	FOR_EACH (children_, it) {
        depth = max(depth, it->depth() + 1);
	}
	return depth;
}

void ConditionSlot::calculateRect(const Vect2f& point, int parent_depth)
{
	int size = height();
	float height = float(size) * SLOT_HEIGHT;
	float border = (SLOT_HEIGHT - TEXT_HEIGHT) * 0.5f;
	//int depth = this->depth();
	if(parent_depth == 0)
		parent_depth = depth();

	sColor4c border_color(0, 0, 100, 255);
	rect_.set(point.x, point.y, 300.0f + (parent_depth) * SLOT_HEIGHT, height);

	ConditionSlot::Slots::iterator it;
	int index = 0;
	FOR_EACH (children_, it) {
		Vect2f offset (SLOT_HEIGHT, float(index) * SLOT_HEIGHT);
		it->calculateRect(point + offset, parent_depth - 1);
		index += it->height();
	}
	//rect_.width(rect_.width() + float(depth) * SLOT_HEIGHT);

	float text_offset = border * 2.0f + TEXT_HEIGHT;
	if (index > 1) { // switcher
		text_rect_.set (rect_.left(), rect_.top() + text_offset, 
			            SLOT_HEIGHT, rect_.height() - text_offset - border);
	} else {
		text_rect_.set (rect_.left() + text_offset, rect_.top(),
			            rect_.width()  - text_offset, rect_.height());
	}
	not_rect_.set(rect_.left() + border, rect_.top() + border, TEXT_HEIGHT, TEXT_HEIGHT);
}

void ConditionSlot::invert()
{
	if(condition_) {
		condition_->setInverted(!condition_->inverted());
	}
}

bool CConditionEditor::canBeDropped(Condition *dest, Condition* source)
{
	ConditionSlot* slot = findSlot(currentSlots_, SlotWithCondition(dest));
	if(!slot)
		return false;
	if(source){
		while(slot){
			if (slot->condition() == source)
				return false;
			slot = slot->parent();
		}
	}else{
	}
	return true;
}

bool CConditionEditor::canBeDropped(const CPoint& _point)
{
	Vect2f point = view_.coordsFromScreen(Vect2f(_point.x, _point.y));

	trackMouse(point, true);    
	
	return canBeDropped(selectedCondition_, dragedCondition_);
}

void CConditionEditor::onDrop (const CPoint& _point, int typeIndex)
{
	if(typeIndex >= 0){
		onDrop(_point, triggerInterface().createCondition(typeIndex));
	}
}


void CConditionEditor::onDrop(const CPoint& _point, Condition* condition)
{
	if(!condition){
		xassert(condition);
		return;
	}

	Vect2f point = view_.coordsFromScreen (Vect2f (_point.x, _point.y));
	trackMouse (point, true);

	bool can_be_dropped = canBeDropped(selectedCondition_, condition);

	ConditionSlot* slot = findSlot(currentSlots_, SlotWithCondition (condition));

	ShareHandle<Condition> cond = condition;

	if(slot && (can_be_dropped || !selectedCondition_)) {
		if(slot->parent()){
			ConditionSlot* slot = findSlot (currentSlots_, SlotWithCondition (condition));
			ConditionSwitcher* switcher = dynamic_cast<ConditionSwitcher*>(slot->parent()->condition());
			// выкидываем condition...
			removeCondition(switcher, condition);

			// update
			if(!can_be_dropped)
				cleanUp(condition_.condition);
			currentSlots_.create(condition_.condition);
		}
	}

	if (!can_be_dropped)
		return;
    
	ConditionSwitcher* switcher = 0;
	if(selectedCondition_ && (switcher = dynamic_cast<ConditionSwitcher*>(selectedCondition_))) {
		// бросаем на Switcher
		addCondition(switcher, cond);
	}
	else{
		if (selectedCondition_) {
			ConditionSlot* dest_slot = findSlot(currentSlots_, SlotWithCondition(selectedCondition_));
			if(dest_slot){
				if(dest_slot->parent()){
					ConditionSwitcher* switcher = safe_cast<ConditionSwitcher*>(dest_slot->parent()->condition());
					ShareHandle<Condition> dest_condition = dest_slot->condition();

					ShareHandle<ConditionSwitcher> new_switcher = static_cast<ConditionSwitcher*>(&*triggerInterface().createConditionSwitcher());
					addCondition(&*new_switcher, dest_condition);
					addCondition(&*new_switcher, condition);

					replaceCondition(switcher, dest_condition, &*new_switcher);
				}
				else{
					ShareHandle<Condition> dest_condition = dest_slot->condition();
					ShareHandle<Condition> saved = condition_.condition;
					
					ShareHandle<ConditionSwitcher> new_switcher = static_cast<ConditionSwitcher*>(&*triggerInterface().createConditionSwitcher());
					addCondition(&*new_switcher, dest_condition);
					addCondition(&*new_switcher, condition);
					condition_.condition = new_switcher;
				}
			}
			else{
				xassert(0);
			}
		}
		else{
			condition_.condition = condition;
		}
	}
	cleanUp(condition_.condition);
	currentSlots_.create(condition_.condition);
	SetFocus();
}

void CConditionEditor::trackMouse (const Vect2f& point, bool dragOver)
{
	selectedCondition_ = 0;
	forEachSlot(currentSlots_, SelectIfUnderPoint(selectedCondition_, point));
	draging_ = dragOver;
	Invalidate ();
}

void CConditionEditor::OnLButtonDblClk(UINT nFlags, CPoint _point)
{
	Vect2f point = view_.coordsFromScreen(Vect2f(float(_point.x), float(_point.y)));
	trackMouse(point);
	if(!selectedCondition_)
		return;
    
	if(selectedCondition_){
		ConditionSlot* slot = findSlot(currentSlots_, SlotWithCondition(selectedCondition_));
		if(slot->not_rect().point_inside(click_point_)) {
			return;
		}
	}

	if(ConditionSwitcher* switcher = dynamic_cast<ConditionSwitcher*>(selectedCondition_)){
		if (switcher->type == ConditionSwitcher::AND) {
			switcher->type = ConditionSwitcher::OR;
		} else {
			switcher->type = ConditionSwitcher::AND;
		}
		ConditionSlot* slot = findSlot(currentSlots_, SlotWithCondition (switcher));
        xassert (slot);
		slot->setName(getEnumNameAlt(switcher->type));
	}
	else{
		if(selectedCondition_){
			EditArchive ea(GetParent()->GetSafeHwnd(), TreeControlSetup(0, 0, 300, 300, "Scripts\\TreeControlSetups\\SelectedCondition"));
			if(ea.edit(*selectedCondition_)){
				ConditionSlot* slot = findSlot(currentSlots_, SlotWithCondition(selectedCondition_));
				slot->setName(conditionName(selectedCondition_).c_str());
			}
		}
	}

	CWnd::OnLButtonDblClk(nFlags, _point);
}

void CConditionEditor::beginExternalDrag (const CPoint& point, int typeIndex)
{
	SetCursor(::LoadCursor (0, MAKEINTRESOURCE(IDC_NO)));
	SetCapture();
	draging_ = true;
	newConditionIndex_ = typeIndex;
}

BOOL CConditionEditor::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (draging_) {
		return FALSE;
	} else {
		SetCursor(::LoadCursor(0, MAKEINTRESOURCE(IDC_ARROW)));
		return TRUE;
	}
}

void CConditionEditor::setCondition (const EditableCondition& condition)
{
	condition_ = condition;
	currentSlots_.create(condition_.condition);
	currentSlots_.calculateRect(Vect2f::ZERO);
	view_.centerOn (currentSlots_.rect().center());
}

int CConditionEditor::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}
