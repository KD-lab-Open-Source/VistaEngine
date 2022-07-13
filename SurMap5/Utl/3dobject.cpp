#include "stdafx.h"
#include "general.h"


#ifdef _SURMAP_
#define dynamics_init(x)
#endif

/*******************************************************************************
			Load Model function
*******************************************************************************/
Model::Model()
{
	memory_allocation_method = 0;
	num_vert = num_norm = num_poly = 0;
	vertices = 0;
	normals = 0;
	polygons = 0;
	x_off=0; y_off=0; z_off=0;
}
void Model::free()
{
	if(memory_allocation_method)
		delete vertices;
	memory_allocation_method = 0;
	num_vert = num_norm = num_poly = 0;
	vertices = 0;
	normals = 0;
	polygons = 0;
}
void Model::loadC3Dvariable(XBuffer& buf)
{
	int i,j,size;
	int num_vert_total,version;
	int num,vert_ind,norm_ind,sort_info;
	unsigned color_id,color_shift;
	int phi,psi,tetta;

	buf > version;
	if(version != C3D_VERSION_1 && version != C3D_VERSION_3)
		ErrH.Abort("Incorrect C3D version");

	buf > num_vert > num_norm > num_poly > num_vert_total;

	buf > xmax > ymax > zmax;
	buf > xmin > ymin > zmin;
	buf > x_off > y_off > z_off;
	buf > rmax;
	buf > phi > psi > tetta;
	if(version == C3D_VERSION_3)
		buf > volume > rcm > J;

	size = num_vert*sizeof(Vertex) + num_norm*sizeof(Normal) +
		   num_poly*(sizeof(PolygonS) + 3*sizeof(PolygonS*));
	size += num_vert_total*(sizeof(Vertex*) + sizeof(Normal*));


	memory_allocation_method = 1;
	HEAP_BEGIN(size);
	vertices = HEAP_ALLOC(num_vert,Vertex);
	normals = HEAP_ALLOC(num_norm,Normal);
	polygons = HEAP_ALLOC(num_poly,PolygonS);

	if(phi == 83 && psi == 83 && tetta == 83){
		for(i = 0;i < num_vert;i++){
			buf > vertices[i].x > vertices[i].y > vertices[i].z;
			buf > vertices[i].x_8 > vertices[i].y_8 > vertices[i].z_8
			    > sort_info;
			}
		}
	else{
		int ti;
		for(i = 0;i < num_vert;i++){
			buf > ti;
			vertices[i].x = (float)ti;
			buf > ti;
			vertices[i].y = (float)ti;
			buf > ti;
			vertices[i].z = (float)ti;
			buf > vertices[i].x_8 > vertices[i].y_8 > vertices[i].z_8
			    > sort_info;
			}
		}

	for(i = 0;i < num_norm;i++)
		buf > normals[i].x > normals[i].y > normals[i].z
		    > normals[i].n_power > sort_info;

	for(i = 0;i < num_poly;i++){
		buf > num > sort_info
		    > color_id > color_shift
		    > polygons[i].flat_normal.x > polygons[i].flat_normal.y
		    > polygons[i].flat_normal.z > polygons[i].flat_normal.n_power
		    > polygons[i].middle_x > polygons[i].middle_y > polygons[i].middle_z;
		    
		polygons[i].color_id = color_id < COLORS_IDS::MAX_COLORS_IDS ? color_id : COLORS_IDS::BODY;

		polygons[i].num_vert = num;
		polygons[i].vertices = HEAP_ALLOC(num,Vertex*);
		polygons[i].normals = HEAP_ALLOC(num,Normal*);
		for(j = 0;j < num;j++){
			buf > vert_ind > norm_ind;
			polygons[i].vertices[j] = &vertices[vert_ind];
			polygons[i].normals[j] = &normals[norm_ind];
			}
		}

	int poly_ind;
	for(i = 0;i < 3;i++){
		sorted_polygons[i] = HEAP_ALLOC(num_poly,PolygonS*);
		for(j = 0;j < num_poly;j++){
			buf > poly_ind;
			sorted_polygons[i][j] = &polygons[poly_ind];
			}
		}
	HEAP_END;
}

void Model::loadC3D(XBuffer& buf){ loadC3Dvariable(buf); }

/*******************************************************************************
			Objects function
*******************************************************************************/
Object::Object()
{
	i_model = n_models = 0;
	models = 0;
	model = 0;
	bound = 0;
	n_wheels = 0;
	wheels = 0;
	n_debris = 0;
	debris = 0;
	bound_debris = 0;
	active = 0;
	original_scale_size = scale_size = 1;
	collision_object = 0;
	slots_existence = 0;
	memset(data_in_slots,0,MAX_SLOTS*sizeof(Object*));
	old_appearance_storage = 0;

  	set_body_color(COLORS_IDS::BODY_RED);

	ID = ID_VANGER;

	dynamic_state = 0;
	Visibility = 0;
	MapLevel = 0;
	x_of_last_update = y_of_last_update = 0;
	R_curr = R_prev = Vector(0,0,0);
}

