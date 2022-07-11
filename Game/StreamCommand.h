#ifndef __STREAM_COMMAND_H_INCLUDED__
#define __STREAM_COMMAND_H_INCLUDED__

/*
Поток данных.
Для каждой порции данных есть своя управляющая функция.
После execute() поток очищается.
*/

typedef void (*StreamDataFunction)(void* data);

class StreamDataManager
{
	XBuffer stream_;
	MTSection lockSection_;

	typedef unsigned short int DataSize;
	unsigned int sizePosition_;

	void calcSize(){
		if(sizePosition_){
			*((DataSize*)&stream_[(int)sizePosition_]) = DataSize(stream_.tell() - sizePosition_ - sizeof(DataSize));
			sizePosition_ = 0;
		}
	}
	
	bool logic2graph_;

public:
	StreamDataManager(bool logic2graph) : stream_(1024, 1), sizePosition_(0), logic2graph_(logic2graph) {}
	~StreamDataManager() { dassert(stream_.tell() == 0); }
	void execute();
	void clear(){ stream_.set(0); sizePosition_ = 0; }

	void lock(){ lockSection_.lock(); }
	void unlock(){ lockSection_.unlock(); }

	StreamDataManager& set(StreamDataFunction func);
	StreamDataManager& operator<<(const XBuffer& xb);
	StreamDataManager& operator<<(StreamDataManager& s);
	
	template<class T>
	StreamDataManager& operator<<(const T& v){
		//stream_.write(&v, sizeof(v));
		stream_.write(v);
		return *this;
	}
};

// поток для команд интерфейса, можно писать в любом месте логики
extern StreamDataManager uiStreamCommand;
// поток для передачи данных в логику из интерфейсных событий графики
// перед использованием долго думать о детерминизме
extern StreamDataManager uiStreamGraph2LogicCommand;

#endif //__STREAM_COMMAND_H_INCLUDED__
