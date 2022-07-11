#include "stdafx.h"
#include "RpcTypes.h"

#include "Serialization\Serialization.h"

using namespace RpcType;

void SumType::serialize(Archive& ar)
{
	ar.serialize(data_, "vec", 0);
}


int SumType::sum() const
{
	int s = 0;
	vector<int>::const_iterator it = data_.begin();
	for(; it != data_.end(); ++it)
		s += *it;
	return s;
}


LoginData::LoginData(const char* l, const char* p, const GUID* guid)
: login(l)
, pass(p)
{
	if(guid)
		session = *guid;
	else
		session = XGUID::ZERO;
}

void LoginData::serialize(Archive& ar)
{
	ar.serialize(login, "login", 0);
	ar.serialize(pass, "pass", 0);
	ar.serialize(session, "session", 0);
}


ScoreData::ScoreData(int sc, unsigned t, const char* l, const GUID* ses)
{
	score = sc;
	type = t;
	query.generate();
	login = l;
	if(ses)
		session = *ses;
	else
		session = XGUID::ZERO;
}

void ScoreData::serialize(Archive& ar)
{
	ar.serialize(login, "login", 0);
	ar.serialize(session, "session", 0);
	ar.serialize(query, "query", 0);
	ar.serialize(score, "score", 0);
	ar.serialize(type, "type", 0);
}


GetScore::GetScore(unsigned long s, unsigned q)
{
	startRank = s;
	quantity = q;
}

void GetScore::serialize(Archive& ar)
{
	ar.serialize(startRank, "startRank", 0);
	ar.serialize(quantity, "quantity", 0);
}

void ScoreNode::ScoreAtom::serialize(Archive& ar)
{
	ar.serialize(type, "type", 0);
	ar.serialize(score, "score", 0);
}

void ScoreNode::serialize(Archive& ar)
{
	ar.serialize(login, "login", 0);
	ar.serialize(scores, "scores", 0);
}

ReturnScore::ReturnScore()
{
	startRank = 0;
}

void ReturnScore::serialize(Archive& ar)
{
	ar.serialize(startRank, "startRank", 0);
	ar.serialize(scores, "scores", 0);
}