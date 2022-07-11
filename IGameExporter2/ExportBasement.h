#pragma once

class ExportBasement
{
public:
	ExportBasement(Saver& saver,const char* name_);
	void Export(IGameMesh* pobject,IGameNode* node,Matrix3 root_node_matrix);
protected:
	Saver& saver;
	IGameMesh* pobject;
	const char* name;
};
