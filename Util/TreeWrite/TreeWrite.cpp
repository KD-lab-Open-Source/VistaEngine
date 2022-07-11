#include "StdAfx.h"
#include "MemoryBuffer.h"
#include "TreeWrite.h"

void TreeWrite::register_buildin_type()
{
	buildin_types.resize(TBIT_MAX);
#define X(id,type) {ObjectType& t=buildin_types[id]; t.type_name=#type; t.size=sizeof(type); t.closed=true; t.save_id=id|FLAG_IS_BUILDIN; }
	X(TBIT_BOOL,bool);
	X(TBIT_CHAR,char);
	X(TBIT_UCHAR,unsigned char);
	X(TBIT_SHORT,short);
	X(TBIT_USHORT,unsigned short);
	X(TBIT_INT,int);
	X(TBIT_UINT,unsigned int);
	X(TBIT_FLOAT,float);
	X(TBIT_DOUBLE,double);

	{//string;
		int id=TBIT_STRING;
		ObjectType& t=buildin_types[id];
		t.type_name="string";
		t.size=sizeof(POINTER_TYPE);
		t.closed=true;
		t.save_id=id|FLAG_IS_BUILDIN;
		t.is_pointer=OTIP_ARRAY;

		ObjectType::Child c;
		c.type=&buildin_types[TBIT_CHAR];
		c.offset=0;
		c.name="";
		t.child.push_back(c);
	}
	
#undef X
}

void TreeRead::register_buildin_type()
{
	buildin_types.resize(TBIT_MAX);
#define X(id,type) {ObjectType& t=buildin_types[id]; t.type_name=#type; t.size=sizeof(type); t.is_pointer=TreeWrite::OTIP_NOT_POINTER; t.is_base=id; }
	X(TBIT_BOOL,bool);
	X(TBIT_CHAR,char);
	X(TBIT_UCHAR,unsigned char);
	X(TBIT_SHORT,short);
	X(TBIT_USHORT,unsigned short);
	X(TBIT_INT,int);
	X(TBIT_UINT,unsigned int);
	X(TBIT_FLOAT,float);
	X(TBIT_DOUBLE,double);

	{
		ObjectType& t=buildin_types[TBIT_STRING];
		t.type_name="string";
		t.size=sizeof(TreeWrite::POINTER_TYPE);
		t.is_pointer=TreeWrite::OTIP_ARRAY;
		t.is_string=true;
		t.is_base=TBIT_STRING;
		ObjectType::Child c;
		c.type=&buildin_types[TBIT_CHAR];
		c.offset=0;
		c.name="";
		t.child.push_back(c);
	}
#undef X
}

//////////////////TreeWrite////////////////////////
TreeWrite::TreeWrite(FileReadWriteFactory* writer)
:writer_(writer),globals_begin(-1),current_save_id(0)
{
	prev_write_declare_new_type_=true;
	repetition_string=false;
	register_buildin_type();
	open_struct("globals");
}

void TreeWrite::close()
{
	close_struct();
	xassert(object_stack.empty());
}

TreeWrite::ObjectType* TreeWrite::RegisterType(int size,const char* type_name,ObjectTypeIsPointer is_pointer,bool is_solid)
{
	prev_write_declare_new_type_=false;
	// ускорение types.find(type_name) за счет того, то мы догадываемс€, какой будет следующий тип в структуре.
	if(!object_stack.empty())
	{
		ObjectStack& cur=object_stack.back();
		if(cur.type->is_pointer==OTIP_ARRAY)
		{
			if(!cur.type->child.empty())
			{
				ObjectType::Child& c=cur.type->child[0];
				if(c.type->type_name==type_name)
					return c.type;
			}
		}else
		if(cur.type->is_pointer==OTIP_NOT_POINTER && cur.type->closed)
		{
			if(cur.current_sub_object<(int)cur.type->child.size())
			{
				ObjectType::Child& c=cur.type->child[cur.current_sub_object];
				if(c.type->type_name==type_name)
					return c.type;
			}
		}
	}

	//преобразование в строчку, new/delete тут есть.
	//сознательно пошли на это, ради €сности общей концепции.
	TypesMap::iterator it=types.find(type_name);
	if(it!=types.end())
	{
		if(is_solid)
			xassert(it->second.size==size);
		xassert(it->second.is_pointer==is_pointer);
		return &it->second;
	}

	ObjectType obj;
	obj.type_name=type_name;
	obj.size=size;
	obj.closed=is_solid;
	obj.is_pointer=is_pointer;
	obj.save_id=current_save_id++;
	types[type_name]=obj;

	prev_write_declare_new_type_=true;
	return &types[type_name];
}
void TreeWrite::open_struct(const char* type_name)
{
	open_struct(type_name,false);
}

