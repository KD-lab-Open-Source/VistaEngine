#include "stdafx.h"

/*
Экспортируем для LOD только меши с нулевого кадра.
Нужно дополнительно экспортировать при экспорте
node name и node index

При экспорте ЛОДов эта информация читается.
А сами лоды в конец пишутся.
*/


bool SaverReplaceBlock(const char* out_filename,const char* in_filename,DWORD block_id,int block_size,void* block)
{
	xassert(sizeof(CLoadData)==2*4);
	FileSaver out;
	if(!out.Init(out_filename))
		return false;
	CLoadDirectoryFile dir;

	if(!dir.Load(in_filename))
		return false;
	while(CLoadData* ld=dir.next())
	if(ld->id!=block_id)
	{
		out.write(ld,ld->size+sizeof(CLoadData));
	}

	out<<block_id;
	out<<block_size;
	out.write(block,block_size);
	return true;
}

bool SaverReplaceBlock(const char* in_out_filename,DWORD block_id,int block_size,void* block)
{
	char out_file_name[_MAX_PATH];
	strncpy(out_file_name,in_out_filename,_MAX_PATH);
	strncat(out_file_name,"$$",_MAX_PATH);
	if(!SaverReplaceBlock(out_file_name,in_out_filename,block_id,block_size,block))
		return false;
	remove(in_out_filename);
	return rename(out_file_name,in_out_filename)==0;
}
