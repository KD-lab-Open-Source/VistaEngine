// Stub PNetCenter (netcenter) for the cross-platform build.
//
// The real implementation is spread across the DirectPlay-based
// P2P_interface*.cpp files, which are Windows-only (gated in Network/CMakeLists.txt).
// These no-op stubs provide the PNetCenter methods the engine references so the
// game links; real cross-platform networking (SDL_net or similar) is a separate
// Track-B effort. Only the symbols that aren't already defined by the compiled
// Network sources (P2P_interface2Th.cpp / P2P_interfaceAnyTh.cpp) are stubbed here.
#include "stdafx.h"
#include "P2P_interface.h"

// Member init list mirrors the real ctor (P2P_interface1Th.cpp): these members
// have no default constructor.
PNetCenter::PNetCenter(ExternalNetTask_Init*)
	: in_ClientBuf(1000000, 1), out_ClientBuf(1000000, 1),
	  in_HostBuf(1000000, 1), out_HostBuf(1000000, 1),
	  startGameParam(m_GeneralLock)
{}
PNetCenter::~PNetCenter() {}

void PNetCenter::getGameHostList(vector<sGameHostInfo>&) {}
void PNetCenter::JoinCommand(int) {}
void PNetCenter::KickInCommand(int, int) {}
void PNetCenter::FinishGame() {}
void PNetCenter::ResetAndStartFindHost() {}
void PNetCenter::implementingENT(ENTCreateAccount*) {}
void PNetCenter::implementingENT(ENTDeleteAccount*) {}
void PNetCenter::implementingENT(ENTChangePassword*) {}
void PNetCenter::implementingENT(ENTLogin*) {}
void PNetCenter::logout() {}
void PNetCenter::implementingENT(ENTDownloadInfoFile*) {}
void PNetCenter::implementingENT(ENTReadGlobalStats*) {}
void PNetCenter::implementingENT(ENTSubscribe2ChatChannel*) {}
void PNetCenter::unsub2ChatChannel(unsigned __int64) {}
void PNetCenter::implementingENT(ENTGame*) {}
void PNetCenter::changePlayerRace(int, Race) {}
void PNetCenter::changePlayerColor(int, int) {}
void PNetCenter::changePlayerSign(int, int) {}
void PNetCenter::changeRealPlayerType(int, RealPlayerType) {}
void PNetCenter::changePlayerDifficulty(int, Difficulty) {}
void PNetCenter::changePlayerClan(int, int) {}
void PNetCenter::changeMissionDescription(MissionDescriptionNet::eChangedMDVal, int) {}
bool PNetCenter::plyaerIsReadyOrStartLoadGame() { return false; }
void PNetCenter::setGameIsReady() {}
bool PNetCenter::setPause(bool) { return false; }
void PNetCenter::SendEvent(const NetCommandBase*) {}
bool PNetCenter::chatMessage(const ChatMessage&) { return false; }
void PNetCenter::setGameHostFilter(int, std::vector<XGUID>, int) {}
void PNetCenter::immediatelyRefreshGameHostList() {}
void PNetCenter::getChatChanelList(vector<ChatChanelInfo>&) {}
void PNetCenter::getChatMembers(vector<ChatMemberInfo>&) {}
const char* PNetCenter::getMyPublicIP() { return ""; }
const char* PNetCenter::getStrWorkMode() { return ""; }
const char* PNetCenter::getStrState() { return ""; }
void PNetCenter::quant_th1() {}
int PNetCenter::Send(const char*, int, const UNetID&, bool) { return 0; }
