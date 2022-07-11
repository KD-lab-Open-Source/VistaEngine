#ifndef __VISTA_MODEL_VIEWER_FILE_TREE_H_INCLUDED__
#define __VISTA_MODEL_VIEWER_FILE_TREE_H_INCLUDED__

#include <string>
#include "kdw/Tree.h"
#include "kdw/TreeView.h"
#include "kdw/TreeModel.h"

class FileTreeModel: public kdw::StringTreeModel{
public:
	FileTreeModel();

	void goUp();
	const char* currentDirectory() const{ return currentDirectory_.c_str(); }
	void setCurrentDirectory(const char* path);
	void rebuild();

	void serialize(Archive& ar);

	sigslot::signal0<>& signalLocationChanged() { return signalLocationChanged_; }
	sigslot::signal1<const char*>& signalFileSelected() { return signalFileSelected_; }
protected:
	void fillNode(kdw::StringTreeRow* node, const char* path);
	bool activateRow(kdw::TreeRow* node, int column);
	void expandRow(kdw::TreeRow* node);

	std::string currentDirectory_;

	sigslot::signal0<> signalLocationChanged_;
	sigslot::signal1<const char*> signalFileSelected_;
};

#endif
