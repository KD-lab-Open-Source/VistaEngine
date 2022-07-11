#include "stdafx.h"
#include "TriggerView.h"
#include "TriggerMiniMap.h"
#include "kdw\Win32\Window.h"
#include "kdw\PopupMenu.h"
#include "kdw\PropertyTree.h"
#include "kdw\DragManager.h"
#include "kdw\Dialog.h"
#include "kdw\Entry.h"
#include <functional>
#include "Serialization\SerializationFactory.h"
#include "Serialization\Decorators.h"
#include "EditableCondition.h"
#include "kdw/PropertyEditor.h"
#include "shlwapi.h"

// Для StrStrI
#pragma message("Automatically linking with shlwapi.lib") 
#pragma comment(lib, "shlwapi.lib") 

using namespace Win32;

TriggerChain TriggerView::copiedTriggers_;

TriggerView::TriggerView(TriggerChain& triggerChain, TriggerMiniMap* miniMap, kdw::PropertyTree* propertyTree)
: kdw::Viewport2D(0, 16)
, triggerChain_(triggerChain)
, miniMap_(miniMap)
, propertyTree_(propertyTree)
{
	debug_ = false;
	areaSelection_ = false;
	moving_ = false;
	movingParent_ = false;
	deselectWhenKeyUp_ = false;
	selectedLink_ = 0;
	movingDelta_ = Vect2i::ZERO;
	creatingLink_ = false;
	wasMouseMove_ = false;
	popupMenu_ = false;
	find_ = false;
	findIndex_ = 0;
	legend_ = false;

	triggerColor_ = Color4c(128, 255, 128);
	linkColor_ = 0;
	linkAutoRestarted_ = false;

	triggerColorPrev_ = Color4c::WHITE;
	linkColorPrev_ = 0;
	linkAutoRestartedPrev_ = false;

	undoIndex_ = 0;

	tooltip_.attach(this);
	miniMap_->setTriggerView(this);
	propertyTree_->signalChanged().connect(this, &TriggerView::onPropertyChanged);

	updatePropertyTree();
	saveStep();
}

float quantize(float x, float delta)
{
	return round(x/delta)*delta;
}

void TriggerView::onRedraw(HDC dc)
{
	xassert(miniMap_);
	miniMap_->redraw();

	__super::onRedraw(dc);

	Rectf rt = visibleArea();
	fillRectangle(dc, rt, Color4c::WHITE);

	Color4c gridColor(192, 192, 192);
	for(float y = quantize(rt.top(), Trigger::SIZE_Y); y <= rt.bottom(); y += Trigger::SIZE_Y) 
		drawLine(dc, Vect2f(rt.left(), y), Vect2f(rt.right(), y), gridColor, PS_DOT);

	for(float x = quantize(rt.left(), Trigger::SIZE_X + Trigger::OFFSET_X); x <= rt.right(); x += Trigger::SIZE_X + Trigger::OFFSET_X){
		drawLine(dc, Vect2f(x, rt.top()), Vect2f(x, rt.bottom()), gridColor, PS_DOT);
		drawLine(dc, Vect2f(x + Trigger::SIZE_X, rt.top()), Vect2f(x + Trigger::SIZE_X, rt.bottom()), gridColor, PS_DOT);
	}

	Win32::StockSelector old_font(dc, positionFont_);

	TriggerList::iterator ti;
	FOR_EACH(triggerChain_.triggers, ti)
		drawLinks(dc, *ti);

	FOR_EACH(triggerChain_.triggers, ti)
		drawTrigger(dc, *ti, false);

	if(movingDelta_ != Vect2i::ZERO){
		Indices::iterator i;
		FOR_EACH(selectedTriggers_, i)
			drawTrigger(dc, triggerChain_.triggers[*i], true);
	}

	if(areaSelection_)
		drawRectangle(dc, areaRectangle_, Color4c(128, 128, 128));

	if(selectedLink_){
		Vect2f size(6, 6);
		fillRectangle(dc, Rectf(selectedLink_->parentPoint() - size, size*2), Color4c::BLACK);
		fillRectangle(dc, Rectf(selectedLink_->childPoint() - size, size*2), Color4c::BLACK);
	}

	if(creatingLink_)
		drawLine(dc, clickPoint_, coordsFromScreen(mousePosition()), TriggerLink::colors[linkColor_], PS_SOLID, linkAutoRestarted_ ? 4 : 1);

	if(creatingLink_){
		Vect2f delta(Vect2f::ZERO);
		if(mousePosition().x < 0)
			delta.x = 1;
		else if(mousePosition().x > viewSize_.x)
			delta.x = -1;
		if(mousePosition().y < 0)
			delta.y = 1;
		else if(mousePosition().y > viewSize_.y)
			delta.y = -1;
		viewOffset_ += delta*(10.f/viewScale());
		redraw();
	}
}	