void TreeWrite::open_struct(const char* type_name,bool is_array)
{
	ObjectType* cur_type=RegisterType(0,type_name,is_array?OTIP_ARRAY:OTIP_NOT_POINTER,false);
	open_struct(cur_type);
}

void TreeWrite::open_struct(ObjectType* cur_type)
{
	ObjectStack o;
	o.type=cur_type;
	o.current_sub_object=0;
	o.current_sub_offset=0;
	o.offset=stack_write.tell();

	object_stack.push_back(o);
}

void TreeWrite::close_struct()
{
	close_struct(false);
}

void TreeWrite::open_array(const char* name,ObjectType* cur_type)
{
	declare(name,cur_type);

	ObjectStack cur;
	cur.type=cur_type;
	cur.last_pointer=-1;
	cur.last_pointer_type=-1;
	object_stack.push_back(cur);

	open_struct(cur_type->type_name.c_str(),true);
}

void TreeWrite::open_array(const char* name,const char* type_name)
{
	string array_name=type_name;
	//ћожно вместо скобок засунуть type_name array в отдельное пространство имЄн, чтобы избежать лишнего new/delete посто€нно.
	//Ќо это не сильно критично пока.
	array_name+="[]";
	ObjectType* cur_type=RegisterType(sizeof(POINTER_TYPE),array_name.c_str(),OTIP_ARRAY,false);
	open_array(name,cur_type);
}

void TreeWrite::open_array(const char* name,TreeBuildInType type)
{
	ObjectType* cur_type=&buildin_types[type];
	open_array(name,cur_type);
}

TreeWrite::POINTER_TYPE TreeWrite::close_array()
{
	if(object_stack.back().type->child.empty())
		declare("empty",&buildin_types[TBIT_BOOL]);

	close_struct(true);
	//write array ptr
	POINTER_TYPE ptr;
	{
		xassert(!object_stack.empty());
		ObjectStack& cur=object_stack.back();
		xassert(cur.is_array());
		ptr=cur.last_pointer;
		stack_write.write(&cur.last_pointer,sizeof(cur.last_pointer));
	}

	object_stack.pop_back();

	{
		ObjectStack& parent=object_stack.back();
		parent.current_sub_offset+=sizeof(POINTER_TYPE);
		parent.current_sub_object++;
	}

	return ptr;
}

void TreeWrite::open_ptr(const char* name,const char* type_name)
{
	ObjectType* cur_type=RegisterType(FULL_POINTER_SIZE,type_name,OTIP_POINTER,true);
	declare(name,cur_type);
	xassert(!object_stack.empty());

	ObjectStack cur;
	cur.type=cur_type;
	cur.last_pointer=-1;
	cur.last_pointer_type=-1;
	object_stack.push_back(cur);
}

void TreeWrite::close_ptr()
{
	{
		xassert(!object_stack.empty());
		ObjectStack& cur=object_stack.back();
		xassert(cur.is_pointer());
		stack_write.write(&cur.last_pointer,sizeof(cur.last_pointer));
		stack_write.write(&cur.last_pointer_type,sizeof(cur.last_pointer_type));
	}

	object_stack.pop_back();

	{
		xassert(!object_stack.empty());
		ObjectStack& parent=object_stack.back();
		parent.current_sub_offset+=FULL_POINTER_SIZE;
		parent.current_sub_object++;
	}
}


void TreeWrite::write_string(const char* name,const string& str)
{
	if(repetition_string)
	{
		map<string,POINTER_TYPE>::iterator it=repetition_string_map.find(str);
		if(it!=repetition_string_map.end())
		{
			POINTER_TYPE ptr=it->second;
			stack_write.write(&ptr,sizeof(POINTER_TYPE));

			{
				ObjectStack& parent=object_stack.back();
				parent.current_sub_offset+=sizeof(POINTER_TYPE);
				parent.current_sub_object++;
			}

			return;
		}
	}

	open_array(name,TBIT_STRING);
	write_data_array(&str[0],TBIT_CHAR,(int)str.size());
	POINTER_TYPE ptr=close_array();
	if(repetition_string)
	{
		repetition_string_map[str]=ptr;
	}
}

