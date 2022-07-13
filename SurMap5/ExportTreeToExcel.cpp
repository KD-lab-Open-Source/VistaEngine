#include "stdafx.h"
#include "ExportTreeToExcel.h"
#include "..\ExcelExport\ExcelExporter.h"
#include "..\Util\Serialization\TreeInterface.h"

ExportTreeToExcel::ExportTreeToExcel(const char* filename)
{
	application_ = ExcelExporter::create(filename);
	application_->beginSheet();
	label_ = "";
}

ExportTreeToExcel::~ExportTreeToExcel()
{
	application_->free();
}

void ExportTreeToExcel::outTreeNode(const TreeNode* node, int level, int line)
{
	if(!application_ || !node)
		return;

	if(label_ != "" && level != 0 && !node->empty()) // что-то типа "+" в дереве
		application_->setCellText(Vect2i(level - 1, line), label_.c_str());

	application_->setCellText(Vect2i(level, line), node->name());
	application_->setCellText(Vect2i(level + 1, line), node->value());
}

bool ExportTreeToExcel::exportTreeNode(const TreeNode* node, int level, int* line)
{
	if(!application_ || !node)
		return false;
	
	outTreeNode(node, level, *line);
	TreeNode::const_iterator i;
	FOR_EACH(node->children(), i)
		if(!exportTreeNode(*i, level + 1, &(++(*line))))
			return false;

	return true;
}

bool ExportTreeToExcel::exportTreeFromNode(const TreeNode* rootNode)
{
	if(!application_ || !rootNode)
		return false;
	int line = 0;
	return exportTreeNode(rootNode, 0, &line);
}