void TriggerView::drawLinks(HDC dc, Trigger& trigger)
{
	OutcomingLinksList::const_iterator i;
	FOR_EACH(trigger.outcomingLinks(), i){
		Color4c color = debug_ ? TriggerLink::debugColors[i->active() & 1] : TriggerLink::colors[i->colorType()];
		Vect2f p0 = i->parentPoint();
		Vect2f p1 = i->childPoint();
		Vect2f dir = p1 - p0;
		dir.normalize(4);
		Vect2f normal(dir.y, -dir.x);
		dir *= 4;
		Vect2i points[3];
		points[0] = p1;
		points[1] = p1 - dir + normal;
		points[2] = p1 - dir - normal;
		p1 -= dir*0.5f;

		if(i->autoRestarted())
			drawLine(dc, p0, p1, Color4c::BLACK, PS_SOLID, 5);
		drawLine(dc, p0, p1, color, PS_SOLID, 2);

		for(int j = 0; j < 3; j++)
			points[j] = coordsToScreen(points[j]);
		
		Win32::AutoSelector Brush(dc, CreateSolidBrush(color.RGBGDI()));
		Polygon(dc, (POINT*)points, 3);
	}
}

void TriggerView::drawTrigger(HDC dc, Trigger& trigger, bool moved) 
{
	Vect2f leftTop = trigger.leftTop();
	if(moved)
		leftTop += movingDelta_*Trigger::gridSizeSpaced();
	Vect2f size = Trigger::gridSize();
	Vect2f border(4, 4);
	fillRectangle(dc, Rectf(leftTop, size), trigger.selected() ? (moved ? Color4c::MAGENTA : Color4c::RED) : Color4c::WHITE);
	fillRectangle(dc, Rectf(leftTop + border, size - border*2), debug_ ? Trigger::debugColors[trigger.state()] : trigger.color());
	drawRectangle(dc, Rectf(leftTop + border, size - border*2), Color4c::BLACK);
	drawRectangle(dc, Rectf(leftTop, size), Color4c::BLACK);
	Vect2f textBorder(8, 4);
	drawText(dc, Rectf(leftTop + textBorder, size - textBorder*2), trigger.displayText(), ALIGN_CENTER, true);
}

void TriggerView::onKeyDown(const sKey& key)
{
	switch(key){
		case 'C' | sKey::CONTROL:
			copyTriggers();
			break;
		case 'V' | sKey::CONTROL:
			pasteTriggers();
			break;
		case VK_DELETE:
			deleteLink();
			deleteTriggers();
			break;
		case 'F' | sKey::CONTROL:
			find();
			return;
	}
	saveStep();
}

void TriggerView::onMouseButtonDown(kdw::MouseButton button)
{
	__super::onMouseButtonDown(button);

	switch(button){
	case kdw::MOUSE_BUTTON_LEFT: {
		captureMouse();
		clickPoint_ = coordsFromScreen(mousePosition());
		if(selectedLink_ && selectedLink_->parentPoint().distance2(clickPoint_) < sqr(8)){
			moving_ = movingParent_ = true;
			break;
		}
		selectedLink_ = 0;
		int index = findTrigger(clickPoint_);
		if(index != -1){
			if(selectedTriggers_.exists(index)){
				if(isKeyPressed(VK_SHIFT) && selectedTriggers_.size() > 1){
					triggerChain_.triggers[index].setSelected(false);
					selectedTriggers_.remove(index);
				}
				else 
					deselectWhenKeyUp_ = true;
			}
			else{
				if(!isKeyPressed(VK_SHIFT) && !isKeyPressed(VK_CONTROL))
					deselect();
				triggerChain_.triggers[index].setSelected(true);
				selectedTriggers_.add(index);
			}
		}
		else{
			if(!isKeyPressed(VK_SHIFT) && !isKeyPressed(VK_CONTROL))
				deselect();
			if(selectedLink_ = findLink(clickPoint_))
				moving_ = true;
			else
				areaSelection_ = true;
		}
		if(!selectedTriggers_.empty())
			moving_ = true;
		updatePropertyTree();
		redraw();
		} break;

	case kdw::MOUSE_BUTTON_RIGHT: 
		captureMouse();
		wasMouseMove_ = false;
		clickPoint_ = coordsFromScreen(mousePosition());
		creatingLink_ = findTrigger(clickPoint_) != -1;
		break;

	case kdw::MOUSE_BUTTON_MIDDLE:
		clickPoint_ = coordsFromScreen(mousePosition());
		treeSelect();
		break;

	case kdw::MOUSE_BUTTON_LEFT_DOUBLE: 
		if(isKeyPressed(VK_SHIFT))
			editTrigger();
		else if(isKeyPressed(VK_MENU))
			editAction();
		else
			editConditions();
		saveStep();
		break;
	}
}

