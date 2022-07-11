#pragma once

class ExportMesh
{
public:
	ExportMesh(Saver& saver,const char* name_);

	void Export(IVisMesh* pobject,int inode);
protected:
	Saver& saver;
	IVisMesh* pobject;
	const char* name;
	int inode_current;

	bool ExportOneMesh(IVisMaterial* mtr);

	enum
	{
		max_bones=4,
	};

	struct sVertex
	{
		Vect3f pos;
		Vect3f norm;
		Vect2f uv;
		Vect2f uv2;

		float bones[max_bones];
		int bones_inode[max_bones];
	};

	struct sPolygon
	{
		int p[3];
	};

	void DeleteSingularPolygon(vector<FaceEx>& faces);
	void SortPolygon(sPolygon* polygon,int n_polygon);

	int NumCashed(sPolygon* polygon,int n_polygon);

	//Треугольник может быть перевернут в IVisMesh относительно Mesh
	int igame_to_mesh_polygon_index[3];
};
