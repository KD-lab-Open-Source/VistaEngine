#include "stdafx.h"
#include "ShowLog.h"
#include <time.h>

#include "kdw\ObjectsTree.h"
#include "kdw\HLine.h"
#include "kdw\Label.h"

ShowLog::ShowLog()
{
	add(tree_ = new kdw::ObjectsTree, true, true, true);
	add(new kdw::HLine(), true, false, false);
	add(status_ = new kdw::Label(" ", false, 0), true, false, false);

}

ShowLog::~ShowLog()
{
}

void ShowLog::addRecord(const char* txt)
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

void ShowLog::setStatus(const char* txt)
{
	if(strcmp(status_->text(), txt) != 0)
		status_->setText(txt);
}

void ShowLog::clearLog()
{
	tree_->root()->clear();
	tree_->update();
}