void TriggerView::onMouseButtonUp(kdw::MouseButton button)
{
	__super::onMouseButtonUp(button);

	switch(button){
	case kdw::MOUSE_BUTTON_LEFT: 
		releaseMouse();
		if(kdw::dragManager.dragging()){ 
			int index = kdw::dragManager.data();
			if(index >= 0)
				createTrigger(index);
			kdw::dragManager.drop();
		}
		if(deselectWhenKeyUp_){
			deselectWhenKeyUp_ = false;
			deselect();
			int index = findTrigger(clickPoint_);
			xassert(index != -1);
			triggerChain_.triggers[index].setSelected(true);
			selectedTriggers_.add(index);
			updatePropertyTree();
		}
		if(areaSelection_){
			selectArea(areaRectangle_);
			areaSelection_ = false;
			areaRectangle_ = Rectf(Vect2f::ZERO, Vect2f::ZERO);
			updatePropertyTree();
		}
		else if(moving_)
			moveTriggers();
	
		movingParent_ = moving_ = false;
		movingDelta_ = Vect2i::ZERO;
		redraw();
		saveStep();
		break;

	case kdw::MOUSE_BUTTON_RIGHT: 
		releaseMouse();
		if(wasMouseMove_ && creatingLink_){
			createLink();
			redraw();
		}
		else if(!wasMouseMove_)
			popupMenu();
		wasMouseMove_ = creatingLink_ = false;
		saveStep();
	}
}

void TriggerView::onMouseMove(const Vect2i& delta)
{
	if(popupMenu_){
		popupMenu_ = false;
		return;
	}

	deselectWhenKeyUp_ = false;
	wasMouseMove_ = true;

	Vect2f point = coordsFromScreen(mousePosition());

	tooltip_.show();

	if(creatingLink_){
		tooltip_.setText("");
		redraw();
		return;
	}

	if(areaSelection_){
		areaRectangle_ = Rectf(Vect2f(min(clickPoint_.x, point.x), min(clickPoint_.y, point.y)), Vect2f(fabsf(clickPoint_.x - point.x), fabsf(clickPoint_.y - point.y)));
		redraw();
	}
	else if(moving_){
		if(selectedLink_){
			if(movingParent_)
				selectedLink_->setParentPoint(point);
			else
				selectedLink_->setChildPoint(point);
		}
		else{
			movingDelta_ = Vect2i((point - clickPoint_)/Trigger::gridSizeSpaced());
			if(!checkToMove())
				movingDelta_ = Vect2i::ZERO;
		}

		redraw();
	}
	else{
		int index = findTrigger(point);
		if(index != -1)
			tooltip_.setText(triggerChain_.triggers[index].debugDisplayText(debug_));
		else
			tooltip_.setText("");
	}

	__super::onMouseMove(delta);
}

TriggerLink* TriggerView::findLink(const Vect2f& point)
{
	Vect2f border(Trigger::OFFSET_X/2, Trigger::SIZE_Y/2);
	TriggerList::iterator ti;
	FOR_EACH(triggerChain_.triggers, ti)
		if(Rectf(ti->leftTop() - border, Trigger::gridSize() + border*2).point_inside(point)){
			TriggerLink* link = 0;
			float distMin = 5;
			IncomingLinksList::iterator li;
			FOR_EACH(ti->incomingLinks(), li){
				Vect2f p0 = (*li)->childPoint();
				MatX2f X(Mat2f(((*li)->parentPoint() - p0).normalize(1.f)), p0);
				Vect2f p = X.invXform(point);
				if(p.x < 20 && p.x > 0 && distMin > fabsf(p.y)){
					distMin = fabsf(p.y);
					link = *li;
				}
			}
			return link;
		}
	return 0;
}

