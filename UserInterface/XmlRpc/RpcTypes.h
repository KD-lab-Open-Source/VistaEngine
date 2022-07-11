#pragma once
#ifndef __VISTARPC_RPC_TYPES_H_INCLUDED__
#define __VISTARPC_RPC_TYPES_H_INCLUDED__

#include "FileUtils\XGUID.h"

class Archive;

namespace RpcType
{

struct SumType
{
	SumType() {}

	void serialize(Archive& ar);

	int sum() const;

	vector<int> data_;
};

struct LoginData
{
	LoginData(const char* l = "", const char* p = "", const GUID* g = 0);
	void serialize(Archive& ar);
	string login;
	string pass;
	XGUID session;
};

struct ScoreData
{
	ScoreData(int sc = 0, unsigned t = 0, const char* l = "", const GUID* ses = 0);
	void serialize(Archive& ar);
	string login;
	XGUID session;
	XGUID query;
	int score;
	unsigned type;
};

struct GetScore
{
	GetScore(unsigned long s = 0, unsigned q = 1);
	void serialize(Archive& ar);

	unsigned long startRank;
	unsigned quantity;
};


struct ScoreNode
{
	struct ScoreAtom
	{
		ScoreAtom(unsigned t = 0, int s = 0) : type(t), score(s) {}
		void serialize(Archive& ar);
		unsigned type;
		int score;
	};
	typedef vector<ScoreAtom> UserScores;

	void serialize(Archive& ar);
	string login;
	UserScores scores;
};
typedef vector<ScoreNode> Scores;

struct ReturnScore
{
	ReturnScore();
	void serialize(Archive& ar);

	unsigned long startRank;
	Scores scores;
};

};

#endif //__VISTARPC_RPC_TYPES_H_INCLUDED__

