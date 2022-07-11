#include "stdafx.h"
#include "sound.h"
#include "wavread.h"
//#include "simpleplayer.h"

static char sound_directory[]="wav\\eff\\";
extern HWND g_hWnd;//Окно приложения
static float global_volume=1.0f;
static bool sound_enable=true;

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

LPDIRECTSOUND       g_pDS       = NULL;
static DWORD dwCreatationFlags=0;

LPDIRECTSOUND  GetDirectSound(){return g_pDS;}

struct SoundBuffers
{
	string name;
	parray<CSound> sound;
};

parray<SoundBuffers> sound_buffer(32,true);

CSound* FindSound(LPCSTR name,bool one,bool* playone)
{
	ASSERT(g_pDS);
	if(playone)
		*playone=false;

	for(int i=0;i<sound_buffer.GetSize();i++)
	{
		if(strcmp(sound_buffer[i]->name,name)==0)
		{
			parray<CSound>& sound=sound_buffer[i]->sound;
			ASSERT(sound.GetSize()>0);
			//Удалить проигранные
			for(int j=0;j<sound.GetSize();j++)
			{
				if(sound.GetSize()==1)
					break;

				CSound* s=sound[j];
				if(!s->IsPlaying())
				{
					SAFE_RELEASE(s->buffer);
					delete s;
					sound.delAt(j--);
				}else
				if(playone)
					*playone=true;
			}

			if( one && *playone)
				return NULL;

			//Добавить
			CSound* s=new CSound;
			VERIFY(g_pDS->DuplicateSoundBuffer(sound[0]->buffer,&s->buffer)==DS_OK);
			sound.add(s);
			return s;
		}
	}

	return NULL;
}

HRESULT InitDirectSound()
{
    HRESULT             hr;
    LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

    // Create IDirectSound using the primary sound device
    if( FAILED( hr = DirectSoundCreate( NULL, &g_pDS, NULL ) ) )
        return hr;

    // Set coop level to DSSCL_PRIORITY 
    if( FAILED( hr = g_pDS->SetCooperativeLevel( g_hWnd, DSSCL_PRIORITY  ) ) )
        return hr;
    
    // Get the primary buffer 
    DSBUFFERDESC        dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat   = NULL;
       
    if( FAILED( hr = g_pDS->CreateSoundBuffer( &dsbd, &pDSBPrimary, NULL ) ) )
        return hr;

    // Set primary buffer format to 22kHz and 16-bit output.
    WAVEFORMATEX wfx;
    ZeroMemory( &wfx, sizeof(WAVEFORMATEX) ); 
    wfx.wFormatTag      = WAVE_FORMAT_PCM; 
    wfx.nChannels       = 2; 
    wfx.nSamplesPerSec  = 44100;//22050; 
    wfx.wBitsPerSample  = 16; 
    wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if( FAILED( hr = pDSBPrimary->SetFormat(&wfx) ) )
        return hr;

    SAFE_RELEASE( pDSBPrimary );

	MpegInitLibrary(g_pDS);
    return S_OK;
}

void ClearAutoSound()
{
	for(int i=0;i<sound_buffer.GetSize();i++)
	{
		parray<CSound>& sound=sound_buffer[i]->sound;
		for(int j=0;j<sound.GetSize();j++)
			delete sound[j];
	}
	sound_buffer.removeAll();
}

void ReleaseDirectSound()
{
	MpegStop();
	MpegDeinitLibrary();

	ClearAutoSound();
    SAFE_RELEASE( g_pDS ); 
}

HRESULT RestoreBuffer(LPDIRECTSOUNDBUFFER g_pDSBuffer)
{
    HRESULT hr;

    if( NULL == g_pDSBuffer )
        return S_OK;

    DWORD dwStatus;
    if( FAILED( hr = g_pDSBuffer->GetStatus( &dwStatus ) ) )
        return hr;

    if( dwStatus & DSBSTATUS_BUFFERLOST )
    {
        // Since the app could have just been activated, then
        // DirectSound may not be giving us control yet, so 
        // the restoring the buffer may fail.  
        // If it does, sleep until DirectSound gives us control.
        do 
        {
            hr = g_pDSBuffer->Restore();
            if( hr == DSERR_BUFFERLOST )
                Sleep( 10 );
        }
        while( hr = g_pDSBuffer->Restore() );
    }

    return S_OK;
}


CSound::CSound()
:buffer(NULL)
{
}

CSound::~CSound()
{
	SAFE_RELEASE(buffer);
}

bool CSound::IsPlaying()
{
	if(buffer==NULL)
		return false;

	DWORD status;
	if(buffer->GetStatus(&status)!=DS_OK)
		return false;

	return (status&DSBSTATUS_PLAYING)?true:false;
}

