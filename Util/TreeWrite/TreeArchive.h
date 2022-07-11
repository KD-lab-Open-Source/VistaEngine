#pragma once
#include "TreeWrite.h"

class TreeOArchive : public Archive
{
public:
	TreeOArchive(const char* fname)
	{
		open(fname);
	}
	TreeOArchive()
	{
	}

	~TreeOArchive()
	{
		close();
	}

	void open(const char* fname)
	{
		if(fname)
			file_name=fname;
		else
			file_name.clear();
		w.refrain_from_repetition_string(true);
	}

	bool close()
	{

		if(!file_name.empty())
		{
			WriteEnumDescriptors();
			w.close();
			w.write_all(file_name.c_str());
		}
		file_name.clear();
		return true;
	}

	bool isOutput() const { return true; }
	bool needDefaultArchive() const{ return false;}

	void setNodeType (const char*) {}
protected:
	string file_name;
	TreeWrite w;

	bool processBinary (XBuffer& buffer, const char* name, const char* nameAlt)
	{
		w.open_array(name,TBIT_CHAR);
		w.write_data_array(buffer.buffer(),TBIT_CHAR,buffer.size());
		w.close_array();
		return true;
	}
/*
	int processEnum (const ComboListString& comboList, const char* name, const char* nameAlt) {
		//Оптимизация чтения/записи enum или BitVector - элементарно на этом уровне проводится.
		w.write_string(name,comboList.value ().c_str ());
		return -1;
	}
	ComboInts processBitVector (const ComboInts& flags, const ComboStrings& comboList, const ComboStrings& comboListAlt, const char* name, const char* nameAlt) {
		//Довольно неудачная абстракция ComboInts/comboList. Поэтому такая кривая реализация.
		//По хорошему должен передаваться uint32 и как какой битик называется.
		//Это позволило бы избежать двойной конвенвенртации.
		w.open_array(name,"BitVector");
		ComboInts::const_iterator it;
		FOR_EACH (flags, it) {
			w.write_string("x",comboList [*it].c_str());
		}
		w.close_array();
		return ComboInts ();
	}
/*/
	bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt)
	{
		w.write_data(&value,sizeof(value),name,descriptor.typeName());
		if(w.prev_write_declare_new_type())
			StoreEnumDescriptor(descriptor);
		return true;
	}

	bool processBitVector(int& flags, const EnumDescriptor& descriptor,const char* name, const char* nameAlt)
	{
		//По хорошему нужно отличать от processEnum тип, потому как разная конвертация.
		w.write_data(&flags,sizeof(flags),name,descriptor.typeName());
		if(w.prev_write_declare_new_type())
			StoreEnumDescriptor(descriptor);
		return true;
	}
/**/

	bool processValue(char& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_CHAR);
		return true;
	}
	bool processValue(signed char& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_CHAR);
		return true;
	}
	bool processValue(signed short& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_SHORT);
		return true;
	}
	bool processValue(signed int& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_INT);
		return true;
	}
	bool processValue(signed long& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_INT);
		return true;
	}
	bool processValue(unsigned char& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_UCHAR);
		return true;
	}
	bool processValue(unsigned short& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_USHORT);
		return true;
	}
	bool processValue(unsigned int& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_UINT);
		return true;
	}
	bool processValue(unsigned long& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_UINT);
		return true;
	}
	bool processValue(float& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_FLOAT);
		return true;
	}
	bool processValue(double& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_DOUBLE);
		return true;
	}
	bool processValue(PrmString& t, const char* name, const char* nameAlt) {
		w.write_string(name,t.value());
		return true;
	}
	bool processValue(ComboListString& t, const char* name, const char* nameAlt) {
		w.write_string(name,t.value());
		return true;
	}
	bool processValue(string& str, const char* name, const char* nameAlt) { 
		w.write_string(name,str);
		return true;
	}
	bool processValue(bool& value, const char* name, const char* nameAlt) {
		w.write_data(&value,name,TBIT_BOOL);
		return true;
	}

