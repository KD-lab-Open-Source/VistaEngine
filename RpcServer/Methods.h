#pragma once
#ifndef __VISTARPC_METHODS_H_INCLUDED__
#define __VISTARPC_METHODS_H_INCLUDED__

#include "XmlRpc\XmlRpcServerMethod.h"
#include "UserInterface\XmlRpc\RpcTypes.h"
#include "Serialization\Serialization.h"

namespace RpcMethod
{
class RpcMethodBase : public XmlRpc::XmlRpcServerMethod
{
public:
	RpcMethodBase(const char* methodName) : XmlRpcServerMethod(methodName, server_) {}

	void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result);

	virtual void serialize(Archive& in) {};
	virtual int call(Archive& out) = 0;

	static void setServer(XmlRpc::XmlRpcServer* s) { server_ = s; }

private:
	static XmlRpc::XmlRpcServer* server_;
};

template<class ParamType>
class RpcMethodParam : public RpcMethodBase
{
public:
	RpcMethodParam(const char* methodName) : RpcMethodBase(methodName) {}
	void serialize(Archive& in)	{ in.serialize(data_, "root", 0); }
protected:
	ParamType data_;
};

struct VectorSum : public RpcMethodParam<RpcType::SumType>
{
	VectorSum() : RpcMethodParam<RpcType::SumType>("SUM") {}
	int call(Archive& out);
};

struct GetScoresMethod : public RpcMethodParam<RpcType::GetScore>
{
	GetScoresMethod() : RpcMethodParam<RpcType::GetScore>("GetScores") {}
	int call(Archive& out);
};

}

#endif //__VISTARPC_METHODS_H_INCLUDED__