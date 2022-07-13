#ifndef __SUR_TOOL_PLAYER_FOLDER_H_INCLUDED__
#define __SUR_TOOL_PLAYER_FOLDER_H_INCLUDED__

#include "../Units/UnitAttribute.h"
#include "../Game/Player.h"

#include "SurToolAux.h"
#include "SurToolEmpty.h"

class CSurToolPlayerFolder : public CSurToolEmpty
{
	DECLARE_DYNAMIC(CSurToolPlayerFolder)

public:
	CSurToolPlayerFolder (CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolPlayerFolder();
	virtual BOOL OnInitDialog ();
	virtual void FillIn ();

    void setPlayer (Player* _player);
	void serialize(Archive& ar);

// Dialog Data
	virtual int getIDD () { return IDD_BARDLG_PLAYERFOLDER; }
protected:
    Player* player_;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
};

#endif
