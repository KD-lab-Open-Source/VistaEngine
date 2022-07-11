// Hierarhy.cpp : implementation file
//

#include "stdafx.h"
#include "WinVG.h"
#include "Hierarhy.h"


// CHierarhy dialog

IMPLEMENT_DYNAMIC(CHierarhy, CDialog)
CHierarhy::CHierarhy(cObject3dx* pObject_,CWnd* pParent /*=NULL*/)
	: CDialog(CHierarhy::IDD, pParent)
{
	selected_object=12;//-1;
	pObject=pObject_;
}

CHierarhy::~CHierarhy()
{
}

void CHierarhy::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE, tree);

	if(!pDX->m_bSaveAndValidate)
		Init();
	else
	{
		selected_object=-1;
		HTREEITEM hselected=tree.GetSelectedItem();
		for(int i=0;i<nodes.size();i++)
		{
			if(hselected==nodes[i])
			{
				selected_object=i;
				break;
			}
		}
	}
		
}


BEGIN_MESSAGE_MAP(CHierarhy, CDialog)
END_MESSAGE_MAP()


// CHierarhy message handlers
void CHierarhy::Init()
{
	cStatic3dx* pStatic=pObject->GetStatic();

	nodes.resize(pStatic->nodes.size());
	if(nodes.empty())
		return;
	
	for(int i=0;i<nodes.size();i++)
	{
		cStaticNode& sn=pStatic->nodes[i];
		HTREEITEM hparent=TVI_ROOT;
		if(sn.iparent>=0)
		{
			xassert(sn.iparent<i);
			hparent=nodes[sn.iparent];
		}

		nodes[i]=tree.InsertItem(sn.name.c_str(),hparent); 
	}

	for(i=0;i<nodes.size();i++)
	{
		tree.Expand(nodes[i],TVE_EXPAND);
	}

	if(selected_object>=0 && selected_object<nodes.size())
		tree.Select(nodes[selected_object],TVGN_CARET );
}