Object& Object::operator = (Object& obj)
{
	memcpy(&n_models,&obj.n_models,(char*)&end_of_object_data - (char*)&n_models);
	return *this;
}

void Object::set_body_color(unsigned int color_id)
{
	if(color_id > COLORS_IDS::MAX_COLORS_IDS)
		ErrH.Abort("Bad color id",XERR_USER,color_id);
	body_color_offset = 128;
	body_color_shift = 8;
}	
#ifdef _ROAD_
void Object::convert_to_beeb(Object* beeb)
{
	if(beeb){
		old_appearance_storage = new Object;
		*old_appearance_storage = *this;
		memcpy(&n_models,&beeb -> n_models,(char*)&old_appearance_storage - (char*)&n_models);
		memcpy(&m,&beeb -> m,(char*)&end_of_object_data - (char*)&m);
		}
	else
		if(old_appearance_storage){
			memcpy(&n_models,&old_appearance_storage -> n_models,(char*)&old_appearance_storage - (char*)&n_models);
			memcpy(&m,&old_appearance_storage -> m,(char*)&end_of_object_data - (char*)&m);
			delete old_appearance_storage;
			old_appearance_storage = 0;
			}
	update_coord();
}
#endif

void Object::free()
{
	for(int i = 0;i < n_models;i++)
		models[i].free();
	delete models;
	if(bound){
		bound -> free();
		delete bound;
		}
	if(n_wheels){
		for(i = 0;i < n_wheels;i++)
			if(wheels[i].steer)
				wheels[i].model.free();
		delete wheels;
		}
	if(n_debris){
		for(i = 0;i < n_debris;i++){
			debris[i].free();
			bound_debris[i].free();
			}
		delete debris;
		delete bound_debris;
		}

	i_model = n_models = 0;
	models = 0;
	model = 0;
	bound = 0;
	n_wheels = 0;
	wheels = 0;
	n_debris = 0;
	debris = 0;
	bound_debris = 0;
	slots_existence = 0;
	memset(data_in_slots,0,MAX_SLOTS*sizeof(Object*));
}
void Object::loadM3D(char* name)
{
	n_models = 1;
	models = model = new Model;
	bound = new Model;

	XStream ff(name,XS_IN);
	XBuffer buf(ff.size());
	ff.read(buf.address(),ff.size());
	ff.close();

	model -> loadC3D(buf);
	buf > xmax > ymax > zmax > rmax > n_wheels > n_debris;
	buf > body_color_offset > body_color_shift;
	if(n_wheels)
		wheels = new Wheel[n_wheels];
	if(n_debris){
		debris = new Model[n_debris];
		bound_debris = new Model[n_debris];
		}
	for(int i = 0;i < n_wheels;i++){
		buf > wheels[i].steer > wheels[i].r > wheels[i].width > wheels[i].radius > wheels[i].bound_index;
		if(wheels[i].steer)
			wheels[i].model.loadC3D(buf);
		wheels[i].dZ = 0;
		}

	for(i = 0;i < n_debris;i++){
		debris[i].loadC3D(buf);
		bound_debris[i].loadC3Dvariable(buf);
		}

	bound -> loadC3Dvariable(buf);

	buf > slots_existence;
	if(slots_existence){
		for(i = 0;i < MAX_SLOTS;i++){
			buf > R_slots[i] > location_angle_of_slots[i];
			data_in_slots[i] = 0;
			}
		}

	dynamics_init(name);
}
void Object::loadA3D(char* name)
{
	XStream ff(name,XS_IN);
	XBuffer buf(ff.size());
	ff.read(buf.address(),ff.size());
	ff.close();

	buf > n_models > xmax > ymax > zmax > rmax;
	buf >  body_color_offset > body_color_shift;
	models = new Model[n_models];

	for(int i = 0;i < n_models;i++)
		models[i].loadC3D(buf);
	ID = ID_INSECT;
	model = models;
	dynamics_init(name);
}
void Object::load(char* name,int size_for_m3d)
{
	if(strstr(name,".c3d")){
		n_models = 1;
		models = model = new Model;
		XStream ff(name,XS_IN);
		XBuffer buf(ff.size());
		ff.read(buf.address(),ff.size());
		ff.close();

		model -> loadC3D(buf);
		xmax = model -> xmax;
		ymax = model -> ymax;
		zmax = model -> zmax;
		rmax = model -> rmax;
		dynamics_init(name);
		return;
		}

	if(strstr(name,".m3d")){
		#ifdef _MT_
			scale_size = 0.5;
		#else
			original_scale_size = scale_size = (double)size_for_m3d/256;
		#endif
		loadM3D(name);
		return;
		}
	ErrH.Abort("Unable to recognize 3d model file extension");
}

void Object::lay_to_slot(int slot,Object* weapon)
{
	if(!((1 << slot) & slots_existence))
		return;
	data_in_slots[slot] = weapon;
}


void Object::StartMoleProcess(void){}
int Object::UsingCopterig(int decr_8){ return 0; }
int Object::UsingCrotrig(int decr_8){ return 0; }
int Object::UsingCutterig(int decr_8){ return 0; }
