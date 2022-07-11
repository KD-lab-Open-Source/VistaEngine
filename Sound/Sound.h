#ifndef __SOUND_H_INCLUDED__
#define __SOUND_H_INCLUDED__
#include <string>

class Channel;
//Инициализация/деинициализация библиотеки
bool SNDInitSound(HWND g_hWnd,bool bEnable3d,bool soft3d);
void SNDReleaseSound();
void* SNDGetDirectSound();//Возвращает указатель на LPDIRECTSOUND8

void SNDEnableSound(bool enable);
bool SNDIsSoundEnabled();

//Работа с ошибками
bool SNDEnableErrorLog(LPCSTR file);

void SNDSetVolume(float volume);//volume=0..1
void SNDSetFade(bool fadeIn,int time=0);
void SNDStopAll();
float SNDGetVolume();
void SNDSetGameActive(bool active);

////////////////////////////3D/////////////////////////////////
class SND3DListener
{
protected:
	friend struct SNDOneBuffer;
	friend class VirtualSound3D;
	friend class SoftSound3D;
	friend class HardSound3D;
	friend class SND3DSound;

	Mat3f rotate,invrotate;
	Vect3f position;
	//MatXf mat;
	Vect3f velocity;

	//Дупликаты для software
	float s_distance_factor;
	float s_doppler_factor;
	float s_rolloff_factor;

	Vect3f front,top,right;

	float zmultiple;
public:
	SND3DListener();
	~SND3DListener();

	//Параметры изменяемые редко (скорее всего их менять и устанавливать не придётся никогда)
	//К тому-же они не работают (уж не знаю по какой причине)
	bool SetDistanceFactor(float);//1 - в метрах, 1000 - в километрах
	bool SetDopplerFactor(float);//0..10, по умолчанию 1
	bool SetRolloffFactor(float);

	//SetPos надо изменять каждый кадр
	bool SetPos(const MatXf& mat);

	Vect3f GetPos(){return position;};

	//SetVelocity - желательно изменять каждый кадр
	//иначе не будет смысла в SetDopplerFactor,SetRolloffFactor
	bool SetVelocity(const Vect3f& velocity);

	//Функция специально для Рубера
	//Что-бы расстояние по Z было меньше.
	//в реальном времени криво работает\
	//zmul=0..1
	void SetZMultiple(float zmul){zmultiple=zmul;};
	float GetZMultiple(){return zmultiple;};

	//Update - Вызывать после установки параметров (SetPos,...)
	//(один раз на кадр!)
	bool Update();
};

extern SND3DListener snd_listener;
#endif
