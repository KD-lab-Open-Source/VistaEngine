#ifndef __HT_H_INCLUDED__
#define __HT_H_INCLUDED__

/* В одном потоке работает логика, в другом - графика.
GameShell в графическом потоке.
*/

class MissionDescription;

class HTManager
{
public:
	HTManager(bool ht);
	~HTManager();

	bool Quant();

	static HTManager* instance(){ return self; }

	void GameStart(const MissionDescription& mission);
	void GameClose();

	void setUseHT(bool use_ht_);
	bool IsUseHT()const{return use_ht;};
	bool PossibilityHT();

	bool terminateLogicThread() const { return end_logic != 0; }

protected:
	static HTManager* self;

	bool use_ht;
	void init();
	void done();
	void initGraphics();
	void finitGraphics();

	DWORD logic_thread_id;
	void logic_thread();
	friend void logic_thread( void * argument);

	bool init_logic;

	HANDLE end_logic;
 };

#endif
