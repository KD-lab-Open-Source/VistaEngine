#ifndef __FILE_UTILS_H_INCLUDED__
#define __FILE_UTILS_H_INCLUDED__

#include <string>

inline std::string extractFileBase(const char* pathName)
{
    std::vector<char> nameBuf(_MAX_FNAME);

    _splitpath (pathName, 0, 0, &nameBuf[0], 0);
    std::string fileName (&nameBuf [0]);
    return fileName;
}

inline std::string extractFileExt(const char* pathName)
{
    std::vector<char> extBuf(_MAX_EXT);

    _splitpath (pathName, 0, 0, 0, &extBuf[0]);
    std::string fileExt (&extBuf [0]);
    return fileExt;
}

inline std::string extractFileName(const char* pathName)
{
    std::vector<char> nameBuf(_MAX_FNAME);
    std::vector<char> extBuf(_MAX_EXT);

    _splitpath (pathName, 0, 0, &nameBuf[0], &extBuf[0]);
    std::string fileName (&nameBuf [0]);
    std::string fileExt (&extBuf [0]);
    return (fileName + fileExt);
}


inline bool compareFileName(const char* lhs, const char* rhs)
{
	char buffer1[_MAX_PATH];
	char buffer2[_MAX_PATH];
	if(_fullpath(buffer1, lhs, _MAX_PATH) == 0)
		return stricmp(lhs, rhs) == 0;
	if(_fullpath(buffer2, rhs, _MAX_PATH) == 0)
		return stricmp(lhs, rhs) == 0;
	return stricmp(buffer1, buffer2) == 0;
}

class DirIterator{
public:
    DirIterator(const char* path = 0){
        if(path)
            handle_ = FindFirstFile(path, &findFileData_);
        else
            handle_ = 0;
    }

	~DirIterator(){
		if(handle_)
			FindClose (handle_);
	}

    DirIterator& operator++(){
		xassert(handle_ && handle_ != INVALID_HANDLE_VALUE && "Incrementing bad DirIterator!");
		if (FindNextFile (handle_, &findFileData_) == false) {
			handle_ = 0;
		}
        return *this;
    }
    const DirIterator operator++(int){
        DirIterator oldValue (*this);
        ++(*this);
        return oldValue;
    }
    const char* c_str() const{
		xassert (handle_ && handle_ != INVALID_HANDLE_VALUE && "Dereferencing bad DirIterator!");
        return findFileData_.cFileName;
    }
    const char* operator* () const{ return c_str(); }
    bool isDirectory() const{
		return findFileData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    }
    bool isFile() const{
        return !(findFileData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }
    bool operator==(const DirIterator& rhs){
        return handle_ == rhs.handle_;
    }
    bool operator!=(const DirIterator& rhs){
        return handle_ != rhs.handle_;
    }
private:
    WIN32_FIND_DATA findFileData_;
    HANDLE handle_;
};

#endif
