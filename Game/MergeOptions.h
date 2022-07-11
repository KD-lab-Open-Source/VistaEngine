#ifndef __MERGE_OPTIONS_H_INCLUDED__
#define __MERGE_OPTIONS_H_INCLUDED__

struct MergeOptions
{
    MergeOptions();
    
	string worldFile;
    bool mergePlayers;
    bool mergeWorld;
    bool mergeSourcesAndAnchors;
    bool mergeCameras;
    void serialize(Archive& ar);
};

#endif
