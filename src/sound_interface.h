
#if !defined(SOUNDINTERFACE_H)

#include "fmod_studio.h"
#include "fmod_studio.hpp"
#include "fmod_errors.h"

//NOTE(Ray):Depends on FMOD

struct SoundClip
{
    FMOD_STUDIO_PLAYBACK_STATE playback_state;
    FMOD::Studio::EventDescription* desc;
    FMOD::Studio::EventInstance* state;    
};

struct SoundBuffer
{
};

struct SoundDevice
{
};

struct SoundListener
{
    FMOD_3D_ATTRIBUTES state;
};

struct SoundBus
{
    FMOD::Studio::Bus* state;
};

struct SoundBank
{
    FMOD::Studio::Bank* state;
};

struct SoundEventBuffer
{
    FMJStretchBuffer sound_events;
};

struct SoundBankBuffer
{
    FMJStretchBuffer sound_banks;
};

struct SoundBusBuffer
{
    FMJStretchBuffer sound_buses;
};

namespace SoundCode
{
//    static FMOD::Studio::System* sound_system = NULL;
    void Init();
    //GOOD for 2d sound effects
    void SetDefaultListener();
    void ContinousPlay(SoundClip* sample);
    void Stop(SoundClip* sample);
    void Update();
}

namespace SoundAssetCode
{
    //NOTE(Ray):This is here for convience.
//    static SoundBankBuffer bank_buffer;
    //static SoundBusBuffer bus_buffer;     
    bool Init();
    void CreateSoundBank(SoundBankBuffer* buffer,char* bank_name);
    void CreateSoundBank(char* bank_name);
    void LoadBankSampleData(SoundBank* bank);
    void LoadBankSampleData();
    void GetBus(SoundBusBuffer* buffer,char* bus_name);
    void GetBus(char* bus_name);
    bool GetEvent(char* event_name,SoundClip* clip);
}
#define SOUNDINTERFACE_H
#endif
 