int TriggerView::findTrigger(const Vect2f& point)
{
	TriggerList::iterator ti;
	FOR_EACH(triggerChain_.triggers, ti)
		if(Rectf(ti->leftTop(), Trigger::gridSize()).point_inside(point))
			return ti - triggerChain_.triggers.begin();
	return -1;
}

void TriggerView::deselect()
{
	Indices::iterator i;
	FOR_EACH(selectedTriggers_, i)
		triggerChain_.triggers[*i].setSelected(false);
	selectedTriggers_.clear();
	selectedLink_ = 0;
}

void TriggerView::selectArea(const Rectf& rect)
{
	if(rect.size().norm2() < FLT_EPS)
		return;
	if(!isKeyPressed(VK_SHIFT) && !isKeyPressed(VK_CONTROL))
		deselect();
	TriggerList::iterator ti;
	FOR_EACH(triggerChain_.triggers, ti)
		if(Rectf(ti->leftTop(), Trigger::gridSize()).rect_overlap(rect)){
			ti->setSelected(true);
			selectedTriggers_.add(ti - triggerChain_.triggers.begin());
		}
}

bool TriggerView::checkToMove()
{
	Indices::iterator i;
	FOR_EACH(selectedTriggers_, i){
		Vect2i pos = Vect2i(triggerChain_.triggers[*i].cellIndex()) + movingDelta_;
		TriggerList::iterator ti;
		FOR_EACH(triggerChain_.triggers, ti)
			if(!ti->selected() && Vect2i(ti->cellIndex()) == pos)
				return false;
	}
	return true;
}

void TriggerView::moveTriggers()
{
	Indices::iterator i;
	FOR_EACH(selectedTriggers_, i){
		Trigger& trigger = triggerChain_.triggers[*i];
		trigger.setCellIndex(trigger.cellIndex() + movingDelta_);
	}
	TriggerList::iterator ti;
	FOR_EACH(triggerChain_.triggers, ti){
		OutcomingLinksList::iterator li;
		FOR_EACH(ti->outcomingLinks(), li){
			li->fixParentOffset();
			li->fixChildOffset();
		}
	}
}

void TriggerView::copyTriggers()
{
	Vect2i pos(INT_INF, INT_INF);
	copiedTriggers_.triggers.clear();
	Indices::iterator i;
	FOR_EACH(selectedTriggers_, i){
		Trigger& trigger = triggerChain_.triggers[*i];
		pos.x = min(pos.x, trigger.cellIndex().x);
		pos.y = min(pos.y, trigger.cellIndex().y);
	}

	FOR_EACH(selectedTriggers_, i){
		BinaryOArchive oa;
		triggerChain_.triggers[*i].serialize(oa);
		Trigger triggerSerialized;
		triggerSerialized.serialize(BinaryIArchive(oa));
		copiedTriggers_.triggers.push_back(triggerSerialized);
		Trigger& trigger = copiedTriggers_.triggers.back();
		trigger.setCellIndex(trigger.cellIndex() - pos);
		trigger.setSelected(false);
	}
	copiedTriggers_.buildLinks();
}

void TriggerView::pasteTriggers()
{
	if(copiedTriggers_.triggers.empty())
		return;

	Vect2i leftTop = coordsFromScreen(mousePosition())/Trigger::gridSizeSpaced();

	TriggerList::iterator ti;
	FOR_EACH(copiedTriggers_.triggers, ti){
		Vect2i pos = Vect2i(ti->cellIndex()) + leftTop;
		TriggerList::iterator tj;
		FOR_EACH(triggerChain_.triggers, tj)
			if(Vect2i(tj->cellIndex()) == pos)
				return;

		string name = triggerChain_.uniqueName(ti->name());
		if(name != ti->name())
			copiedTriggers_.renameTrigger(ti->name(), name.c_str());
	}

	FOR_EACH(copiedTriggers_.triggers, ti){
		triggerChain_.triggers.push_back(*ti);
		Trigger& trigger = triggerChain_.triggers.back();
		trigger.setCellIndex(trigger.cellIndex() + leftTop);
	}
	
	triggerChain_.buildLinks();
	redraw();
}

