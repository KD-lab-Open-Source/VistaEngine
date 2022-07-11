#pragma once

class ExportBasement
{
public:
	ExportBasement(Saver& saver,const char* name_);
	void Export(IVisMesh* pobject,IVisNode* node,Matrix3 root_node_matrix);
protected:
	Saver& saver;
	IVisMesh* pobject;
	const char* name;
};
