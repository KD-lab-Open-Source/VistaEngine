#ifndef __CLIP_BOARD_DATA_INSERTER_H_INCLUDED__
#define __CLIP_BOARD_DATA_INSERTER_H_INCLUDED__

#include "TEEngine/UndoManager/TEUndoManager.h"
#include "TriggerExport.h"

#include <map>

class TriggerEditorLogic;
class TEBaseWorkMode;
class TriggerClipBuffer;

class ClipBoardDataInserter  
{
typedef std::map<CString, std::vector<TriggerLink> > LinkIndex;
public:
	ClipBoardDataInserter(TriggerEditorLogic& logic,
							TEBaseWorkMode& mode);
	~ClipBoardDataInserter();
	bool insert(TriggerClipBuffer& clipBuffer, TEUndoManager& undoManager );
protected:
	TEBaseWorkMode& getMode();
	TriggerEditorLogic& getTriggerEditorLogic();
	TriggerEditorLogic const& getTriggerEditorLogic() const;

	//! Находит место для расположения элементов и перемещает туда элементы
	void arrangeTriggersUnderMouse(TriggerList& triggers) const;

	void addTriggers(TriggerChain& chain,
							TriggerList const& triggers,
							TEUndoManager::Bunch& undoBunch,
							TEUndoManager::Bunch& redoBunch);

	void addLink(TEUndoManager::Action& undo, 
							 TEUndoManager::Action& redo,
							 TriggerChain& chain,
							 int parentTriggerIndex, 
							 int childTriggerIndex,
							 int type,
							 CSize const& childOffset,
							 CSize const& parentOffset,
							 bool isAutoRestarted);

	void addLinksFromIndex(TriggerChain& chain,
						   LinkIndex const& index,
						   TEUndoManager::Bunch& undoBunch,
							TEUndoManager::Bunch& redoBunch);

	void renameTriggersForChain(
							TriggerChain const& chain,
							TriggerList& triggers);

	void makeAddLinkIndex(TriggerList& triggers, 
									  LinkIndex& index);

	void renameTrigger(TriggerList& triggers,
								   Trigger& trigger,
								   LPCTSTR newName);
private:
	TriggerChain& getTriggerChain();
private:
	TriggerEditorLogic& logic_;
	TEBaseWorkMode& mode_;
};

#endif
