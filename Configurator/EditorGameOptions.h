#ifndef __NICE_GAME_OPTIONS_H_INCLUDED__
#define __NICE_GAME_OPTIONS_H_INCLUDED__

class Archive;

class EditorGameOptions{
public:
    EditorGameOptions();
    void serialize(Archive& ar);
};

#endif
