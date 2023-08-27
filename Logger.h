#pragma once
#include"Epoll.h"
#include"Socket.h"
#include"Thread.h"
#include<list>
#include<sys/timeb.h>
#include<stdarg.h>
#include<sstream>
#include<sys/stat.h>

enum logLevel
{
	LOG_INFO,
	LOG_DEBUG,
	LOG_WARNING,
	LOG_ERROR,
	LOG_FATIAL
};

class CLoggerServer;
class LogInfo;

class LogInfo
{
public:
	LogInfo(const char* file, int line, const char* func,
		pid_t pid, pthread_t tid, int level,
		const char* fmt, ...);
	
	LogInfo(const char* file, int line, const char* func,
		pid_t pid, pthread_t tid, int level);
	
	LogInfo(const char* file, int line, const char* func,
		pid_t pid, pthread_t tid, int level,
		void* pData, size_t nSize);
	
	~LogInfo();
	
	operator Buffer()const {
		return m_buf;
	}
	 const char* c_str() { return m_buf.c_str(); }
	template<typename T>
	LogInfo& operator<<(const T& data)
	{
		std::stringstream stream;
		stream << data;
		m_buf += stream.str().c_str();
		return *this;
	}
private:
	bool bAuto;//默认是false 流失日志则为true
	Buffer m_buf;
};