void TreeWrite::close_struct(bool is_array)
{
	xassert(!object_stack.empty());

	ObjectStack o=object_stack.back();
	xassert(o.is_array()==is_array);
	xassert(!o.is_pointer());

	if(!object_stack.back().type->closed)
	{
		object_stack.back().type->close();
	}
	object_stack.pop_back();

	if(object_stack.empty() )
	{
		xassert(globals_begin==-1);
		globals_begin=(POINTER_TYPE)data_write.tell();

		xassert(o.current_sub_offset-o.offset==o.type->size);
		data_write.copy(stack_write,o.offset,o.type->size);
	}else
	{
		ObjectStack& parent=object_stack.back();

		bool o_is_array=o.is_array();
		if(o_is_array)
			xassert(o.current_sub_offset==o.type->child[0].type->size*o.current_sub_object);
		else
			xassert(o.current_sub_offset==o.type->size);

		if(o_is_array)
		{
			parent.last_pointer=(POINTER_TYPE)data_write.tell();
			parent.last_pointer_type=o.type->save_id;

			data_write.write(&o.current_sub_object,sizeof(o.current_sub_object));
			data_write.copy(stack_write,o.offset,o.current_sub_offset);
			stack_write.seek(o.offset);
		}else
		{
			if(parent.is_array())
			{
				ObjectType* cur=parent.type->child[0].type;
				parent.current_sub_offset+=cur->size;
				parent.current_sub_object++;
			}else
			if(parent.is_pointer())
			{
				parent.last_pointer=(POINTER_TYPE)data_write.tell();
				parent.last_pointer_type=o.type->save_id;
				
				data_write.copy(stack_write,o.offset,o.type->size);
				stack_write.seek(o.offset);
			}else
			if(parent.current_sub_object<(int)parent.type->child.size())
			{
			#ifdef _DEBUG
				ObjectType* cur=parent.type->child[parent.current_sub_object].type;
				xassert(cur==o.type);
			#endif
				parent.current_sub_offset+=o.type->size;
				parent.current_sub_object++;
			}
		}
	}
}

void TreeWrite::declare_data(const char* name,const char* type_name)
{
	declare(0,name,type_name,OTIP_NOT_POINTER,false);
}

void TreeWrite::declare(int size,const char* name,const char* type_name,ObjectTypeIsPointer is_pointer,bool is_solid)
{
	ObjectType* cur_type=RegisterType(size,type_name,is_pointer,is_solid);
	declare(name,cur_type);
}

void TreeWrite::declare(const char* name,ObjectType* cur_type)
{
	xassert(!object_stack.empty());
	ObjectStack& parent=object_stack.back();

	if(parent.is_array())
	{
		if(!parent.type->child.empty())
		{
			ObjectType::Child& c=parent.type->child[0];
			xassert(c.type->type_name==cur_type->type_name);
			xassert(c.offset+c.type->size*parent.current_sub_object==parent.current_sub_offset);
		}else
		{
			xassert(!parent.type->closed);

			ObjectType::Child c;
			c.type=cur_type;
			c.offset=parent.current_sub_offset;
			c.name.clear();
			parent.type->child.push_back(c);
			parent.type->closed=true;
		}
	}else
	{
		if(parent.current_sub_object<(int)parent.type->child.size())
		{
			ObjectType::Child& c=parent.type->child[parent.current_sub_object];
			xassert(c.name==name);
			xassert(c.type->type_name==cur_type->type_name);
			xassert(c.type->size==cur_type->size);
			xassert(c.offset==parent.current_sub_offset);
		}else
		{
			xassert(!parent.type->closed);

			ObjectType::Child c;
			c.type=cur_type;
			c.offset=parent.current_sub_offset;
			c.name=name;
			parent.type->child.push_back(c);
		}
	}
}


void TreeWrite::write_data(const void* data,int size,const char* name,const char* type_name)
{
	declare(size,name,type_name,OTIP_NOT_POINTER,true);

	stack_write.write(data,size);

	ObjectStack& parent=object_stack.back();
	parent.current_sub_offset+=size;
	parent.current_sub_object++;
}

void TreeWrite::write_data(const void* data,const char* name,TreeBuildInType type)
{
	ObjectType* cur_type=&buildin_types[type];
	declare(name,cur_type);
 
	stack_write.write(data,cur_type->size);

	ObjectStack& parent=object_stack.back();
	parent.current_sub_offset+=cur_type->size;
	parent.current_sub_object++;
}

