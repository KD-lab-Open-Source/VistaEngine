// AnimationChainDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AnimationChainDlg.h"
#include "resource.h"
#include "AddChainDlg.h"

// CAnimationChainDlg dialog

IMPLEMENT_DYNAMIC(CAnimationChainDlg, CDialog)
CAnimationChainDlg::CAnimationChainDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAnimationChainDlg::IDD, pParent)
{
}

CAnimationChainDlg::~CAnimationChainDlg()
{
}

void CAnimationChainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CHAINS, m_ListChain);
}


BEGIN_MESSAGE_MAP(CAnimationChainDlg, CDialog)
	ON_BN_CLICKED(IDC_ADD_CHAIN, OnBnClickedAddChain)
	ON_BN_CLICKED(IDC_DELETE_CHAIN, OnBnClickedDeleteChain)
	ON_BN_CLICKED(IDC_EDIT_CHAIN, OnBnClickedEditChain)
	ON_NOTIFY(HDN_ITEMDBLCLICK, 0, OnHdnItemdblclickListChains)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_CHAINS, OnNMDblclkListChains)
END_MESSAGE_MAP()


// CAnimationChainDlg message handlers

BOOL CAnimationChainDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ImageList.Create(IDB_TREE_IMAGE,16,4,RGB(255,255,255));
	m_ListChain.SetImageList(&m_ImageList);

	DWORD style = m_ListChain.GetExtendedStyle();
	style |= LVS_EX_FULLROWSELECT; 
	m_ListChain.SetExtendedStyle(style);
	m_ListChain.InsertColumn(LS_NAME,"Name",LVCFMT_LEFT,75);
	m_ListChain.InsertColumn(LS_BEGIN,"Chain Start",LVCFMT_LEFT,75);
	m_ListChain.InsertColumn(LS_END,"Chain End",LVCFMT_LEFT,75);
	m_ListChain.InsertColumn(LS_TIME,"Time",LVCFMT_LEFT,75);
	m_ListChain.InsertColumn(LS_CYCLED,"Cycled",LVCFMT_LEFT,75);

	animation_chain=pRootExport->animation_data.animation_chain;
	animation_chain_group=pRootExport->animation_data.animation_chain_group;

	for(int i=0;i<animation_chain.size();i++)
	{
		SetItem(i,true);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
void CAnimationChainDlg::SetItem(int index,bool insert)
{
	xassert(index>=0 && index<animation_chain.size());
	AnimationChain& ac=animation_chain[index];
	LVITEM item;
	item.mask=LVIF_TEXT | LVIF_IMAGE;
	item.state=0;
	item.stateMask=0;
	item.iItem=index;
	item.iSubItem=0;
	item.pszText=(char*)ac.name.c_str();
	item.iImage = 2;

	if(insert)
	{
		int id = m_ListChain.InsertItem(&item);
		xassert(index==id);
	}else
	{
		m_ListChain.SetItem(&item);
	}

	char buf[32];

	sprintf(buf,"%i",ac.begin_frame);
	m_ListChain.SetItemText(index,LS_BEGIN,buf);

	sprintf(buf,"%i",ac.end_frame);
	m_ListChain.SetItemText(index,LS_END,buf);

	sprintf(buf,"%.2f",ac.time);
	m_ListChain.SetItemText(index,LS_TIME,buf);

	m_ListChain.SetItemText(index,LS_CYCLED,ac.cycled?"cycled":"");
}

void CAnimationChainDlg::OnBnClickedAddChain()
{
	CAddChainDlg dlg;
	
	if (dlg.DoModal() == IDOK)
	{
		string name = TrimString((LPCSTR)dlg.m_Name);
		for(int ichain=0;ichain<animation_chain.size();ichain++)
		{
			AnimationChain& ac=animation_chain[ichain];
			if(ac.name == name)
			{
				MessageBox("Такая анимационная цепочка уже существует","Error",MB_OK | MB_ICONSTOP);
				return;
			}
		}

		AnimationChain ac;
		ac.name			= name;
		ac.begin_frame	= dlg.m_nBegin;
		ac.end_frame	= dlg.m_nEnd;
		ac.time			= dlg.m_fTime;
		ac.cycled		= dlg.m_bCycled;
		animation_chain.push_back(ac);
		SetItem(animation_chain.size()-1,true);
	
	}
}

void CAnimationChainDlg::OnBnClickedDeleteChain()
{
	int mark = m_ListChain.GetSelectionMark();
	if(mark < 0 || mark >= animation_chain.size())
		return;

	m_ListChain.DeleteItem(mark);
	animation_chain.erase(animation_chain.begin()+mark);
}

void CAnimationChainDlg::OnBnClickedEditChain()
{
	int mark = m_ListChain.GetSelectionMark();
	if(mark < 0 || mark >= animation_chain.size())
		return;

	CAddChainDlg dlg;

	AnimationChain& ac=animation_chain[mark];

	dlg.m_Name = ac.name.c_str();
	dlg.m_nBegin = ac.begin_frame;
	dlg.m_nEnd = ac.end_frame;
	dlg.m_fTime = ac.time;
	dlg.m_bCycled = ac.cycled;

	if (dlg.DoModal() == IDOK)
	{
		ac.name			= TrimString((LPCSTR)dlg.m_Name);
		ac.begin_frame	= dlg.m_nBegin;
		ac.end_frame	= dlg.m_nEnd;
		ac.time			= dlg.m_fTime;
		ac.cycled		= dlg.m_bCycled;
		SetItem(mark,false);
	}
	
	//m_ListChain.DeleteItem(mark);
	//animation_chain.erase(animation_chain.begin()+mark);
}

void CAnimationChainDlg::OnHdnItemdblclickListChains(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;
}

void CAnimationChainDlg::OnNMDblclkListChains(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnBnClickedEditChain();
	*pResult = 0;
}

void CAnimationChainDlg::OnOK()
{
	pRootExport->animation_data.animation_chain = animation_chain;
	pRootExport->animation_data.animation_chain_group = animation_chain_group;
	CDialog::OnOK();
}


ChainsBlock::ChainsBlock()
{
    positionCounter_ = rotationCounter_ = scaleCounter_ = visibilityCounter_ = uvCounter_ = 0;
}


template<class Chains>
int calcNodesInChains(const Chains& chains){
	int count = 0;
	Chains::const_iterator it;
	FOR_EACH(chains, it){
		count += it->size();
	}
	// Msg("calcNodesInChains(): %i in %i\n", count, chains.size());
	return count;
}

void ChainsBlock::save(Saver& saver) const
{
	int sz = 0;
	saver.push(C3DX_STATIC_CHAINS_BLOCK);

    saver << int(calcNodesInChains(positionChains_));
    saver << int(calcNodesInChains(rotationChains_));
    saver << int(calcNodesInChains(scaleChains_));
	saver << int(calcNodesInChains(visibilityChains_));
    saver << int(calcNodesInChains(uvChains_));

	PositionChains::const_iterator itp;
	FOR_EACH(positionChains_, itp)
		itp->SaveFixed(saver);

	RotationChains::const_iterator itr;
	FOR_EACH(rotationChains_, itr)
		itr->SaveFixed(saver);

	ScaleChains::const_iterator its;
	FOR_EACH(scaleChains_, its)
		its->SaveFixed(saver);

	VisibilityChains::const_iterator itv;
	FOR_EACH(visibilityChains_, itv)
		itv->SaveFixed(saver);

	UVChains::const_iterator itu;
	FOR_EACH(uvChains_, itu)
		itu->SaveFixed(saver);

	sz = saver.pop();
	// Msg("ChainsBlock closed, size = %i\n", sz);
}

ChainsBlock::~ChainsBlock()
{
#ifdef _DEBUG
    Msg(" -- DEBUG -- \n");
    Msg("Уникальных цепочек перемещения:        %i/%i (%.2f%%)\n",
        positionChains_.size(), positionCounter_, 100.0f * float(positionChains_.size()) / float(max(positionCounter_,1)));
    Msg("Уникальных цепочек вращения:           %i/%i (%.2f%%)\n",
        rotationChains_.size(), rotationCounter_, 100.0f * float(rotationChains_.size()) / float(max(rotationCounter_,1)));
    Msg("Уникальных цепочек масшатбирования:    %i/%i (%.2f%%)\n",
        scaleChains_.size(),    scaleCounter_,    100.0f * float(scaleChains_.size())    / float(max(scaleCounter_,1)));
    Msg("Уникальных цепочек видимости:          %i/%i (%.2f%%)\n",
        visibilityChains_.size(), visibilityCounter_, 100.0f * float(visibilityChains_.size())    / float(max(visibilityCounter_,1)));
    Msg("Уникальных цепочек UV:                 %i/%i (%.2f%%)\n",
        uvChains_.size(), uvCounter_, 100.0f * float(uvChains_.size()) / float(max(uvCounter_,1)));
#endif
}

template<class Chains, class ChainType>
int findOrPushBackChain(Chains& chains, const ChainType& chain){
    Chains::iterator it;
	int key_index = 0;
	for(it = chains.begin(); it != chains.end(); ++it){
		if(*it == chain){
			return key_index;
		}
		key_index += it->size();
	}

    chains.push_back(chain);
	return key_index;
}


int ChainsBlock::put(PositionChain& chain)
{
    ++positionCounter_;
	int result = findOrPushBackChain(positionChains_, chain);
    return result;
}

int ChainsBlock::put(RotationChain& chain)
{
    ++rotationCounter_;
    return findOrPushBackChain(rotationChains_, chain);
}

int ChainsBlock::put(ScaleChain& chain)
{
    ++scaleCounter_;
    return findOrPushBackChain(scaleChains_, chain);
}

int ChainsBlock::put(VisibilityChain& chain)
{
    ++visibilityCounter_;
    return findOrPushBackChain(visibilityChains_, chain);
}

int ChainsBlock::put(UVChain& chain)
{
	++uvCounter_;
    return findOrPushBackChain(uvChains_, chain);
}

struct OneVisibility{
	int interval_begin;
	int interval_size;
	bool value;
};

VisibilityChain::VisibilityChain(const vector<bool>& visibility, int interval_size, bool cycled)
{
    if(visibility.empty())
        return;
    vector<OneVisibility> intervals;
    //В принципе аналог кода из InterpolatePosition, но для констант.
    int size = visibility.size();
    bool cur = visibility[0];
    int begin=0;
    for(int i=0;i<size;i++)
    {
        bool b=visibility[i];
        if(b!=cur)
        {
            OneVisibility one;
            one.interval_begin=begin;
            one.interval_size=i-begin;
            xassert(one.interval_size>0);
            one.value=cur;
            intervals.push_back(one);

            begin=i;
            cur=b;
        }
    }

    OneVisibility one;
    one.interval_begin=begin;
    one.interval_size=i-begin;
    xassert(one.interval_size>0);
    one.value=cur;
    intervals.push_back(one);

    int sum=0;
    int cur_begin=0;
    for(i=0;i<intervals.size();i++)
    {
        OneVisibility& o=intervals[i];
        xassert(o.interval_begin==sum);
        sum+=o.interval_size;
    }
    xassert(sum==size);

    nodes_.resize(intervals.size());
    for(i = 0; i < intervals.size(); i++){
        OneVisibility& o = intervals[i];
        nodes_[i].interval_begin = float((o.interval_begin)/(float)interval_size);
        nodes_[i].interval_size  = float(o.interval_size/(float)interval_size);
        nodes_[i].value = o.value;
    }
}

void VisibilityChain::SaveFixed(Saver& saver) const{
    for(int i=0; i < nodes_.size(); i++){
        const Node& node = nodes_[i];
        saver << node.interval_begin;
        saver << 1.0f / node.interval_size;
        saver << (int)node.value;
    }
}

void VisibilityChain::Save(Saver& saver) const{
    saver << (int)nodes_.size();
    for(int i=0; i < nodes_.size(); i++){
        const Node& node = nodes_[i];
        saver << node.interval_begin;
        saver << node.interval_size;
        saver << node.value;
    }
}
