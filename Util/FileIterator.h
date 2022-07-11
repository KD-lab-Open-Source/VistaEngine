#ifndef __FILE_ITERATOR_H_INCLUDED__
#define __FILE_ITERATOR_H_INCLUDED__

class FileIterator {
public:
    FileIterator (const char* path = 0) {
        if (path) {
            handle_ = FindFirstFile (path, &findFileData_);
        } else {
            handle_ = 0;
        }
    }

	~FileIterator () {
		if (handle_)
			FindClose (handle_);
	}

    inline FileIterator& operator++ () {
		xassert (handle_ && handle_ != INVALID_HANDLE_VALUE && "Incrementing bad FileIterator!");
		if (FindNextFile (handle_, &findFileData_) == false) {
			handle_ = 0;
		}
        return *this;
    }
    const FileIterator operator++(int) {
        FileIterator oldValue (*this);
        ++(*this);
        return oldValue;
    }
    inline const char* operator* () const {
		xassert (handle_ && handle_ != INVALID_HANDLE_VALUE && "Dereferencing bad FileIterator!");
        return findFileData_.cFileName;
    }
    inline bool isDirectory() const {
		return findFileData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    }
    inline bool isFile() const {
        return !(findFileData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    }
    inline bool operator== (const FileIterator& rhs) {
        return handle_ == rhs.handle_;
    }
    inline bool operator!= (const FileIterator& rhs) {
        return handle_ != rhs.handle_;
    }
private:
    WIN32_FIND_DATA findFileData_;
    HANDLE handle_;
};

#endif
