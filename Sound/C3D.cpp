#include "stdafx.h"
#include "C3D.h"
#include "SoundInternal.h"

#include <algorithm>



//SNDSoundParam::SNDSoundParam()
//{
//	is3d=false;
//	radius=100;
//	max_radius=DS3D_DEFAULTMAXDISTANCE;
//	max_num_sound=5;
//	volume=1.0f;
//	min_volume=0.0f;
//
//	delta_random=false;
//	delta_up=1.5f;
//	delta_down=0.5f;
//}

using namespace SND;
//
//SNDScript script3d(false);
//SNDScript script2d(true);
//
//
//SNDScript::SNDScript(bool _b2d)
//:b2d(_b2d)
//{
//	map_script_updated=false;
//}
//
//SNDScript::~SNDScript()
//{
//	RemoveAll();
//}
//
//
//ScriptParam* SNDScript::Find(LPCSTR name)
//{
//	if(!name)
//		return NULL;
//	if(!map_script_updated)
//		RebuildMapScript();
//	MapScript::iterator it=map_script.find(name);
//	if(it==map_script.end())
//	{
//		logs("Sound not found:%s\n",name);
//		return NULL;
//	}
//	return ((*it).second);
//}
//
//void SNDScript::RemoveAll()
//{
//	map_script.clear();
//
//	SoundArray::iterator it;
//	FOR_EACH(sounds,it)
//	{
//		it->Release();
//	}
//	sounds.clear();
//}
//
//
//void SNDScript::RebuildMapScript()
//{
//	map_script_updated=true;
//	map_script.clear();
//
//	SoundArray::iterator it;
//	FOR_EACH(sounds,it)
//	{
//		ScriptParam& sp=*it;
//		map_script[sp.sound_name.c_str()]=&sp;
//	}
//}
//
//bool SNDScript::AddSound(struct SNDSoundParam& snd)
//{
//	ScriptParam sp;
//	{
//		MTAuto lock(sp.GetLock());
//		sp.sound_name = snd.sound_name;
//		sp.radius = snd.radius;
//		sp.max_radius = (snd.max_radius >= 0) ? snd.max_radius : DS3D_DEFAULTMAXDISTANCE;
//
//		sp.max_num_sound = snd.max_num_sound;
//
//		sp.def_volume = snd.volume;
//		sp.min_volume = snd.min_volume;
//
//		sp.delta_random = snd.delta_random;
//		sp.delta_up = snd.delta_up;
//		sp.delta_down = snd.delta_down;
//
//		vector<string>::iterator it;
//		FOR_EACH(snd.sound_files,it)
//		{
//			sp.LoadSound(it->c_str(),b2d);
//		}
//
//		if(sp.GetSounds().empty())
//		{
//			return false;
//		}
//
//		sounds.push_back(sp);
//	}
//	return true;
//}
//
//void SNDScript::StopAll()
//{
//	SNDScript::MapScript::iterator it;
//	FOR_EACH(map_script,it)
//	{
//		ScriptParam* sp=(*it).second;
//		MTAuto lock(sp->GetLock());
//		
//		vector<SNDOneBuffer>::iterator itb;
//		FOR_EACH(sp->GetBuffer(),itb)
//		{
//			SNDOneBuffer& sb=*itb;
//			LPDIRECTSOUNDBUFFER buffer=sb.buffer;
//			DWORD status;
//			if(buffer->GetStatus(&status)==DS_OK)
//			{
//				if(status&DSBSTATUS_PLAYING)
//				{
//					buffer->Stop();
//					sb.pause_level=0;
//				}
//			}
//		}
//	}
//}
//
//void SNDScript::PauseAllPlayed(int pause_level)
//{
//	SNDScript::MapScript::iterator it;
//	FOR_EACH(map_script,it)
//	{
//		ScriptParam* sp=(*it).second;
//		MTAuto lock(sp->GetLock());
//		vector<SNDOneBuffer>::iterator itb;
//		FOR_EACH(sp->GetBuffer(),itb)
//		{
//			SNDOneBuffer& sb=*itb;
//			LPDIRECTSOUNDBUFFER buffer=sb.buffer;
//			DWORD status;
//			if(buffer->GetStatus(&status)==DS_OK)
//			{
//				if(status&DSBSTATUS_PLAYING)
//				{
//					buffer->Stop();
//					sb.pause_level=pause_level;
//				}
//
//				if(sb.p3DBuffer)
//				{
//					sb.p3DBuffer->Pause(true);
//					sb.pause_level=pause_level;
//				}
//			}
//		}
//	}
//}
//
//void SNDScript::PlayByLevel(int pause_level)
//{
//	SNDScript::MapScript::iterator it;
//	FOR_EACH(map_script,it)
//	{
//		ScriptParam* sp=(*it).second;
//		MTAuto lock(sp->GetLock());
//		vector<SNDOneBuffer>::iterator itb;
//		FOR_EACH(sp->GetBuffer(),itb)
//		{
//			SNDOneBuffer& sb=*itb;
//			LPDIRECTSOUNDBUFFER buffer=sb.buffer;
//			if(sb.pause_level==pause_level)
//			{
//				if(sb.p3DBuffer)
//					sb.p3DBuffer->Pause(false);
//				else
//					buffer->Play(0,0,sb.played_cycled?DSBPLAY_LOOPING:0);
//				sb.pause_level=0;
//			}
//		}
//	}
//}
//
/////////////SNDOneBuffer///////////////////////////////////
//SNDOneBuffer::SNDOneBuffer()
//{
//	script=NULL;
//	buffer=NULL;
//	nSamplesPerSec=0;
//	nAvgBytesPerSec=0;
//	p3DBuffer=NULL;
//	pSound=NULL;
//	used=false;
//	played_cycled=false;
//	pause_level=0;
//	begin_play_time=0;
//	pos.set(0,0,0);
//	velocity.set(0,0,0);
//	volume=0;
//}
//
//SNDOneBuffer::~SNDOneBuffer()
//{
//}
//
//bool SNDOneBuffer::SetFrequency(float frequency)
//{
//	float dw=script->delta_up-script->delta_down;
//	float mul=frequency*dw+script->delta_down;
//	frequency=round(nSamplesPerSec*mul);
//
//	if(p3DBuffer)
//		return p3DBuffer->SetFrequency(frequency);
//	else
//		return buffer->SetFrequency(frequency);
//}
//
//
//void ScriptParam::LoadSound(const char* name,bool b2d)
//{
//	LPDIRECTSOUNDBUFFER buf=NULL;
//	if(b2d || soft3d)
//	{
//		buf=SNDLoadSound(name,
//			DSBCAPS_STATIC|
//			DSBCAPS_GETCURRENTPOSITION2|
//			DSBCAPS_CTRLVOLUME|
//			DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLPAN
//			);
//	}else
//	{
//		buf=SNDLoadSound(name,
//			DSBCAPS_STATIC|
//			DSBCAPS_GETCURRENTPOSITION2|
//			DSBCAPS_CTRLVOLUME|
//			DSBCAPS_CTRLFREQUENCY|
//			DSBCAPS_CTRL3D |
//			DSBCAPS_MUTE3DATMAXDISTANCE
//			);
//	}
//
//	if(buf == NULL)
//		logs("Sound not loaded: %s\n",name);
//	else
//		GetSounds().push_back(buf);
//}
//
//void ScriptParam::Release()
//{
//	vector<SNDOneBuffer>::iterator its;
//	FOR_EACH(soundbuffer,its)
//	{
//		SNDOneBuffer& sb=*its;
//		SAFE_RELEASE(sb.buffer);
//
//		SAFE_DELETE(sb.p3DBuffer);
//	}
//	soundbuffer.clear();
//
//	vector<LPDIRECTSOUNDBUFFER>::iterator itb;
//	FOR_EACH(sounds,itb)
//	{
//		LPDIRECTSOUNDBUFFER p=*itb;
//		if(p)
//		{
//			int num=p->Release();
//			ASSERT(num==0);
//		}
//	}
//	sounds.clear();
//}
//
//struct SNDOneBufferDistance
//{
//	SNDOneBuffer* p;
//	float distance;
//
//	inline bool operator()(const SNDOneBufferDistance& s1,const SNDOneBufferDistance& s2)const
//	{
//		return s1.distance<s2.distance;
//	}
//};
//
//void ScriptParam::RecalculateClipDistance()
//{
//	vector<SNDOneBufferDistance> real_played;
//	vector<SNDOneBuffer>::iterator its;
//	if(max_num_sound<1)
//		return;
//
//	if(soundbuffer.size()<=max_num_sound)
//		return;
//
//	SNDOneBufferDistance s;
//	FOR_EACH(soundbuffer,its)
//	{
//		SNDOneBuffer& p=*its;
//		if(!p.used)continue;
//		if(p.p3DBuffer==NULL)continue;
//
//		if(!p.p3DBuffer->IsPlaying())
//			continue;
//		s.p=&p;
//		s.distance=p.p3DBuffer->VectorToListener().norm();
//		real_played.push_back(s);
//	}
//
//	int sz=real_played.size();
//	if(sz<=max_num_sound)
//		return;
//
//	sort(real_played.begin(),real_played.end(),s);
//
//	int i;
//#ifdef _DEBUG
//	for(i=0;i<sz-1;i++)
//	{
//		float d1=real_played[i].distance;
//		float d2=real_played[i+1].distance;
//		ASSERT(d1<=d2);
//	}
//#endif _DEBUG
///*
//	float clip_dist=(real_played[max_num_sound-1].distance+
//					 real_played[max_num_sound].distance)*0.5f;
//
//	FOR_EACH(soundbuffer,its)
//	if(its->p3DBuffer)
//		its->p3DBuffer->SetClipDistance(clip_dist);
///*/
//	for(i=0;i<real_played.size();i++)
//	{
//		if(i<max_num_sound)
//			real_played[i].p->p3DBuffer->SetMute(false);
//		else
//			real_played[i].p->p3DBuffer->SetMute(true);
//	}
///**/
//
//}
//
//bool SNDAddSound(SNDSoundParam& param)
//{
//	if(param.is3d)
//		return script3d.AddSound(param);
//	else
//		return script2d.AddSound(param);
//}
