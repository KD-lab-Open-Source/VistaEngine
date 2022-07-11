#ifndef __ERASE_ACTION_H_INCLUDED__
#define __ERASE_ACTION_H_INCLUDED__
#include "EditorAction.h"
#include "Selection.h"

#include "Handle.h"
class UI_ControlContainer;

/// Операция удаления контрола
class EraseControlAction : public EditorAction
{
public:
  EraseControlAction (const Selection& _selection, UI_ControlContainer& _container);

  virtual void act();
  void undo();
  std::string description() const;
protected:
  /// Удаляемые контролы.
  std::vector<ShareHandle<UI_ControlBase> > removed_controls_;
  Selection selection_;
  UI_ControlContainer& container_;
};

#endif