protected:
	bool openStruct(const char* name, const char* nameAlt, const char* typeName) {
		w.declare_data(name,typeName);
		w.open_struct(typeName);
		return true;
	}
	void closeStruct (const char* name) {
		w.close_struct();
	}
	bool openContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int& _size, bool readOnly) {
		w.open_array(name,elementTypeName);
		return true;
	}
	void closeContainer (const char* name) {
		w.close_array();
	}

	int openPointer (const char* name, const char* nameAlt,
		const char* baseName, const char* baseNameAlt,
		const char* typeName, const char* typeNameAlt) {
		string ptypeName=baseName;
		ptypeName+=" *";
		w.open_ptr(name,ptypeName.c_str());
		if(typeName)
			w.open_struct(typeName);
		return NULL_POINTER;
	}

	void closePointer (const char* name, const char* baseName, const char* typeName) {
		if(typeName)
			w.close_struct();
		w.close_ptr();
	}


	void StoreEnumDescriptor(const EnumDescriptor& descriptor)
	{
		all_enum.push_back(&descriptor);
	}

	vector<const EnumDescriptor*> all_enum;

	void WriteEnumDescriptors()
	{
		if(all_enum.empty())
			return;
		const char* e="!!!enums!!!";
		w.open_array(e,e);
		for(size_t i=0;i<all_enum.size();i++)
		{
			const EnumDescriptor* f=all_enum[i];

			const char* e_struct="!!!enums!!!struct";
			w.declare_data(e_struct,e_struct);
			w.open_struct(e_struct);
			w.write_string("name",f->typeName());

			const char* e_elem="!!!enums!!!array";
			w.open_array(e_elem,e_elem);

			for(EnumDescriptor::NameToKey::const_iterator it=f->beginNameToKey();it!=f->endNameToKey();++it)
			{
				const char* e_elem_one="!!!enums!!!element!one";
				w.declare_data(e_elem_one,e_elem_one);
				w.open_struct(e_elem_one);

				w.write_string("name",it->first.c_str());
				w.write_data(&it->second,"value",TBIT_INT);

				w.close_struct();
			}

			w.close_array();
			w.close_struct();
		}

		w.close_array();
	}

};


class TreeIArchive : public Archive
{
public:
	TreeIArchive(const char* fname)
	{
		open(fname);
	}
	TreeIArchive()
	{
	}
	~TreeIArchive()
	{
		close();
	}

	bool open(const char* fname)
	{
		string s=fname;
		if(!r.read(s.c_str()))
			return false;
		StackStruct next;

		next.handle=r.get_root();
		r.begin_read_struct(next.handle,next.it);
		stack.push_back(next);

		ReadEnums();
		return true;

	}
	bool close()
	{
		stack.pop_back();
		xassert(stack.empty());
		return true;
	}

	bool needDefaultArchive() const { return false; }

protected:
	TreeRead r;
	struct StackStruct
	{
		int it;
		TreeRead::ObjectHandle handle;
	};

	vector<StackStruct> stack;

	struct OneEnum
	{
		string name;
		typedef StaticMap<int, string> KeyToName;
		KeyToName keyToName;
	};

	vector<OneEnum> enums;

	struct ltstr
	{
		bool operator()(const char* s1, const char* s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};
	typedef StaticMap<const char*,OneEnum*,ltstr> TypeEnumsSorted;
	TypeEnumsSorted enums_sorted;
	
	bool processBinary (XBuffer& buffer, const char* name, const char* nameAlt)
	{
		xassert(0);
		return true;
	}
/*
	int processEnum (const ComboListString& comboList, const char* name, const char* nameAlt)
	{
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		string str;
		r.get_string(sub_object,str);
		int result = indexInComboListString (comboList.comboList(), str.c_str());

		return result;
	}
	ComboInts processBitVector (const ComboInts& flags, const ComboStrings& comboList, const ComboStrings& comboListAlt, const char* name, const char* nameAlt) {
		int size=0;
		if(!openContainer (name,nameAlt, "BitVector",size,true))
			return ComboInts ();
		ComboInts result;
		for(int i=0;i<size;i++)
		{
			string name;
			processValue(name,"x",0);
			ComboStrings::const_iterator it;
			it = std::find (comboList.begin (), comboList.end (), name);
			if(it != comboList.end ()) {
				result.push_back (std::distance (comboList.begin (), it));
			}
		}
		closeContainer(name);
		return result;

	}
/*/
	bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt)
	{
		//Пущай так!
		//user_type_data==0 - первое обращение
		//user_type_data==1 - нет конвертации
		//user_type_data==2 - конвертация

		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;

		if(sub_object.user_type_data()==0)
			sub_object.user_type_data()=EnumIsChanged(descriptor)?2:1;

		value=r.get_int(sub_object);
		if(sub_object.user_type_data()==2)
			ConvertEnum(value,descriptor);
		return true;
	}

