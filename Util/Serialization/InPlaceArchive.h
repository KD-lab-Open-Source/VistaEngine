#ifndef __IN_PLACE_ARCHIVE_H__
#define __IN_PLACE_ARCHIVE_H__

/*
	InPlace Archive

	������� ������:
	������ ���������� � ��������� �������� ������, ����� ������������ ��� �� ����, ����� ��� ���������� 
	�������� (��������� � ������� ����) ������ ����������� �� �������������� �� ��������� �������. ��������� ����,
	vector<> � ����������� ��������� ���������� ����� ������.

	��� �������� ������������ ��������� �� ������� ���������.

	����������� �������� fixup ���������� ����������� ������ ��� ����������� �������: ����������, ���� ����� ������
	�������������� ������ (��� � ������ StringTable), �� ��� ���� ����������� "����� �����" ������ ������.

	��������� ���������� �������������� ���� � serialize():
	
	if(ar.isInput()) - ���������������� ��� �������� ��� �� ����� ������ ��������
	
	if(ar.isOutput()) - ������ �������� ���������� ��� ������, �.�. ��� ����� �������� ���������� ������.
	��� ����������� ����� ������� ������� � ������ �����.
	
	if(!ar.inPlace()) - ��������� ��������� �������, ������� ����� � ������ ������� (������� ��������� ��� ��������)
	
	if(ar.inPlace()) - ������������� non-POD ������, ������� �� ������������� �������� ��������

	� ������� ��� ������ �������� ������� start == end == end_of_storage ����� ��������� � ������������ �������.

	
*/

#include <vector>
#include "Handle.h"
#include "Serialization/Serialization.h"

class InPlaceOArchive : public Archive
{
public:
	InPlaceOArchive(const char* fname, bool fixVtable);
	~InPlaceOArchive();

	void open(const char* fname); 
	bool close();  // true if there were changes, so file was updated

	bool isInput() const { return false; }
	bool isOutput() const { return false; }

protected:
	bool processBinary(MemoryBlock& buffer, const char* name, const char* nameAlt);