void TreeWrite::write_data_array(const void* data,int size,const char* type_name,int num)
{
	xassert(object_stack.back().type->is_pointer==OTIP_ARRAY);
	declare(size,"",type_name,OTIP_NOT_POINTER,true);

	stack_write.write(data,size*num);

	ObjectStack& parent=object_stack.back();
	parent.current_sub_offset+=size*num;
	parent.current_sub_object+=num;
}

void TreeWrite::write_data_array(const void* data,TreeBuildInType type,int num)
{
	xassert(object_stack.back().type->is_pointer==OTIP_ARRAY);
	ObjectType* cur_type=&buildin_types[type];
	declare("",cur_type);
 
	stack_write.write(data,cur_type->size*num);

	ObjectStack& parent=object_stack.back();
	parent.current_sub_offset+=cur_type->size*num;
	parent.current_sub_object+=num;
}

void TreeWrite::dump_structure()
{
	FILE* f=fopen("dump.h","wt");
	for(TypesMap::iterator it=types.begin();it!=types.end();it++)
	{
		ObjectType& ot=it->second;
		if(ot.child.empty())
		{
			fprintf(f,"%s //size=%i \n",ot.type_name.c_str(),ot.size);
		}
	}
	
	for(TypesMap::iterator it=types.begin();it!=types.end();it++)
	{
		ObjectType& ot=it->second;
		if(ot.child.empty())
			continue;
		fprintf(f,"%s { //size=%i \n",ot.type_name.c_str(),ot.size);
//		sort(ot.child.begin(),ot.child.end());
		for(int i=0;i<(int)ot.child.size();i++)
		{
			ObjectType::Child& c=ot.child[i];
			fprintf(f,"\t%s %s;//offset=%i %s\n",c.type->type_name.c_str(),c.name.c_str(),c.offset,c.type->is_pointer?"pointer":"");
		}
		fprintf(f,"}\n");
	}

	fprintf(f,"//-----------------------------------\n");
	fprintf(f,"globals_begin=%xh\n",globals_begin);
	fclose(f);
}

void TreeWrite::write_all(const char* file_name)
{
	WriteAdapter* f;
	f=writer_->open_write(file_name);
	unsigned int abbreviature='rmlb';
	f->write(&abbreviature,sizeof(abbreviature));
	f->write(&globals_begin,sizeof(globals_begin));
	write_buffer(f,data_write);
	write_structure(f);
	f->close();
}

void TreeWrite::write_buffer(WriteAdapter* f,MemoryBuffer& buf)
{
	POINTER_TYPE p=(POINTER_TYPE)buf.size();

	f->write(&p,sizeof(POINTER_TYPE));
	for(int i=0;i<buf.num_block();i++)
	{
		void* ptr;
		size_t size;
		buf.get_block_ptr(i,ptr,size);
		f->write(ptr,size);
	}
}

void TreeWrite::write_structure(WriteAdapter* f)
{
	POINTER_TYPE types_size=(POINTER_TYPE)types.size();
#define WR(x) f->write(&x,sizeof(x))
#define WRS(x) {POINTER_TYPE sz=(POINTER_TYPE)x.size();f->write(&sz,sizeof(sz));if(sz)f->write((x).data(),sz);}
	WR(types_size);

	xassert(types_size==current_save_id);
	vector<ObjectType*> objects(types_size,0);
	POINTER_TYPE save_id=0;
	for(TypesMap::iterator it=types.begin();it!=types.end();++it)
	{
		ObjectType& t=it->second;
		xassert(t.save_id<types_size);
		objects[t.save_id]=&t;
	}
	
	for(vector<ObjectType*>::iterator it=objects.begin();it!=objects.end();++it)
	{
		ObjectType& t=**it;
		WR(t.save_id);
		WRS(t.type_name);
		WR(t.size);
		unsigned int flags=t.is_pointer;
		WR(flags);

		POINTER_TYPE child_size=(POINTER_TYPE)t.child.size();
		WR(child_size);
		for(POINTER_TYPE i=0;i<child_size;i++)
		{
			ObjectType::Child& c=t.child[i];
			WR(c.type->save_id);
			WR(c.offset);
			WRS(c.name);
		}
	}

#undef WR
#undef WRS
}

//////////////////////////////////TreeRead///////////////
TreeRead::TreeRead(FileReadWriteFactory* writer)
:writer_(writer),data(0),data_size(0)
{
	register_buildin_type();
}

TreeRead::~TreeRead()
{
	delete data;
}