	bool processBitVector(int& value, const EnumDescriptor& descriptor,const char* name, const char* nameAlt)
	{
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;

		if(sub_object.user_type_data()==0)
			sub_object.user_type_data()=EnumIsChanged(descriptor)?2:1;

		value=r.get_int(sub_object);
		if(sub_object.user_type_data()==2)
			ConvertBitVector(value,descriptor);
		return true;
	}
/**/
	bool processValue(char& value, const char* name, const char* nameAlt) 
	{
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_char(sub_object);
		return true;
	} 

	bool processValue(signed char& value, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_char(sub_object);
		return true;
	}
	bool processValue(signed short& value, const char* name, const char* nameAlt) 
	{
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_short(sub_object);
		return true;
	}
	bool processValue(signed int& value, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_int(sub_object);
		return true;
	}
	bool processValue(signed long& value, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_int(sub_object);
		return true;
	}
	bool processValue(unsigned char& value, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_uchar(sub_object);
		return true;
	}
	bool processValue(unsigned short& value, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_ushort(sub_object);
		return true;
	}
	bool processValue(unsigned int& value, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_uint(sub_object);
		return true;
	}
	bool processValue(unsigned long& value, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_uint(sub_object);
		return true;
	}
	bool processValue(float& value, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_float(sub_object);
		return true;
	}
	bool processValue(double& value, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_double(sub_object);
		return true;
	}
	bool processValue(PrmString& t, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		string value;
		r.get_string(sub_object,value);
		t=value;
		return true;
	}
	bool processValue(ComboListString& t, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		string value;
		r.get_string(sub_object,value);
		t=value;
		return true;
	}
	bool processValue(string& value, const char* name, const char* nameAlt) { 
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		r.get_string(sub_object,value);
		return true;
	}
	bool processValue(bool& value, const char* name, const char* nameAlt) {
		StackStruct& cur_struct=stack.back();
		TreeRead::ObjectHandle sub_object;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,sub_object,cur_struct.it))
			return false;
		value=r.get_bool(sub_object);
		return true;
	}

