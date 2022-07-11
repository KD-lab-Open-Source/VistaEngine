#include "stdafx.h"
#include "ShowLog.h"
#include <time.h>

#include "kdw\ObjectsTree.h"

ClientLog::ClientLog()
{
	add(tree_ = new kdw::ObjectsTree, true, true, true);
}

ClientLog::~ClientLog()
{
}

void ClientLog::addRecord(const char* txt)
{
	time_t time = ::time(NULL);
	time += 2*3600; // наш часовой пояс

	XBuffer buf;
	if(struct tm* dt = gmtime(&time)){
		if(dt->tm_mday < 10)
			buf < "0";
		buf <= dt->tm_mday;

		if(dt->tm_mon < 9)
			buf < ".0";
		else
			buf < ".";
		buf <= dt->tm_mon;

		if(dt->tm_hour < 10)
			buf < " 0";
		else
			buf < " ";
		buf <= dt->tm_hour;

		if(dt->tm_min < 10)
			buf < ":0";
		else
			buf < ":";
		buf <= dt->tm_min;

		if(dt->tm_sec < 10)
			buf < ":0";
		else
			buf < ":";
		buf <= dt->tm_sec;
	}

	buf < " > " < txt;

	tree_->root()->add(new kdw::TreeObject(buf));
	tree_->update();
}
