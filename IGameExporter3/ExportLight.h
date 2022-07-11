#pragma once

class ExportLight
{
public:
	ExportLight(Saver& saver,const char* name_);
	void Export(IVisLight* pobject,int inode, ChainsBlock& chains_block);
protected:
	Saver& saver;
	IVisLight* pobject;
	IVisNode* node;
	const char* name;
	int inode_current;
	GenLight* LightObj;

	void SaveColor(int interval_begin,int interval_size,bool cycled, ChainsBlock& chains_block);
};