protected:
	bool openStruct(const char* name, const char* nameAlt, const char* typeName) {
		StackStruct& cur_struct=stack.back();
		StackStruct next;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,next.handle,cur_struct.it))
			return false;

		r.begin_read_struct(next.handle,next.it);
		stack.push_back(next);
		
		return true;
	}
	void closeStruct (const char* name) {
		StackStruct& cur_struct=stack.back();
		r.end_read_struct(cur_struct.handle,cur_struct.it);
		stack.pop_back();
	}
	bool openContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int& _size, bool readOnly) {
		StackStruct& cur_struct=stack.back();
		StackStruct next;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,next.handle,cur_struct.it))
			return false;
		xassert(next.handle.is_pointer());
		next.handle=r.get_ptr(next.handle);
		r.begin_read_struct(next.handle,next.it);
		stack.push_back(next);

		xassert(next.handle.is_array());
		_size=r.get_array_size(next.handle);
		
		return true;
	}
	void closeContainer (const char* name) {
		closeStruct(name);
	}

	int openPointer (const char* name, const char* nameAlt,
		const char* baseName, const char* baseNameAlt,
		const char* typeName, const char* typeNameAlt) {
		StackStruct& cur_struct=stack.back();
		StackStruct next;
		if(!r.next_sub_object_by_name(cur_struct.handle,name,next.handle,cur_struct.it))
			return NULL_POINTER;
		xassert(next.handle.is_pointer());
		next.handle=r.get_ptr(next.handle);

		if(next.handle.is_null())
			return NULL_POINTER;
		int result = indexInComboListString (typeName, next.handle.type_name());
		if (result == NULL_POINTER) {
			XBuffer msg;
			msg < "ERROR! no such class registered: ";
			msg < next.handle.type_name();
			xassertStr (0, static_cast<const char*>(msg));
			return UNREGISTERED_CLASS;
		}

		r.begin_read_struct(next.handle,next.it);
		stack.push_back(next);

		return result;
	}

	void closePointer (const char* name, const char* baseName, const char* derivedName) {
		if(!baseName)
			return;
		StackStruct& cur_struct=stack.back();
		r.end_read_struct(cur_struct.handle,cur_struct.it);
		stack.pop_back();
	}

	void ReadEnums()
	{
		TreeRead::ObjectHandle root=r.get_root();
		int size=r.get_num_object(root);
		for(int iroot=size-1;iroot>=0;iroot--)
		{
			if(strcmp(r.get_sub_object_name(root,iroot),"!!!enums!!!")==0)
			{
				TreeRead::ObjectHandle henums=r.get_sub_object(root,iroot);

				xassert(henums.is_array());
				henums=r.get_ptr(henums);
				enums.resize(r.get_array_size(henums));
				for(TreeWrite::POINTER_TYPE ienum=0;ienum<enums.size();ienum++)
				{
					TreeRead::ObjectHandle helem_struct=r.get_array_elem(henums,ienum);
					xassert(helem_struct.is_struct());
					xassert(strcmp(r.get_sub_object_name(helem_struct,0),"name")==0);
					xassert(strcmp(r.get_sub_object_name(helem_struct,1),"!!!enums!!!array")==0);
						
					OneEnum& e=enums[ienum];

					r.get_string(r.get_sub_object(helem_struct,0),e.name);

					TreeRead::ObjectHandle helem_array=r.get_sub_object(helem_struct,1);
					xassert(helem_array.is_array());
					helem_array=r.get_ptr(helem_array);

					int size_elem_array=r.get_array_size(helem_array);
					for(int i=0;i<size_elem_array;i++)
					{
						int key;
						string name;
						TreeRead::ObjectHandle h=r.get_array_elem(helem_array,i);
						xassert(h.is_struct());
						xassert(r.get_num_object(h)==2);

						xassert(strcmp(r.get_sub_object_name(h,0),"name")==0);
						xassert(strcmp(r.get_sub_object_name(h,1),"value")==0);
						
						r.get_string(r.get_sub_object(h,0),name);
						key=r.get_int(r.get_sub_object(h,1));

						e.keyToName[key]=name;
					}
					
					enums_sorted[enums[ienum].name.c_str()]=&enums[ienum];
				}

				break;
			}
		}
	}

	bool EnumIsChanged(const EnumDescriptor& descriptor)
	{
		TypeEnumsSorted::iterator ite=enums_sorted.find(descriptor.typeName());
		if(ite==enums_sorted.end())
		{
			xassert(0 && "enum name changed");
			return false;
		}

		OneEnum& e=*ite->second;
		
		for(OneEnum::KeyToName::iterator it=e.keyToName.begin();it!=e.keyToName.end();++it)
		{
			if(descriptor.nameExists(it->second.c_str()))
			{
				int key=descriptor.keyByName(it->second.c_str());
				if(key!=it->first)
					return true;
			}else
				return true;
		}

		return false;
	}

	void ConvertEnum(int& value,const EnumDescriptor& descriptor)
	{
		TypeEnumsSorted::iterator ite=enums_sorted.find(descriptor.typeName());
		if(ite==enums_sorted.end())
		{
			xassert(0 && "enum changed");
			return;
		}

		OneEnum& e=*ite->second;

		OneEnum::KeyToName::iterator it=e.keyToName.find(value);
		if(it==e.keyToName.end())
		{
			xassert(0 && "key not found");
			return;
		}
		string& s=it->second;

		if(descriptor.nameExists(s.c_str()))
		{
			int key=descriptor.keyByName(s.c_str());
			value=key;
			return;
		}

		xassert(0);
	}

	void ConvertBitVector(int& value,const EnumDescriptor& descriptor)
	{

		TypeEnumsSorted::iterator ite=enums_sorted.find(descriptor.typeName());
		if(ite==enums_sorted.end())
			return;

		OneEnum& e=*ite->second;

		int result=0;
		for(OneEnum::KeyToName::iterator it=e.keyToName.begin();it!=e.keyToName.end();++it)
		{
			if((value&it->first)==it->first)
			{
				value &= ~it->first;
				string& s=it->second;
				if(descriptor.nameExists(s.c_str()))
				{
					int key=descriptor.keyByName(s.c_str());
					result|=key;
				}
			}
		}
		value=result;
	}

};