bool TreeRead::read(const char* file_name)
{
	delete data;
	data=0;
	data_size=0;

	ReadAdapter* f=writer_->open_read(file_name);
	if(f==0)
		return false;

#define RD(x) f->read(&x,sizeof(x))
//≈сли формат внутренний string изменитс€ можно облажатьс€ сильно, да.
#define RDS(x) { unsigned int size;f->read(&size,sizeof(size)); x.resize(size); f->read((void*)x.data(),size); }

	unsigned int abbreviature;
	if(!RD(abbreviature))
	{
		f->close();
		return false;
	}
	if(abbreviature!='rmlb')
	{
		f->close();
		return false;
	}

	RD(globals.begin_offset);
	//read data
	RD(data_size);
	data=new unsigned char[data_size];
	f->read(data,data_size);

	//read types
	{
		POINTER_TYPE types_size;
		RD(types_size);
		types.resize(types_size);
		for(POINTER_TYPE itype=0;itype<types_size;itype++)
		{
			ObjectType& t=types[itype];
			POINTER_TYPE save_id;
			RD(save_id);
			xassert(save_id==itype);
			RDS(t.type_name);
			RD(t.size);
			unsigned int flags;
			RD(flags);
			t.is_pointer=(TreeWrite::ObjectTypeIsPointer)flags;

			POINTER_TYPE child_size;
			RD(child_size);
			t.child.resize(child_size);
			for(POINTER_TYPE i=0;i<child_size;i++)
			{
				ObjectType::Child& c=t.child[i];
				POINTER_TYPE save_id;
				RD(save_id);

				c.type=get_type(save_id);

				RD(c.offset);
				RDS(c.name);
			}

		}
	}

#undef RD
	f->close();

	for(POINTER_TYPE itype=0;itype<types.size();itype++)
	{
		ObjectType& t=types[itype];
		if(t.type_name=="globals")
		{
			globals.type=&t;
			break;
		}
	}
	if(globals.type==0)
		return false;

	return true;
}

void TreeRead::dump_all()
{
	FILE* f=fopen("read_dump.h","wt");
	if(globals.type)
		dump_struct(f,globals,0);
	fclose(f);
}

int TreeRead::get_num_object(const ObjectHandle& p)
{
	xassert(p.is_struct());
	return (int)p.type->child.size();
}

TreeRead::ObjectHandle TreeRead::get_sub_object(const ObjectHandle& p,int i)
{
	ObjectType::Child& c=p.type->child[i];
	return ObjectHandle(p.begin_offset+c.offset,c.type);
}

const char* TreeRead::get_sub_object_name(const ObjectHandle& p,int i)
{
	xassert(p.is_struct());
	ObjectType::Child& c=p.type->child[i];
	return c.name.c_str();
}

void TreeRead::dump_tab(FILE* f,int recursion_num)
{
	for(int i=0;i<recursion_num-1;i++)
		fprintf(f,"\t");
}

TreeRead::ObjectHandle TreeRead::get_ptr(const ObjectHandle& p)
{
	if(p.is_array())
	{
		POINTER_TYPE offset=*(POINTER_TYPE*)(data+p.begin_offset);
		return ObjectHandle(offset,p.type);
	}else
	{
		POINTER_TYPE offset=*(POINTER_TYPE*)(data+p.begin_offset);
		ObjectType* ptr_type;
		if(offset==(POINTER_TYPE)-1)
		{
			ptr_type=0;
		}else
		{
			POINTER_TYPE type_id=*(POINTER_TYPE*)(data+p.begin_offset+sizeof(POINTER_TYPE));
			ptr_type=get_type(type_id);
		}

		return ObjectHandle(offset,ptr_type);
	}
}

void TreeRead::get_string(const ObjectHandle& p,string& str)
{
	ObjectHandle ptr=get_ptr(p);
	POINTER_TYPE size=*(POINTER_TYPE*)(data+ptr.begin_offset);
	str.assign((char*)(data+ptr.begin_offset+sizeof(POINTER_TYPE)),size);
}

TreeRead::POINTER_TYPE TreeRead::get_array_size(const ObjectHandle& ptr)
{
	return *(POINTER_TYPE*)(data+ptr.begin_offset);
}

TreeRead::ObjectHandle TreeRead::get_array_elem(const ObjectHandle& ptr,POINTER_TYPE i)
{
	ObjectType* child_type=ptr.type->child[0].type;
	return ObjectHandle(ptr.begin_offset+i*child_type->size+sizeof(POINTER_TYPE),child_type);
}