void TriggerView::deleteTriggers()
{
	if(selectedTriggers_.empty())
		return;

	sort(selectedTriggers_.begin(), selectedTriggers_.end(), greater<int>());
	Indices::iterator i;
	FOR_EACH(selectedTriggers_, i)
		triggerChain_.removeTrigger(*i);
	
	selectedTriggers_.clear();
	redraw();
}

void TriggerView::deleteLink()
{
	if(!selectedLink_)
		return;

	TriggerList::iterator ti;
	FOR_EACH(triggerChain_.triggers, ti){
		OutcomingLinksList::iterator li;
		FOR_EACH(ti->outcomingLinks(), li)
			if(&*li == selectedLink_){
				ti->outcomingLinks().erase(li);
				selectedLink_ = 0;
				triggerChain_.buildLinks();
				redraw();
				return;
			}
	}

	selectedLink_ = 0;
	redraw();
}


void TriggerView::createLink()
{
	int parentIndex = findTrigger(clickPoint_);
	Trigger& parent = triggerChain_.triggers[parentIndex];
	Vect2i childPoint = coordsFromScreen(mousePosition());
	int childIndex = findTrigger(childPoint);
	if(childIndex == -1 || parentIndex == childIndex)
		return;
	Trigger& child = triggerChain_.triggers[childIndex];
	OutcomingLinksList::iterator li;
	FOR_EACH(parent.outcomingLinks(), li)
		if(li->child == &child)
			return;

	parent.outcomingLinks().push_back(TriggerLink());
	TriggerLink& link = parent.outcomingLinks().back();
	link.setTriggerName(child.name());
	triggerChain_.buildLinks();
	link.setParentPoint(clickPoint_);
	link.setChildPoint(childPoint);
	link.setParentPoint(clickPoint_);
	link.setColorType(linkColor_);
	link.setAutoRestarted(linkAutoRestarted_);
	
	deselect();
	selectedLink_ = &link;
	
	updatePropertyTree();
}

void TriggerView::popupMenu()
{
	kdw::PopupMenu menu(300);

	clickPoint_ = coordsFromScreen(mousePosition());
	int index = findTrigger(clickPoint_);
	if(index != -1){
		if(selectedTriggers_.exists(index)){
			if(isKeyPressed(VK_SHIFT) && selectedTriggers_.size() > 1){
				triggerChain_.triggers[index].setSelected(false);
				selectedTriggers_.remove(index);
			}
		}
		else{
			if(!isKeyPressed(VK_SHIFT) && !isKeyPressed(VK_CONTROL))
				deselect();
			triggerChain_.triggers[index].setSelected(true);
			selectedTriggers_.add(index);
		}

		menu.root().add(TRANSLATE("Удалить триггер"))
			.connect(this, &TriggerView::deleteTriggers)
			.setHotkey(sKey(VK_DELETE));
		menu.root().add(TRANSLATE("Копировать триггер"))
			.connect(this, &TriggerView::copyTriggers)
			.setHotkey(sKey('C' | sKey::CONTROL));
		menu.root().add(TRANSLATE("Селект дерева (ср.кнопка)"))
			.connect(this, &TriggerView::treeSelect);
		menu.root().addSeparator();
		menu.root().add(TRANSLATE("Условия (дв.клик)"))
			.connect(this, &TriggerView::editConditions);
		menu.root().add(TRANSLATE("Действие (Alt + дв.клик)"))
			.connect(this, &TriggerView::editAction);
		menu.root().add(TRANSLATE("Свойства (Shift + дв.клик)"))
			.connect(this, &TriggerView::editTrigger);
	}
	else if(selectedLink_ = findLink(clickPoint_)){
		menu.root().add(TRANSLATE("Удалить стрелку"))
		.connect(this, &TriggerView::deleteLink)
		.setHotkey(sKey(VK_DELETE));
	}
	else{
		menu.root().add(TRANSLATE("Вставить триггер"))
			.connect(this, &TriggerView::pasteTriggers)
			.setHotkey(sKey('V' | sKey::CONTROL));
		menu.root().addSeparator();
		menu.root().add(TRANSLATE("Добавить строку"))
			.connect(this, &TriggerView::insertRow);
		menu.root().add(TRANSLATE("Добавить столбец"))
			.connect(this, &TriggerView::insertColumn);
		menu.root().addSeparator();
		menu.root().add(TRANSLATE("Удалить строку"))
			.connect(this, &TriggerView::deleteRow);
		menu.root().add(TRANSLATE("Удалить столбец"))
			.connect(this, &TriggerView::deleteColumn);
	}

	menu.spawn(this);
	popupMenu_ = true;
}

