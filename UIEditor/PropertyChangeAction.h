#ifndef __UIEDITOR_PROPERTY_CHANGE_EDITOR_ACTION_H_INCLUDED__
#define __UIEDITOR_PROPERTY_CHANGE_EDITOR_ACTION_H_INCLUDED__

#include <string>

#include "EditorAction.h"

typedef std::vector<std::string> TreeNodePath;
class UI_ControlBase;

/// Операция изменения свойства объекта
class PropertyChangeAction : public EditorAction
{
public:
  /**
   *  @param _element - Изменённый элемент (свойство)
   *  @param _new_value - Новое значение элемента в сткроковом представлении
   */
  PropertyChangeAction (UI_ControlBase* _control, const TreeNodePath& _path, const std::string& _new_value);

  virtual ~PropertyChangeAction() {}

  virtual void act();
  virtual void undo();

  virtual std::string description() const {
	  return "Property Change";
  }

  TreeNodePath node_path_;
  UI_ControlBase* control_;
  std::string new_value_;
  std::string old_value_;
};

#endif // __UIEDITOR_PROPERTY_CHANGE_EDITOR_ACTION_H_INCLUDED__
