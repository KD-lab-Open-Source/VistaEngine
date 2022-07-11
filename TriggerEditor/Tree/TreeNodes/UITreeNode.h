#ifndef __U_I_TREE_NODE_H_INCLUDED__
#define __U_I_TREE_NODE_H_INCLUDED__


#include "TREE\TREENODES\ITreeNode.h"
#include <string>

class UITreeNodeFabric;
class xTreeListCtrl;
class CTreeListItem;

class UITreeNode : public IUITreeNode  
{
	friend class UITreeNodeFabric;
protected:
	UITreeNode(std::string const& action, int ordinalNumber);
	virtual ~UITreeNode();
public:

	//! Загрузка узла в дерево
	virtual bool load(xTreeListCtrl& tree, CTreeListItem* pParent);
	//! Обработка команд от пунктов  меню
	virtual bool onCommand(TETreeLogic& logic, WPARAM wParam, LPARAM lParam);
	//! Обработка начал перетаскивания
	virtual void onBeginDrag(TETreeLogic& logic);
	//! Обработчик сообщения об удалении узла
	virtual void onDeleteItem(TETreeLogic& logic);
protected:
	//! Возвращает имя действия
	std::string const& getAction() const;
	//! Назначить имя действия
	void setAction(std::string const& action);
	//! Возвращает узел действия в дереве
	CTreeListItem* getTreeListItem() const;
	//! Назначает узел действия в дереве
	void setTreeListItem(CTreeListItem* pItem);
	//! Возвращает порядковый номер действия
	int getOrdinalNumber() const;
	//! Назначает порядковый номер
	void setOrdinalNumber(int ordNum);
private:
	//! Имя действия
	std::string action_;

	//! Порядковый номер действия
	int ordinalNumber_;
	//! Указатель на узел действия в дереве
	CTreeListItem* treeListItem_;
};

#endif
