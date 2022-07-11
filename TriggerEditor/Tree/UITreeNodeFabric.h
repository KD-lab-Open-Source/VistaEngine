#ifndef __UI_TREE_NODE_FABRIC_H_INCLUDED__
#define __UI_TREE_NODE_FABRIC_H_INCLUDED__
// UITreeNodeFabric.h: interface for the UITreeNodeFabric class.
//
//////////////////////////////////////////////////////////////////////

#include <memory>

interface IUITreeNode;
class UITreeNodeFabric  
{
	UITreeNodeFabric();
	~UITreeNodeFabric();
public:
	static std::auto_ptr<IUITreeNode> create(std::string const& action, 
											int ordinalNumber);
};

#endif
