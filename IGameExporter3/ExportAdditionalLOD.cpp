#include "StdAfx.h"
#include "RootExport.h"
bool SaverReplaceBlock(const char* in_out_filename,DWORD block_id,int block_size,void* block);
void RootExport::ExportLODHelpers(Saver& saver)
{
	BuildMaterialList();
	saver.push(C3DX_DUPLICATE_INFO_TO_LOD);
		saver.push(C3DX_DUPLICATE_MATERIALS);
		saver<<materials.size();
		for(int imat=0;imat<materials.size();imat++)
			saver<<materials[imat]->GetName();
		saver.pop();
		saver.push(C3DX_DUPLICATE_NODES);//Если что потом прикрутим макросов для версии
		saver<<(int)all_nodes.size();
		for(int inode=0;inode<all_nodes.size();inode++){
			NextLevelNode& n=all_nodes[inode];
			saver<<n.node->GetName();
			saver<<n.current;
			saver<<n.parent;
			saver<<n.additional.size();
			for(int ia=0;ia<n.additional.size();ia++)
				saver<<n.additional[ia]->GetName();
		}
		saver.pop();
	saver.pop();
}

void RootExport::ExportLOD(const char* filename,int lod)
{
	{
		const char* no_data="Недостаточно информации, перезапишите новым экспортёром\n";
		CLoadDirectoryFile dir;
		if(!dir.Load(filename))
		{
			Msg("Не могу прочитать файл %s",filename);
			return;
		}

		IF_FIND_DIR(C3DX_DUPLICATE_INFO_TO_LOD)
		{
			IF_FIND_DATA(C3DX_DUPLICATE_MATERIALS)
			{
				int size;
				rd>>size;
				duplicate_material_names.resize(size);
				for(int i=0;i<size;i++)
				{
					const char* name=rd.LoadString();
					duplicate_material_names[i]=name;
				}
				BuildMaterialList();
			}else
			{
				Msg(no_data);
				return;
			}

			IF_FIND_DATA(C3DX_DUPLICATE_NODES)
			{
				CalcAllNodes();
				CalcNodeMapEasy();
				vector<NextLevelNode> all_nodes_correct;
				int num_nodes;
				rd>>num_nodes;
				all_nodes_correct.resize(num_nodes);
				for(int i=0;i<num_nodes;i++)
				{
					NextLevelNode& n=all_nodes_correct[i];
					const char* name=rd.LoadString();
					n.is_nodelete=true;
					rd>>n.current;
					rd>>n.parent;
					xassert(n.current==i);
					xassert(n.parent<(int)all_nodes_correct.size());

					n.node=Find(name);
					if(n.node==NULL)
					{
						Msg("%s не найдена.\n",name);
						return;
					}

					if(n.parent>=0)
						n.pParent=all_nodes_correct[n.parent].node;

					int num_additional;
					rd>>num_additional;
					n.additional.resize(num_additional);
					for(int ia=0;ia<n.additional.size();ia++)
					{
						const char* aname=rd.LoadString();
						n.additional[ia]=Find(aname);
						if(n.additional[ia]==NULL)
						{
							Msg("%s не найдена.\n",aname);
							return;
						}
					}
				}

				all_nodes=all_nodes_correct;
				CalcNodeMapOptimize();
			}else
			{
				Msg(no_data);
				return;
			}
		}else
		{
			Msg(no_data);
			return;
		}
	}

	MemorySaver save_meshes;
	SaveMeshes(save_meshes);
	if(!SaverReplaceBlock(filename,lod==1?C3DX_ADDITIONAL_LOD1:C3DX_ADDITIONAL_LOD2,save_meshes.length(),save_meshes.buffer()))
	{
		Msg("Не могу записать файл");
		return;
	}
}