void TreeRead::dump_struct(FILE* f,ObjectHandle p,int recursion_num)
{
	if(p.is_pointer())
	{
		if(p.is_string())
		{
			string s;
			get_string(p,s);
			fprintf(f,"\"%s\"",s.c_str());
		}else
		{
			ObjectHandle ptr=get_ptr(p);

			if(ptr.is_null())
			{
				//dump_tab(f,recursion_num+1);
				fprintf(f,"NULL");
			}else
			if(p.is_array())
			{
				POINTER_TYPE size=get_array_size(ptr);

				fprintf(f,"{//%s\n",ptr.type->type_name.c_str());
				dump_tab(f,recursion_num+1);
				fprintf(f,"%i;\n",size);

				for(POINTER_TYPE i=0;i<size;i++)
				{
					dump_tab(f,recursion_num+1);
					dump_struct(f,get_array_elem(ptr,i),recursion_num+1);
					fprintf(f,i!=size-1?",\n":"\n");
				}

				dump_tab(f,recursion_num);fprintf(f,"}");
			}else
			{
				xassert(p.is_pointer());
				dump_struct(f,ptr,recursion_num);
			}
		}
	}else
	if(p.is_solid())
	{
		switch(p.is_base())
		{
		case TBIT_BOOL:	fprintf(f,get_bool(p)?"true":"false");	break;
		case TBIT_CHAR: fprintf(f,"%i",get_char(p)); break;
		case TBIT_UCHAR: fprintf(f,"%i",get_uchar(p)); break;
		case TBIT_SHORT: fprintf(f,"%i",get_short(p)); break;
		case TBIT_USHORT: fprintf(f,"%i",get_ushort(p)); break;
		case TBIT_INT: fprintf(f,"%i",get_int(p)); break;
		case TBIT_UINT: fprintf(f,"%i",get_uint(p)); break;
		case TBIT_FLOAT: fprintf(f,"%f",get_float(p)); break;
		case TBIT_DOUBLE: fprintf(f,"%f",get_double(p)); break;
		default:
			fprintf(f,"unknown %s",p.type_name());
		}
	}else
	{
		if(recursion_num)
			fprintf(f,"\"%s\" {\n",p.type->type_name.c_str());
		int num_object=get_num_object(p);
		for(int i=0;i<num_object;i++)
		{
			ObjectHandle child=get_sub_object(p,i);
			const char* name=get_sub_object_name(p,i);
			dump_tab(f,recursion_num+1);
			fprintf(f,"%s = ",name);
			dump_struct(f,child,recursion_num+1);
			fprintf(f,";\n");
		}
		if(recursion_num)
		{
			dump_tab(f,recursion_num);
			fprintf(f,"}");
		}
	}
}

void TreeRead::begin_read_struct(const ObjectHandle& object,int& it)
{
	it=0;
}

void TreeRead::end_read_struct(const ObjectHandle& object,int& it)
{
#ifdef _DEBUG
	if(object.type->conv==IT_LINEAR && !object.is_array())
	{
		xassert(it==object.type->child.size());
	}
#endif
	if(object.type->conv==IT_FIRST_READ)
		object.type->conv=IT_LINEAR;
}

bool TreeRead::next_sub_object_by_name(const ObjectHandle& object,const char* name,ObjectHandle& sub_object,int& it)
{
	ObjectType* t=object.type;
	if(object.is_array())
	{
		int size=get_array_size(object);
		xassert(it<get_array_size(object));
		sub_object=get_array_elem(object,it);
		it++;
		return true;
	}

	//!!!!!!
	//“ут ещЄ встроить что нить.
	//≈сли первый раз читаетс€ текстура и последовательность чтени€ линейна€,
	//то по окончании чтении текстуры выставить флаг - всЄ последовательно читаетс€
	//и при последующих чтени€х значит читать без сравнени€ строкового.
	int child_size=(int)t->child.size();
	if(t->conv==IT_LINEAR)
	{
		xassert(it<child_size);
		sub_object=get_sub_object(object,it);
		it++;
		return true;
	}

	for(int i=it;i<child_size;i++)
	{
		if(t->child[i].name==name)
		{
			if(i!=it)
				t->conv=IT_RANDOM_READ;
			sub_object=get_sub_object(object,i);
			it=i+1;
			return true;
		}
	}

	t->conv=IT_RANDOM_READ;

	for(int i=0;i<it;i++)
	{
		if(t->child[i].name==name)
		{
			sub_object=get_sub_object(object,i);
			it=i+1;
			return true;
		}
	}

	return false;
}