bool DDCache(LPCSTR fname)
{
	for(int i=0;i<sound_buffer.GetSize();i++)
	{
		if(strcmp(sound_buffer[i]->name,fname)==0)
		{
			return true;
		}
	}

	CSound* sound=NULL;
	sound=DDLoadSound(fname);
	if(sound==NULL)return false;

	SoundBuffers* sb=new SoundBuffers;
	sb->name=fname;
	sb->sound.add(sound);
	sound_buffer.add(sb);
	return true;
}

bool DDPlaySound(LPCSTR fname,long vol,bool one,long pan)
{
	if(g_pDS==NULL || !sound_enable)return false;

	bool playone=false;
	CSound* sound=NULL;
	sound=FindSound(fname,one,&playone);
	if(sound==NULL && one && playone)
		return true;

	if(sound==NULL)
	{
		sound=DDLoadSound(fname);
		if(sound==NULL)return false;

		SoundBuffers* sb=new SoundBuffers;
		sb->name=fname;
		sb->sound.add(sound);
		sound_buffer.add(sb);
	}

	sound->Play();
	sound->SetVolume(vol);
	sound->SetPan(pan);

	return true;
}

CSound* DDLoadSound(LPCSTR fxname)
{
	HRESULT hr;
	if(g_pDS==NULL  || !sound_enable)
		return NULL;

	char fname[256];
	sprintf(fname,"%s%s.wav",sound_directory,fxname);

	CSound* sound=new CSound;

    CWaveSoundRead* g_pWaveSoundRead = new CWaveSoundRead();

    // Load the wave file
    ERRORM( SUCCEEDED( g_pWaveSoundRead->Open( fname ) ),
			"Cannot load sound %s",fname );

	//CreateStaticBuffer 
    // Reset the wave file to the beginning 
    g_pWaveSoundRead->Reset();

    // The size of wave data is in pWaveFileSound->m_ckIn
    INT nWaveFileSize = g_pWaveSoundRead->m_ckIn.cksize;

    // Allocate that buffer.
    BYTE* pbWavData = new BYTE[ nWaveFileSize ];
	UINT  cbWavSize;

    ERRORM( SUCCEEDED( hr = g_pWaveSoundRead->Read( nWaveFileSize, 
                                             pbWavData, 
                                             &cbWavSize ) ) ,
			"Cannot load sound %s",fname);

    // Set up the direct sound buffer, and only request the flags needed
    // since each requires some overhead and limits if the buffer can 
    // be hardware accelerated
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_CTRLPAN | //DSBCAPS_CTRLFREQUENCY |
						 DSBCAPS_CTRLVOLUME | DSBCAPS_STATIC;
    dsbd.dwBufferBytes = cbWavSize;
    dsbd.lpwfxFormat   = g_pWaveSoundRead->m_pwfx;

    // Add in the passed in creation flags 
    dsbd.dwFlags       |= dwCreatationFlags;

    // Create the static DirectSound buffer using the focus set by the UI
    if( FAILED( hr = g_pDS->CreateSoundBuffer( &dsbd, &sound->buffer, NULL ) ) )
	{
		delete sound;
        return NULL;
	}

    VOID* pbData  = NULL;
    VOID* pbData2 = NULL;
    DWORD dwLength;
    DWORD dwLength2;

    // Make sure we have focus, and we didn't just switch in from
    // an app which had a DirectSound device
    if( FAILED( hr = RestoreBuffer(sound->buffer) ) )
	{
		delete sound;
        return NULL;
	}

    // Lock the buffer down
    if( FAILED( hr = sound->buffer->Lock( 0, dsbd.dwBufferBytes, &pbData, &dwLength, 
                                        &pbData2, &dwLength2, 0L ) ) )
	{
		delete sound;
        return NULL;
	}

    // Copy the memory to it.
    memcpy( pbData, pbWavData, dsbd.dwBufferBytes );

    // Unlock the buffer, we don't need it anymore.
    sound->buffer->Unlock( pbData, dsbd.dwBufferBytes, NULL, 0 );
    pbData = NULL;

    // We dont need the wav file data buffer anymore, so delete it 
    SAFE_DELETE( pbWavData );


	delete g_pWaveSoundRead;
	return sound;
}


float GetGlobalVolume()
{
	return global_volume;
}

void SetGlobalVolume(float g)
{
	global_volume=g;
}

void CSound::SetVolume(long vol)
{
	if(buffer==NULL)return;

	vol += round((1.0f - global_volume) * (float)(-10000 - vol));
	buffer->SetVolume(vol);
}

void SoundEnable(bool b)
{
	sound_enable=b;
}

bool IsSoundEnabled()
{
	return sound_enable;
}
