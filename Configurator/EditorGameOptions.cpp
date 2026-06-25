#include "StdAfx.h"
#include "EditorGameOptions.h"
Game/GameOptions.h

EditorGameOptions::EditorGameOptions()
{

}

void EditorGameOptions::serialize(Archive& ar)
{
    GameOptions::instance().serializeForEditor(ar);
}
