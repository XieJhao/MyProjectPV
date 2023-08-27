#pragma once
#include"ThreadPool.h"
#include"Socket.h"
#include"Epoll.h"
#include"Procass.h"
#include"Logger.h"

template<typename _FUNCTION_, typename... _ARGS_>
class CConnectedFunction : public CFunctionBase
{
public:
	CConnectedFunction(_FUNCTION_ func, _ARGS_... args)
		:m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...)
	{

	}
	~CConnectedFunction() {}

	virtual int operator()(CSocketBase* pCLient) {
		return m_binder(pCLient);
	}
	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;

};

template<typename _FUNCTION_, typename... _ARGS_>
class CReceivedFunction : public CFunctionBase
{
public:
	CReceivedFunction(_FUNCTION_ func, _ARGS_... args)
		:m_binder(std::forward<_FUNCTION_>(func), std::forward<_ARGS_>(args)...)
	{

	}
	~CReceivedFunction() {}

	virtual int operator()(CSocketBase* pCLient, const Buffer& data) {
		return m_binder(pCLient, data);
	}
	typename std::_Bindres_helper<int, _FUNCTION_, _ARGS_...>::type m_binder;

};

class CBusiness
{
public:
	CBusiness()
		:m_connectedcallback(NULL),
		m_recvcallback(NULL)
	{}
	virtual int BusinessProcass(CProcass* proc) = 0;
	template<typename _FUNCTION_, typename... _ARGS_>
	int setConnectedCallback(_FUNCTION_ func, _ARGS_... args)
	{
		m_connectedcallback = new CConnectedFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_connectedcallback == NULL)return -1;
		return 0;
	}
	template<typename _FUNCTION_, typename... _ARGS_>
	int setRecvCallback(_FUNCTION_ func, _ARGS_... args)
	{
		m_recvcallback = new CReceivedFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_recvcallback == NULL)return -1;
		return 0;
	}
protected:
	CFunctionBase* m_connectedcallback;
	CFunctionBase* m_recvcallback;
};

class CServer
{
public:
	CServer();
	~CServer() { Close(); }
	CServer(const CServer&) = delete;
	CServer operator=(const CServer&) = delete;
public:
	int Init(CBusiness* business,const Buffer& ip = "0.0.0.0", short port = 9999);
	int Run();
	int Close();
private:
	int ThreadFunc();
private:
	CThreadPool m_pool;
	CSocketBase* m_server;
	CEpoll m_epoll;
	CProcass m_process;
	CBusiness* m_business;
};

