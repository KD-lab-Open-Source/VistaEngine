#pragma once

class ExportMaterial
{
public:
	ExportMaterial(Saver& saver);
	bool Export(Saver& saver, IVisMaterial * mat, ChainsBlock& chains_block);
protected:
	Saver& saver_;
	IVisMaterial * mat;
	Mtl* max_mat;

	void ExportOpacity(Saver& saver, int interval_begin,int interval_size,bool cycled, ChainsBlock& chains_block);
	void ExportUV(Saver& saver, IVisTexmap* texmap,int interval_begin,int interval_size,bool cycled, int index, ChainsBlock& chains_block);
};
