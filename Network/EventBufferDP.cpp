
#include "StdAfx.h"

#include "ConnectionDP.h"
#include "EventBufferDP.h"

#include "P2P_interface.h"

#ifndef _FINAL_VERSION_
#define xassertStr2(exp, str) { string s = #exp; s += "\n"; s += str; xxassert(exp,s.c_str()); }
#else
#define xassertStr2(exp, str) 
#endif


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//			Universal buffer
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

InOutNetComBuffer::InOutNetComBuffer(unsigned int size, bool autoRealloc)
: XBuffer(size, autoRealloc) 
{
	reset();
	byte_sending=0;
	byte_receive=0;
	event_ID = NETCOM_ID_NONE;
}


void InOutNetComBuffer::reset()
{
	init();
	next_event_pointer = 0;
	filled_size = 0;
	set(0);
	event_ID = NETCOM_ID_NONE;
}


//Out
int InOutNetComBuffer::send(PNetCenter& conn, const UNetID& unid, bool flag_guaranted)
{
///	“ест 
///	unsigned int size=filled_size;
///	unsigned int msize=size;
///	while(msize>0){
///		int sizeEvent=*((unsigned long*)(&buf[size-msize])) + sizeof(size_of_event);
///		xassert(sizeEvent<=msize);
///		msize-=sizeEvent;
///	}
///	xassert(msize==0);

	unsigned int sent=0;
	while(sent < filled_size){ // подразумеваетс€ ==
		sent+=conn.Send(buffer()+sent, filled_size-sent, unid, flag_guaranted);
	};
	xassert(filled_size==sent);
	reset();
	byte_sending+=sent;
	return sent;
}

//Out
void InOutNetComBuffer::putNetCommand(const NetCommandBase* event)
{
	clearBufferOfTheProcessedCommands();
	set(filled_size);
	unsigned int pointer_to_size_of_event;
	//unsigned int size_of_event;
	pointer_to_size_of_event = tell();
	size_of_event=0;
	event_ID=event->EventID;
	write(&size_of_event, sizeof(size_of_event));
	write(&event_ID, sizeof(event_ID));

	event->Write(*this);

	int off = tell();
	set(pointer_to_size_of_event);
	size_of_event=off-pointer_to_size_of_event - sizeof(size_of_event);
	write(&size_of_event, sizeof(size_of_event));
	set(off);
	filled_size=off;
	set(0);
	//дл€ нормального next event
	event_ID = NETCOM_ID_NONE;
	size_of_event=0;
}

//in
void InOutNetComBuffer::clearBufferOfTheProcessedCommands(void)
{
	if(next_event_pointer){
		if(filled_size != next_event_pointer)
			memmove(buffer(),buffer() + next_event_pointer, filled_size - next_event_pointer);
		filled_size -= next_event_pointer;
		set(next_event_pointer = 0);
	}
}

bool InOutNetComBuffer::putBufferPacket(unsigned char* buf, unsigned int sz)
{
	clearBufferOfTheProcessedCommands();
	if(size()-filled_size < sz) {
		xassert("Net input buffer is small.");
		return 0;
	}
	memcpy(buffer() + filled_size, buf, sz);
	byte_receive+=sz;
	filled_size +=sz;
	return 1;
}

int InOutNetComBuffer::currentNetCommandID()
{
	if(event_ID == NETCOM_ID_NONE){
		clearBufferOfTheProcessedCommands();
		return nextNetCommand();
	}
	return event_ID;
} 

NCEventID InOutNetComBuffer::nextNetCommand()
{
	if(event_ID != NETCOM_ID_NONE){
		if(next_event_pointer /*+ sizeof(size_of_event)*/ > filled_size){
			xassert(0&&"dividual packet ?!");
			reset();
			return NETCOM_ID_NONE;
		}
		if(next_event_pointer != tell()){
			//ErrH.Abort("Incorrect events reading");
			XBuffer buf;
			buf < "Incorrect events reading" < " eventID=" <= event_ID < " offset=" <= tell() < " nextEventPoint=" <= next_event_pointer;
			xassertStr2(0, buf);
			reset();
			return NETCOM_ID_NONE;
		}
	}

	event_ID = NETCOM_ID_NONE;

	if(filled_size-tell() > (sizeof(size_of_event) + sizeof(event_ID)) ) {
		read(&size_of_event, sizeof(size_of_event)); //get_short();
		unsigned int new_pointer = next_event_pointer + size_of_event + sizeof(size_of_event);
		if(new_pointer > filled_size){
			xassert(0&&"dividual packet ?!");
			set(next_event_pointer);
			reset();
			return NETCOM_ID_NONE;
		}

		next_event_pointer = new_pointer;
		
		read(&event_ID, sizeof(event_ID));
	}
	return event_ID;
}

void InOutNetComBuffer::ignoreNetCommand()
{
	event_ID = NETCOM_ID_NONE;
	set(next_event_pointer);
}

void InOutNetComBuffer::backNetCommand()
{
	const unsigned int SIZE_HEAD_PACKET=sizeof(event_ID)+sizeof(size_of_event);
	if(tell()>=SIZE_HEAD_PACKET){
		event_ID = NETCOM_ID_NONE;
		*this -= SIZE_HEAD_PACKET;
		next_event_pointer = tell();
	}
	else {
		xassert(0 && "Invalid back event");
	}
}

unsigned long InOutNetComBuffer::getQuantAmount()
{
	unsigned long cntQuant = 0;
	unsigned long i = tell();
	unsigned long sizeCurEvent;
	NCEventID curID;
	if(event_ID != NETCOM_ID_NONE){
		if(event_ID==NETCOM_ID_NEXT_QUANT) cntQuant++;
		i=next_event_pointer;
	}
	while(i<filled_size){
		sizeCurEvent=*(unsigned long*)(&buffer()[i]);
		i+=sizeof(unsigned long);
		curID=*(NCEventID*)(&buffer()[i]);
		if(curID==NETCOM_ID_NEXT_QUANT) cntQuant++;
		i+=sizeCurEvent;

	}
	return cntQuant;
}