class CLoggerServer
{
public:
	CLoggerServer()
		:m_thread(&CLoggerServer::ThreadFunc,this)
	{
		m_server = NULL;
		char curpath[256] = "";
		getcwd(curpath, sizeof(curpath));
		m_path = curpath;
		m_path += "/log/" + GetTimeStr() + ".log";
		printf("%s(%d):[%s] path=%s\n", __FILE__, __LINE__, __FUNCTION__, (char*)m_path);
	}
	CLoggerServer(const CLoggerServer&) = delete;
	~CLoggerServer() { Close(); }
public:
	CLoggerServer& operator=(const CLoggerServer&) = delete;
public:
	int Start()
	{
		//日志服务器的启动
		if (m_server != NULL)return -1;
		if (access("log", W_OK | R_OK) != 0)
		{
			mkdir("log", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		}
		m_file = fopen(m_path, "w+");
		if (m_file == NULL)return -2;
		int ret = m_epoll.Create(1);
		if (ret != 0)return -3;
		m_server = new CSocket();
		if (m_server == NULL)
		{
			Close();
			return -4;
		}
		CSocketParam param("./log/server.sock", (int)SOCK_ISSERVER | SOCK_ISREUSE);
		printf("%s(%d):[%s] thread_start ret=%d\n", __FILE__, __LINE__, __FUNCTION__, param.attr);
		ret = m_server->Init(param);
		printf("%s(%d):[%s] pid= %d  errno:%d  msg:%s  ret =%d  logserver__init\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
		if (ret != 0)
		{
			Close();
			return -5;
		}
		ret = m_epoll.Add(m_server->getInt(), EpollData((void*)m_server), EPOLLIN | EPOLLERR);
		printf("%s(%d):[%s] epoll_ADD ret=%d  m_server=%d\n", __FILE__, __LINE__, __FUNCTION__, ret, m_server->getInt());
		if (ret != 0)
		{
			Close();
			return -6;
		}
		ret = m_thread.Start();
		printf("%s(%d):[%s] thread_start ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
		if (ret != 0)
		{
			printf("%s(%d):[%s] pid= %d  errno:%d  msg:%s  ret =%d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
			Close();
			return -7;
		}
		return 0;
	}
	
	int Close()
	{
		if (m_server != NULL)
		{
			CSocketBase* p = m_server;
			m_server = NULL;
			delete p;
		}
		m_epoll.Close();
		m_thread.Stop();
		return 0;
	}
	//给其他非日志进程的进程和线程使用的
	static void Trace(const LogInfo& info)
	{
		int ret = 0;
		static thread_local CSocket client;
		if (client == -1)
		{
			
			ret = client.Init(CSocketParam("./log/server.sock", 0));

			if (ret != 0)
			{
				printf("%s(%d):[%s] ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
#ifdef _DEBUG
				printf("%s(%d):[%s] ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
#endif // _DEBUG
				return;
			}
			printf("%s(%d):[%s] ret=%d  client=%d\n", __FILE__, __LINE__, __FUNCTION__, ret, (int)client);
			ret = client.Link();
			printf("%s(%d):[%s] Lret=%d  client=%d\n", __FILE__, __LINE__, __FUNCTION__, ret, (int)client);
			if (ret != 0)printf("errno:%d msg:%s\n", errno, strerror(errno));
		}
		ret = client.Send(info);
		printf("%s(%d):[%s] ret=%d  client=%d  \n", __FILE__, __LINE__, __FUNCTION__, ret,(int)client);
	}
	static Buffer GetTimeStr()
	{
		Buffer result(128);
		timeb tmb;
		ftime(&tmb);
		tm* pTm = localtime(&tmb.time);
		int nSize = snprintf(result, result.size(),
			"%04d-%02d-%02d__%02d-%02d-%02d.%03d",
			pTm->tm_year + 1900, pTm->tm_mon + 1, pTm->tm_mday,
			pTm->tm_hour, pTm->tm_min, pTm->tm_sec, tmb.millitm);
		result.resize(nSize);
		return result;
	}
private:
	int ThreadFunc()
	{
		printf("%s(%d):[%s] m_thread.isValid()=%d m_epoll=%d m_server=%p pid=%d\n", __FILE__, __LINE__, __FUNCTION__, m_thread.isValid(), (int)m_epoll, m_server,getpid());
		EPEvents events;
		
		while (m_thread.isValid() && m_epoll != -1 && m_server != NULL)
		{
			//printf("%s(%d):[%s] m_thread.isValid()=%d  m_epoll=%d  m_server=%p\n", __FILE__, __LINE__, __FUNCTION__, m_thread.isValid(), (int)m_epoll, m_server);
			ssize_t ret = m_epoll.WaitEvents(events,-1);
			//printf("%s(%d):[%s] ret=%d  events.size=%d\n", __FILE__, __LINE__, __FUNCTION__, ret,events.size());
			if (ret < 0)
			{
				printf("%s(%d):[%s] ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
				break;
			}
			if (ret > 0)
			{
				printf("%s(%d):[%s] ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
				ssize_t i = 0;
				for (; i < ret; i++)
				{
					if (events[i].events & EPOLLERR)
					{
						printf("%s(%d):[%s] ret=%d  epoll事件发生 EPOLLERR\n", __FILE__, __LINE__, __FUNCTION__, ret);
						break;
					}
					else if (events[i].events & EPOLLIN)
					{
						if (events[i].data.ptr == m_server)
						{
							CSocketBase* pClient = NULL;

							int r = m_server->Link(&pClient);
							printf("%s(%d):[%s] r=%d \n", __FILE__, __LINE__, __FUNCTION__, r);
							if (r < 0)continue;

							r = m_epoll.Add(pClient->getInt(), EpollData((void*)pClient), EPOLLIN | EPOLLERR);
							printf("%s(%d):[%s] r=%d  pClient->getInt()=%d pClient=%p\n", __FILE__, __LINE__, __FUNCTION__, r,pClient->getInt(),pClient);
							if (r < 0)
							{
								delete pClient;
								continue;
							}
							auto it = mapClients.find(pClient->getInt());
							if (it != mapClients.end())
							{
								if(it->second)delete it->second;
								
							}
							mapClients[pClient->getInt()] = pClient;
						}
						else
						{
							//printf("%s(%d):[%s] data.ptr=%p data.fd=%d\n", __FILE__, __LINE__, __FUNCTION__, events[i].data.ptr,events[i].data.fd);
							CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
							if (pClient != NULL)
							{
								Buffer data(1024 * 1024);
								int r = pClient->Recv(data);
								if (r <= 0)
								{
									mapClients[pClient->getInt()] = NULL;
									delete pClient;
									
								}
								else
								{
									printf("%s(%d):[%s] data=%s \n", __FILE__, __LINE__, __FUNCTION__, (char*)data);
									WriteLog(data);
								}
							}
							
						}
					}
				}
				if (i != ret)
				{
					printf("%s(%d):[%s] i=%d  ret=%d\n", __FILE__, __LINE__, __FUNCTION__, i,ret);
					break;
				}
				printf("%s(%d):[%s] i=%d  ret=%d\n", __FILE__, __LINE__, __FUNCTION__, i, ret);
			}
			printf("%s(%d):[%s] ret=%d\n", __FILE__, __LINE__, __FUNCTION__,ret);
		}
		for (auto it = mapClients.begin(); it != mapClients.end(); it++)
		{
			delete it->second;
		}
		mapClients.clear();
		printf("%s(%d):[%s] pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
		return 0;
	}
	void WriteLog(const Buffer& data)
	{
		if (m_file != NULL)
		{
			FILE* pFile = m_file;
			fwrite((char*)data, 1, data.size(), pFile);
			fflush(pFile);
#ifdef _DEBUG
			printf("%s", (char*)data);
#endif // DEBUG
		}
		
	}
private:
	CThread m_thread;
	CEpoll m_epoll;
	CSocketBase* m_server;
	Buffer m_path;
	FILE* m_file;
	std::map<int, CSocketBase*> mapClients;
};

#ifndef TRACE
#define TRACEI(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_INFO,__VA_ARGS__))
#define TRACED(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_DEBUG,__VA_ARGS__))
#define TRACEW(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_WARNING,__VA_ARGS__))
#define TRACEE(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_ERROR,__VA_ARGS__))
#define TRACEF(...) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_FATIAL,__VA_ARGS__))

//LOGI<<""<<
#define LOGI LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_INFO)
#define LOGD LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_DEBUG)
#define LOGW LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_WARNING)
#define LOGE LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_ERROR)
#define LOGF LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_FATIAL)
//内存导出
//00 01 02 65... ; ...a....
#define DUMPI(data,size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_INFO,data,size))
#define DUMPD(data,size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_DEBUG,data,size))
#define DUMPW(data,size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_WARNING,data,size))
#define DUMPE(data,size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_ERROR,data,size))
#define DUMPF(data,size) CLoggerServer::Trace(LogInfo(__FILE__,__LINE__,__FUNCTION__,getpid(),pthread_self(),LOG_FATIAL,data,size))
#endif // !TRACE

