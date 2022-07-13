#ifndef __EXPORT_TREE_TO_EXCEL__
#define __EXPORT_TREE_TO_EXCEL__

class ExcelExporter;
class TreeNode;

class ExportTreeToExcel
{
public:
	ExportTreeToExcel(const char* filename);
	~ExportTreeToExcel();
	
	bool exportTreeFromNode(const TreeNode* rootNode);
	void setLabel(const char* label) { label_ = label; }

private:
	bool exportTreeNode(const TreeNode* node, int level, int* line);
	void outTreeNode(const TreeNode* node, int level, int line);

	ExcelExporter* application_;
	string label_;
};

#endif // __EXPORT_TREE_TO_EXCEL__
