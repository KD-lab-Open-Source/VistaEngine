#ifndef __SOFTWARE_BUFFER_H_INCLUDED__
#define __SOFTWARE_BUFFER_H_INCLUDED__

class VirtualSound3D
{
protected:
	LPDIRECTSOUNDBUFFER pSound;
	DWORD BytePerSample;
	DWORD nSamplesPerSec,//frequency==nSamplesPerSec
		nAvgBytesPerSec,dwBufferBytes;

	double begin_play_sound,last_start_stop;
	DWORD RealFrequency;

	DWORD GetCurPos(double curtime);

	Vect3f position;
	Vect3f velocity;
	bool is_playing,is_cycled;
	bool is_muted;
	bool is_paused;

	void Mute(bool mute);
public:
	VirtualSound3D();
	virtual ~VirtualSound3D(){};
	
	virtual bool Init(LPDIRECTSOUNDBUFFER ptr);

	virtual float GetMaxDistance()=0;
	virtual float GetMinDistance()=0;
	virtual bool SetMaxDistance(float)=0;
	virtual bool SetMinDistance(float)=0;
	virtual Vect3f GetPosition()=0;
	virtual bool SetPosition(const Vect3f& pos)=0;
	virtual Vect3f GetVelocity()=0;
	virtual bool SetVelocity(const Vect3f& vel)=0;

	virtual bool IsPlaying();
	virtual bool Play(bool cycled);
	virtual bool Stop();
	virtual bool SetFrequency(DWORD dwFrequency);

	virtual bool SetVolume(float vol)=0;//vol=0..1

	virtual void RecalculatePos()=0;
	virtual void RecalculateVolume()=0;

	virtual Vect3f VectorToListener();

	virtual void SetMute(bool is_muted_){is_muted=is_muted_;};

	virtual void Pause(bool p);
};

class SoftSound3D:public VirtualSound3D
{
	float min_distance,max_distance;

	float volume;
	float set_volume;

public:
	SoftSound3D();
	~SoftSound3D();

	bool Init(LPDIRECTSOUNDBUFFER ptr);

	float GetMaxDistance();
	float GetMinDistance();
	bool SetMaxDistance(float);
	bool SetMinDistance(float);
	Vect3f GetPosition();
	bool SetPosition(const Vect3f& pos);
	Vect3f GetVelocity();
	bool SetVelocity(const Vect3f& vel);

	bool SetVolume(float vol);

	void RecalculatePos();
	void RecalculateVolume();
};

class HardSound3D:public VirtualSound3D
{
	IDirectSound3DBuffer8* pBuffer3d;
public:
	HardSound3D();
	virtual ~HardSound3D();
	
	virtual bool Init(LPDIRECTSOUNDBUFFER ptr);

	virtual float GetMaxDistance();
	virtual float GetMinDistance();
	virtual bool SetMaxDistance(float);
	virtual bool SetMinDistance(float);
	virtual Vect3f GetPosition();
	virtual bool SetPosition(const Vect3f& pos);
	virtual Vect3f GetVelocity();
	virtual bool SetVelocity(const Vect3f& vel);

	virtual bool SetVolume(float vol);//vol=0..1

	virtual void RecalculatePos();
	virtual void RecalculateVolume();
};

#endif
