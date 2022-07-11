#include "stdafx.h"
#include "Methods.h"
#include "Serialization\BinaryArchive.h"
#include "XmlRpc\XmlRpcValue.h"
#include "Runtime.h"

using namespace XmlRpc;
using namespace RpcMethod;

XmlRpcServer* RpcMethodBase::server_ = 0;

void RpcMethodBase::execute(XmlRpcValue& params, XmlRpcValue& result)
{
	XmlRpcValue::BinaryData& data = params[0];
	//decodeData(data);

	BinaryIArchive ia(&data[0], data.size());
	serialize(ia);

	BinaryOArchive oa;
	int status = call(oa);
	oa.serialize(status, "status", 0);

	result = XmlRpcValue(oa.data(), oa.size());
	//encodeData(result);
}

// VectorSum 
// -------------------------------------------------------------------------- 
int VectorSum::call(Archive& out)
{
	int answer = data_.sum();
	out.serialize(answer, "root", 0);
	return 0;
}


int GetScoresMethod::call(Archive& out)
{
	RpcType::ReturnScore ret;
	ret.startRank = data_.startRank;
	int status = runtime->getScores(data_.startRank, data_.quantity, ret.scores);
	out.serialize(ret, "root", 0);
	return status;
}