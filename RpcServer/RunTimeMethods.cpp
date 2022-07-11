#include "stdafx.h"
#include "Runtime.h"
#include "GUIDString.h"

#include "MySQLpp\TableBase.h"
#include "MySQLpp\mysql++.h"
#include "MySQLpp\transaction.h"

#include "UserInterface\XmlRpc\RpcEnums.h"
#include "UserInterface\XmlRpc\RpcTypes.h"

class TableUser : public TableBase
{
public:
	TableUser() : TableBase("users") {}
};

class TableScore : public TableBase
{
public:
	TableScore() : TableBase("scores") {}
};

void Runtime::initTables()
{
	users_ = new TableUser;
	scores_ = new TableScore;
}

void Runtime::finitTables()
{
	delete scores_;
	delete users_;
}

int Runtime::registerUser(const RpcType::LoginData& data)
{
	TableConnection con(users_);

	mysqlpp::Query query = con->query();
	mysqlpp::Transaction transaction(*con);

	query	<< "SELECT COUNT(*) FROM " << users_->table()
		<< " WHERE LOGIN = " << mysqlpp::quote << data.login;

	mysqlpp::StoreQueryResult res = query.store();
	if(!res){
		addLog(-1, "Ошибка выборки данных в Register user");
		addLog(2, con->error());
		return STATUS_OTHER_ERROR;
	}

	if(!res.empty() && (unsigned)res[0][0] > 0)
		return STATUS_USER_NAME_EXIST;

	query	<< "INSERT INTO " << users_->table()
		<< " (LOGIN, PASSWORD) VALUES ("
		<< mysqlpp::quote << data.login << ", "
		<< mysqlpp::quote << data.pass << ")";

	if(query.execute()){
		transaction.commit();
		return STATUS_GOOD;
	}
	else {
		addLog(-1, "Ошибка добавления данных в Register user");
		addLog(2, con->error());
	}

	return STATUS_OTHER_ERROR;
}

int Runtime::login(const RpcType::LoginData& data)
{
	TableConnection con(users_);

	GUIDString session(data.session);

	mysqlpp::Query query = con->query();
	query	<< "UPDATE " << users_->table()
		<< " SET SESSION = " << mysqlpp::quote_only << session.getString()
		<< " WHERE LOGIN = " << mysqlpp::quote << data.login
		<< " AND PASSWORD = " << mysqlpp::quote << data.pass;

	mysqlpp::SimpleResult res = query.execute();
	if(!res){
		addLog(-1, "Ошибка обновления данных в Login user");
		addLog(2, con->error());
		return STATUS_OTHER_ERROR;
	}

	if(res.rows() == 0)
		return STATUS_BAD_USER_OR_PASSWORD;

	return STATUS_GOOD;
}

int Runtime::logout(const XGUID& g)
{
	TableConnection con(users_);

	GUIDString session(g);

	mysqlpp::Query query = con->query();
	query	<< "UPDATE " << users_->table()
		<< " SET SESSION = '00000000-0000-0000-0000-000000000000'"
		<< " WHERE SESSION = " << mysqlpp::quote_only << session.getString();

	mysqlpp::SimpleResult res = query.execute();
	if(!res){
		addLog(-1, "Ошибка обновления данных в Logout user");
		addLog(2, con->error());
		return STATUS_OTHER_ERROR;
	}

	if(res.rows() == 0)
		return STATUS_DOUBLE_OR_NOT_LOGON;

	return STATUS_GOOD;
}

int Runtime::addScores(const RpcType::ScoreData& data)
{
	unsigned long id = 0;
	TableConnection U(users_);
	mysqlpp::Query qU = U->query();

	if(data.session != XGUID::ZERO){ // ищем по валидной сессии
		GUIDString session(data.session);
		qU	<< "SELECT ID FROM " << users_->table()
			<< " WHERE SESSION = " << mysqlpp::quote_only << session.getString();

		mysqlpp::StoreQueryResult res = qU.store();
		if(!res){
			addLog(-1, "Ошибка выборки данных в addScores user");
			addLog(2, U->error());
			return STATUS_OTHER_ERROR;
		}

		if(res.empty())
			return STATUS_DOUBLE_OR_NOT_LOGON;

		id = res[0][0];
	}
	else if(!data.login.empty()){ // по имени юзера
		qU	<< "SELECT ID FROM " << users_->table()
			<< " WHERE LOGIN = " << mysqlpp::quote << data.login;
		mysqlpp::StoreQueryResult res = qU.store();
		if(!res){
			addLog(-1, "Ошибка выборки данных в addScores user");
			addLog(2, U->error());
			return STATUS_OTHER_ERROR;
		}

		if(res.empty()){
			qU	<< "INSERT INTO " << users_->table()
				<< " (LOGIN, PASSWORD) VALUES ("
				<< mysqlpp::quote << data.login << ", '')";

			mysqlpp::SimpleResult qr = qU.execute();
			if(!qr){
				addLog(-1, "Ошибка добавления юзера в addScores user");
				addLog(2, U->error());
				return STATUS_OTHER_ERROR;
			}
			id = static_cast<unsigned long>(qr.insert_id());
		}
		else
			id = res[0][0];
	}
	else
		return STATUS_BAD_USER_OR_PASSWORD;

	GUIDString lock(data.query);

	TableConnection S(scores_);
	mysqlpp::Query qS = S->query();

	qS	<< "INSERT IGNORE " << scores_->table()
		<< " (USER_ID, TYPE, SCORES, LAST) VALUES ("
		<< id << ", " << data.type << ", " << data.score << ", "
		<< mysqlpp::quote_only << lock.getString() << ")";
	mysqlpp::SimpleResult qr = qS.execute();
	if(!qr){
		addLog(-1, "Ошибка выборки данных в addScores user");
		addLog(2, S->error());
		return STATUS_OTHER_ERROR;
	}

	if(qr.rows() == 0){
		qS	<< "UPDATE " << scores_->table()
			<< " SET SCORES = SCORES + (" << data.score << "),"
			<< " LAST = " << mysqlpp::quote_only << lock.getString()
			<< " WHERE USER_ID = " << id
			<< " AND TYPE = " << data.type
			<< " AND LAST <> " << mysqlpp::quote_only << lock.getString();
		mysqlpp::SimpleResult qr = qS.execute();
		if(!qr){
			addLog(-1, "Ошибка обновления данных в addScores user");
			addLog(2, S->error());
			return STATUS_OTHER_ERROR;
		}
		if(qr.rows() == 0)
			addLog(2, "Попытка двойного добавления очков");
	}

	return STATUS_GOOD;
}


int Runtime::getScores(unsigned long start, unsigned quantity, RpcType::Scores& out)
{
	out.push_back(RpcType::ScoreNode());
	RpcType::ScoreNode& node = out.back();
	node.login = "PlayerName";
	node.scores.push_back(RpcType::ScoreNode::ScoreAtom(1, 2));
	
	return STATUS_GOOD;
}