void TriggerView::treeSelect()
{
	deselect();

	int index = findTrigger(clickPoint_);
	if(index != -1){
		treeSelectRecursive(triggerChain_.triggers[index]);
		redraw();
	}
}

void TriggerView::treeSelectRecursive(Trigger& trigger)
{
	trigger.setSelected(true);
	selectedTriggers_.add(triggerChain_.triggerIndex(trigger));

	OutcomingLinksList::iterator li;
	FOR_EACH(trigger.outcomingLinks(), li)
		if(!li->child->selected())
			treeSelectRecursive(*li->child);
}

/////////////////////////////////////////////////////

struct TriggerChainPropertySerializer
{
	TriggerChainPropertySerializer(TriggerChain& triggerChain)  
	{
		triggerChain_ = &triggerChain;
	}
	void serialize(Archive& ar)
	{
		triggerChain_->serializeProperties(ar);
	}
	static TriggerChain* triggerChain_;
};

TriggerChain* TriggerChainPropertySerializer::triggerChain_;

/////////////////////////////////////////////////////

struct Legend {
	Color4c sleepingTrigger;
	Color4c checkingTrigger;
	Color4c workingTrigger;
	Color4c doneTrigger;
	Color4c passiveLink;
	Color4c activeLink;

	void serialize(Archive& ar)
	{
		sleepingTrigger = Trigger::debugColors[0];
		checkingTrigger = Trigger::debugColors[1];
		workingTrigger = Trigger::debugColors[2];
		doneTrigger = Trigger::debugColors[3];
		passiveLink = TriggerLink::debugColors[0];
		activeLink = TriggerLink::debugColors[1];
		
		ar.serialize(sleepingTrigger, "sleepingTrigger", "Неактивный триггер");
		ar.serialize(checkingTrigger, "checkingTrigger", "Ждущий триггер");
		ar.serialize(workingTrigger, "workingTrigger", "Работающий триггер");
		ar.serialize(doneTrigger, "doneTrigger", "Отработанный триггер");

		ar.serialize(HLineDecorator(), "line", "<");

		ar.serialize(passiveLink, "passiveLink", "Неактивная связь");
		ar.serialize(activeLink, "activeLink", "Активная связь");
	}
};

/////////////////////////////////////////////////////

struct FindData
{
	string name;
	ComboListString condition;
	ComboListString action;
	ButtonDecorator button;

	FindData() : button("Найти следующий") {}

	void serialize(Archive& ar){
		condition.setComboList((string("|") + FactorySelector<Condition>::Factory::instance().comboListAlt()).c_str());
		action.setComboList((string("|") + FactorySelector<Action>::Factory::instance().comboListAlt()).c_str());
		ar.serialize(name, "name", "Имя");
		ar.serialize(condition, "condition", "Условие");
		ar.serialize(action, "action", "Действие");
		ar.serialize(button, "button", "<");
		if(ar.isInput()){
			int pos = condition.value().find_last_of("\\");
			if(pos != string::npos)
				condition = string(condition.value().begin() + pos + 1, condition.value().end());
			pos = action.value().find_last_of("\\");
			if(pos != string::npos)
				action = string(action.value().begin() + pos + 1, action.value().end());

		}
	}
};

FindData findData;

void TriggerView::find()
{
	find_ = true;
	updatePropertyTree();
	find_ = true;
	propertyTree_->setFocus();
}

void TriggerView::updatePropertyTree()
{
	propertyTree_->detach();
	if(find_){
		propertyTree_->attach(Serializer(findData));
		find_ = false;
	}
	else if(legend_){
		Legend legend;
		propertyTree_->attach(Serializer(legend));
		legend_ = false;
	}
	else if(selectedLink_){
		linkColorPrev_ = selectedLink_->colorType();
		linkAutoRestartedPrev_ = selectedLink_->autoRestarted();
		propertyTree_->attach(Serializer(*selectedLink_));
	}
	else if(selectedTriggers_.size() == 1){
		Trigger& trigger = triggerChain_.triggers[selectedTriggers_.front()];
		triggerColorPrev_ = trigger.color();
		triggerName_ = trigger.name();
		propertyTree_->attach(Serializer(trigger));
	}
	else
		propertyTree_->attach(Serializer(TriggerChainPropertySerializer(triggerChain_)));
}