	bool processEnum(int& value, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processBitVector(int& flags, const EnumDescriptor& descriptor, const char* name, const char* nameAlt) { writeValue(flags); return true; }

	bool processValue(char& value, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processValue(signed char& value, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processValue(signed short& value, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processValue(signed int& value, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processValue(signed long& value, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processValue(unsigned char& value, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processValue(unsigned short& value, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processValue(unsigned int& value, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processValue(unsigned long& value, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processValue(float& value, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processValue(double& value, const char* name, const char* nameAlt) { writeValue(value); return true; }
	bool processValue(bool& value, const char* name, const char* nameAlt) { writeValue(value); return true; }

	bool processValue(ComboListString& t, const char* name, const char* nameAlt);
	bool processValue(string& str, const char* name, const char* nameAlt);

	bool openStructInternal(void* object, int size, const char* name, const char* nameAlt, const char* typeName, bool polymorphic);
	void closeStruct(const char* name);

	bool openContainer(void* array, int& number, const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, int elementSize, bool readOnly);
	void closeContainer(const char* name);

	int openPointer(void*& object, const char* name, const char* nameAlt, const char* baseName, const char* typeName, const char* typeNameAlt);
	void closePointer(const char* name, const char* baseName, const char* derivedName);


private:
	class Saver
	{
	public:
		Saver(int initial_size=124)
		{
			buffer_ = (char*)::malloc(initial_size);
			assert(buffer_);
			position_ = buffer_;
			allocated_size_ = initial_size;
		}

		~Saver()
		{
			::free(buffer_);
		}

		char* buffer() { return buffer_; };
		int size() { return position_ - buffer_; }

		void set(int offset) { position_ = buffer_ + offset; }

		void write(const char* data, size_t size)
		{
			if(buffer_ + allocated_size_ < position_ + size)
				reallocate(max(allocated_size_ + 1024*(size/1024 + 2), allocated_size_*2));
			memcpy(position_, data, size);
			position_ += size;
		}

		template<class T>
		void write(const T& x){ write((const char*)&x, sizeof(x)); }

		void write(const char* str) { write(str, strlen(str) + 1); }
		void write(const string& str) { write(str.c_str(), (int)str.size() + 1); }

	private:
		void reallocate(size_t new_size)
		{
			size_t offset = position_ - buffer_;
			buffer_ = (char*)realloc(buffer_, new_size);
			assert(buffer_);
			position_ = buffer_ + offset;
			allocated_size_ = new_size;
		}

		char* buffer_;
		char* position_;
		size_t allocated_size_;
	};

	class Node
	{
	public:
		Node(const char* address, int size, int offset)
			: address_(address), size_(size), offset_(offset) {}

		// [address, size] ����� ������ this
		bool in(const char* address, int size) const { return address_ <= address && address_ + size_ >= address + size; }
		// [address, size] ����� ��������� ��� this
		bool out(const char* address, int size) const { return address_ + size_ <= address || address_ >= address + size; }

		int offset(const char* address) const { 
			xassert(in(address, 1)); 
			return address - address_ + offset_; 
		}

	private:
		const char* address_;
		int size_;
		int offset_;
	};

	typedef vector<Node> Stack;
	Stack stack_;

	bool beginBlock_;
	Saver saver_;
	Saver fixUpSaver_;
	bool fixVTable_;
	int fixVTableSize_;
	Saver fixVTableSaver_;
	string fileName;

	void checkIn(const char* object, int size){ xassert("������� �������� � InPlaceOArchive �������, �� ������������� ���������" && stack_.back().in(object, size)); }

	void writeString(const string& str);
	template<class T> void writeValue(const T& value) { checkIn((char*)&value, sizeof(value)); }

	const Node& back() const;
};

// Customization point for InPlaceIArchive::construct().
//
// The saved image is a raw *32-bit* memory dump: its offsets are 32-bit, and its struct
// strides and padding are 32-bit. The original engine was a 32-bit process, so it could
// relocate the image in place and cast the result straight to a live object. We are 64-bit
// everywhere, and that trick cannot work here -- writing a 64-bit base into a 32-bit slot
// truncates it.
//
// So each concrete type provides a reader `inPlaceReconstruct(T*, image, size)` (found by
// ordinary lookup / ADL where T is complete) that reads the 32-bit offsets by hand and
// rebuilds a real native object. This primary template is the fallback for types that have
// no reader; it asserts rather than hand back a bogus object.
//
// The only type that actually reaches it today is LibraryWrapperBase, and it never runs:
// LIBRARY_IN_PLACE is 0 (LibraryWrapper.h), so inPlaceEnabled() is always false and the
// libraries take the XPrm path. Models (cStatic3dx / LodsCache) have real readers, in
// Render/3dx/Static3DX.cpp.
template<class T>
T* inPlaceReconstruct(T*, const char* /*image*/, int /*size*/)
{
	xassert(!"InPlaceIArchive::construct: no in-place reader for this type");
	return 0;
}

class InPlaceIArchive
{
public:
	InPlaceIArchive(const char* name);

	bool open(const char* fname);  // true if file exists

	// Reconstruction copies everything into native objects, so this archive owns the raw
	// image and frees it on destruction.
	~InPlaceIArchive() { delete[] data_; }

	template<class T>
	void construct(T*& ptr) { ptr = inPlaceReconstruct((T*)0, data_, size_); }

	// Native objects: run the real destructor. (The 32-bit engine could not: its object
	// *was* the image blob, and its members aliased it.)
	template<class T>
	static void destruct(T* ptr) { delete ptr; }

private:
	int version_;
	int size_;
	union{
		char* data_;
		int* dataInt_;
	};
};

#endif //__IN_PLACE_ARCHIVE_H__
