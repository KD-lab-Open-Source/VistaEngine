#pragma once

class ExportMaterial
{
public:
	ExportMaterial(Saver& saver);
	bool Export(Saver& saver, IGameMaterial * mat, ChainsBlock& chains_block);
protected:
	Saver& saver_;
	IGameMaterial * mat;
	Mtl* max_mat;

	void ExportOpacity(Saver& saver, int interval_begin,int interval_size,bool cycled, ChainsBlock& chains_block);
	void ExportUV(Saver& saver, Texmap* texmap,int interval_begin,int interval_size,bool cycled, int index, ChainsBlock& chains_block);
};