void TriggerView::onPropertyChanged() 
{ 
	if(find_){
		deselect();
		TriggerList::iterator ti;
		FOR_EACH(triggerChain_.triggers, ti){
			if(!findData.name.empty() && !StrStrI(ti->name(), findData.name.c_str())) 
				continue;
			if(!findData.condition.value().empty()){
				 if(!ti->condition)
					 continue;
				 XBuffer buffer;
				 ti->condition->writeInfo(buffer, "", false);
				 if(!strstr(buffer, findData.condition))
					 continue;
			}
			if(!findData.action.value().empty()){
				if(!ti->action)
					continue;
				if(!strstr(ti->action->name(), findData.action))
					continue;
			}
			selectedTriggers_.push_back(ti - triggerChain_.triggers.begin());
			ti->setSelected(true);
		}

		if(findData.button && !selectedTriggers_.empty())
			centerOn(triggerChain_.triggers[selectedTriggers_[findIndex_++ % selectedTriggers_.size()]].leftTop() + Trigger::gridSize()*0.5f);
	}
	if(selectedLink_){
		if(linkColorPrev_ != selectedLink_->colorType())
			linkColor_ = selectedLink_->colorType();
		if(linkAutoRestartedPrev_ != selectedLink_->autoRestarted())
			linkAutoRestarted_ = selectedLink_->autoRestarted();
	}
	else if(selectedTriggers_.size() == 1){
		Trigger& trigger = triggerChain_.triggers[selectedTriggers_.front()];
		if(triggerColorPrev_ != trigger.color())
			triggerColor_ = trigger.color();
		if(triggerName_ != trigger.name()){
			string newName = trigger.name();
			trigger.setName(triggerName_.c_str());
			triggerChain_.renameTrigger(triggerName_.c_str(), newName.c_str());
		}
	}
	redraw(); 
	saveStep();
	propertyTree_->setFocus();
}

void TriggerView::createTrigger(int index)
{
	static string name = "Триггер";
	kdw::Dialog dialog(this, 0);
	dialog.setTitle("Имя триггера");
	dialog.setDefaultPosition(kdw::POSITION_CURSOR);
	kdw::Entry* entry = new kdw::Entry(name.c_str());
	dialog.add(entry);
	dialog.addButton("OK", kdw::RESPONSE_OK);
	dialog.addButton("Cancel", kdw::RESPONSE_CANCEL);
	if(dialog.showModal() == kdw::RESPONSE_CANCEL)
		return;
	name = triggerChain_.uniqueName(entry->text());

	deselect();

	{
		Trigger trigger;
		trigger.setName(name.c_str());
		trigger.action = FactorySelector<Action>::Factory::instance().findByIndex(index).create();
		trigger.setCellIndex((clickPoint() - Trigger::gridSize()*0.5f)/Trigger::gridSizeSpaced());
		trigger.setColor(triggerColor_);
		trigger.setSelected(true);
		triggerChain_.triggers.push_back(trigger);
		triggerChain_.buildLinks();
		selectedTriggers_.push_back(triggerChain_.triggers.size() - 1);
	}

	updatePropertyTree();
}

void TriggerView::editConditions()
{
	deselect();
	clickPoint_ = coordsFromScreen(mousePosition());
	int index = findTrigger(clickPoint_);
	if(index != -1){
		selectedTriggers_.push_back(index);
		Trigger& trigger = triggerChain_.triggers[index];
		trigger.setSelected(true);

		EditableCondition editableCondition(trigger.condition);
		trigger.condition = 0;
		bool result = kdw::edit(Serializer(editableCondition), "Scripts\\TreeControlSetups\\editConditionSetupState", kdw::ONLY_TRANSLATED, this, "Условия");
		trigger.condition = editableCondition.condition;
		editableCondition.condition = 0;
		if(result)
			updatePropertyTree();
	}
}

