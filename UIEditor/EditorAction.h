#ifndef __EDITOR_ACTION_H_INCLUDED__
#define __EDITOR_ACTION_H_INCLUDED__

#include "Handle.h"

class ControlsTree;

/// Абстрактная база для всех возможных операций редактирования.
class EditorAction : public ShareHandleBase
{
public:
    EditorAction(){};
	virtual ~EditorAction(){};
    /// Метод соврешающий операцию.
    virtual void act() = 0;
    /// Метод соврешающий откат операции.
    virtual void undo() = 0;
    /// Краткое описание действия (использувается в меню Edit->Undo).
	virtual std::string description() const = 0;
protected:
	ControlsTree* controlsTree();
};

#endif