void TriggerView::editAction()
{
	deselect();
	clickPoint_ = coordsFromScreen(mousePosition());
	int index = findTrigger(clickPoint_);
	if(index != -1){
		selectedTriggers_.push_back(index);
		Trigger& trigger = triggerChain_.triggers[index];
		trigger.setSelected(true);

		ActionPtr action = trigger.action;
		trigger.action = 0;
		bool result = kdw::edit(Serializer(action), "Scripts\\TreeControlSetups\\editActionSetupState", kdw::ONLY_TRANSLATED, this, "Действие");
		trigger.action = action;
		action = 0;
		if(result)
			updatePropertyTree();
	}
}

void TriggerView::editTrigger()
{
	deselect();
	clickPoint_ = coordsFromScreen(mousePosition());
	int index = findTrigger(clickPoint_);
	if(index != -1){
		selectedTriggers_.push_back(index);
		Trigger& trigger = triggerChain_.triggers[index];
		trigger.setSelected(true);

		triggerName_ = trigger.name();
		if(kdw::edit(Serializer(trigger), "Scripts\\TreeControlSetups\\editTriggerSetupState", kdw::ONLY_TRANSLATED, this, "Свойства триггера")){
			if(triggerName_ != trigger.name()){
				string newName = trigger.name();
				trigger.setName(triggerName_.c_str());
				triggerChain_.renameTrigger(triggerName_.c_str(), newName.c_str());
			}
			updatePropertyTree();
		}
	}
}

void TriggerView::insertRow()
{
	Vect2i cell = clickPoint_/Trigger::gridSizeSpaced();
	TriggerList::iterator ti;
	FOR_EACH(triggerChain_.triggers, ti){
		Vect2i point = ti->cellIndex();
		if(point.y >= cell.y)
			ti->setCellIndex(point + Vect2i(0, 1));
	}
	redraw();
}

void TriggerView::insertColumn()
{
	Vect2i cell = clickPoint_/Trigger::gridSizeSpaced();
	TriggerList::iterator ti;
	FOR_EACH(triggerChain_.triggers, ti){
		Vect2i point = ti->cellIndex();
		if(point.x >= cell.x)
			ti->setCellIndex(point + Vect2i(1, 0));
	}
	redraw();

}

void TriggerView::deleteRow()
{
	Vect2i cell = clickPoint_/Trigger::gridSizeSpaced();
	TriggerList::iterator ti;
	FOR_EACH(triggerChain_.triggers, ti)
		if(ti->cellIndex().y == cell.y)
			return;
	FOR_EACH(triggerChain_.triggers, ti){
		Vect2i point = ti->cellIndex();
		if(point.y > cell.y)
			ti->setCellIndex(point + Vect2i(0, -1));
	}
	redraw();
}

void TriggerView::deleteColumn()
{
	Vect2i cell = clickPoint_/Trigger::gridSizeSpaced();
	TriggerList::iterator ti;
	FOR_EACH(triggerChain_.triggers, ti)
		if(ti->cellIndex().x == cell.x)
			return;
	FOR_EACH(triggerChain_.triggers, ti){
		Vect2i point = ti->cellIndex();
		if(point.x > cell.x)
			ti->setCellIndex(point + Vect2i(-1, 0));
	}
	redraw();
}

void TriggerView::saveStep()
{
	ShareHandle<BinaryOArchive> oa = new BinaryOArchive();
	oa->serialize(triggerChain_, "triggerChain", 0);
	if(!history_.empty() && *history_[undoIndex_] == *oa)
		return;
	
	if(!history_.empty() && undoIndex_ != history_.size() - 1)
		history_.erase(history_.begin() + undoIndex_ + 1, history_.end());
	
	if(history_.size() > HISTORY_STEPS)
		history_.erase(history_.begin());
	
	history_.push_back(oa);
	undoIndex_ = history_.size() - 1;
	updatePropertyTree();
}

void TriggerView::undo()
{
	if(!undoIndex_)
		return;
	BinaryIArchive ia(*history_[--undoIndex_]);
	ia.serialize(triggerChain_, "triggerChain", 0);
	redraw();
}

void TriggerView::redo()
{
	if(undoIndex_ == history_.size() - 1)
		return;
	BinaryIArchive ia(*history_[++undoIndex_]);
	ia.serialize(triggerChain_, "triggerChain", 0);
	redraw();
}

void TriggerView::showLegend()
{
	legend_ = true;
	updatePropertyTree();
}
