#pragma once
#include<unistd.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string>
#include<fcntl.h>
#include"Public.h"

enum SOCK_ATTR
{
	SOCK_ISSERVER = 1,//是否服务器 1表示是 0表示客户端
	SOCK_ISNONBLOCK = 2, //是否阻塞  1表示非阻塞  0表示阻塞
	SOCK_ISUDP = 4,//是否为udp 1表示为udp 0表示为tcp
	SOCK_ISIP = 8, //是否为ip协议 1表示是 0表示本地套接字
	SOCK_ISREUSE = 16,//是否重用地址
};



class CSocketParam
{
public:
	CSocketParam()
	{
		bzero(&addr_in, sizeof(addr_in));
		bzero(&addr_un, sizeof(addr_un));
		port = -1;
		attr = 0;//默认客户端 tcp 阻塞
	}
	CSocketParam(const Buffer& ip,short port,int attr) 
	{
		this->ip = ip;
		this->port = port;
		this->attr = attr;
		addr_in.sin_family = AF_INET;
		addr_in.sin_port = htons(port);//h host主机 n net网络  s就是short 主机字节序转为网络字节序
		addr_in.sin_addr.s_addr = inet_addr(ip);

	}
	CSocketParam(const sockaddr_in* addrin, int attr)
	{
		this->attr = attr;
		memcpy(&addr_in, addrin, sizeof(addr_in));

	}
	CSocketParam(const Buffer& path,int attr) 
	{
		ip = path;
		addr_un.sun_family = AF_UNIX;
		strcpy(addr_un.sun_path, path);
		this->attr = attr;
	}
	CSocketParam(const CSocketParam& param)
	{
		ip = param.ip;
		port = param.port;
		attr = param.attr;
		memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
		memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
	}
	~CSocketParam(){}
public:
	CSocketParam& operator=(const CSocketParam& param)
	{
		if (this != &param)
		{
			ip = param.ip;
			port = param.port;
			attr = param.attr;
			memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
			memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
		}
		return *this;
	}
	sockaddr* addrin() { return (sockaddr*)&addr_in; }
	sockaddr* addrun() { return (sockaddr*)&addr_un; }
public:
	//地址
	sockaddr_in addr_in;
	sockaddr_un addr_un;
	//ip
	Buffer ip;
	short port;
	//参考 SOCK_ATTR
	int attr;
};


class CSocketBase
{
public:
	CSocketBase()
	{
		m_socket = -1;
		m_status = 0;
	}
	virtual ~CSocketBase()
	{
		Close();
	}
public:
	//初始化服务器 套接字创建、bind、listen 客户端套接字创建
	virtual int Init(const CSocketParam & param) = 0;
	//连接服务器 accept 客户端 connect  对于UDP这里可以忽略
	virtual int Link(CSocketBase** pClient = NULL) = 0;
	virtual int Send(const Buffer& data) = 0;
	virtual int Recv(Buffer& data) = 0;
	virtual int Close()
	{
		m_status = 3;
		if (m_socket != -1)
		{
			if ((m_param.attr & SOCK_ISSERVER) && (m_param.attr & SOCK_ISIP) == 0)
				unlink(m_param.ip);
			int fd = m_socket;
			m_socket = -1;
			close(fd);
		}
		return 0;
	}
	virtual operator int() { return m_socket; }
	virtual operator int()const { return m_socket; }
	virtual int getInt() { return m_socket; }
	virtual operator const sockaddr_in* ()const { return &m_param.addr_in; }
	virtual operator  sockaddr_in* () { return &m_param.addr_in; }
protected:
	//套接字描述符 默认为-1
	int m_socket;
	//状态 0未初始化 1初始化 2连接完成  3已经关闭
	int m_status;
	//初始化参数
	CSocketParam m_param;
};

class CSocket : public CSocketBase
{
public:
	CSocket() : CSocketBase(){}
	CSocket(int socket) : CSocketBase() 
	{
		m_socket = socket;
		//m_status = 2;
	}
	~CSocket() { Close(); }
public:
	virtual int Init(const CSocketParam& param)
	{
		if ( m_status != 0)
		{
			return -1;
		}
		m_param = param;
		int type = (m_param.attr & SOCK_ISUDP) ? SOCK_DGRAM : SOCK_STREAM;
		
		if (m_socket == -1)
		{
			if(m_param.attr & SOCK_ISIP)
				m_socket = socket(PF_INET, type, 0);
			else m_socket = socket(PF_LOCAL, type, 0);	
		}
		else
		{
			m_status = 2;//已经处于连接状态
			
		}
		if (m_socket == -1)
		{
			return -2;
		}
		int ret = 0;
		printf("%s(%d):[%s] pid=%d type=%d  m_socket=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), m_param.attr & SOCK_ISUDP,m_socket);
		if (m_param.attr & SOCK_ISSERVER)
		{
			//struct sockaddr un = *((sockaddr*)&param.addr_un);
			if (m_param.attr & SOCK_ISIP)
				ret = bind(m_socket, m_param.addrin(), sizeof(sockaddr_in));
			else ret = bind(m_socket, m_param.addrun(), sizeof(sockaddr_un));
			if (ret == -1)
			{
				return -3;
			}
			
			ret = listen(m_socket, 32);
			if (ret == -1)
			{
				return -4;
			}
			
		}
		if (m_param.attr & SOCK_ISNONBLOCK)
		{
			int option = fcntl(m_socket, F_GETFL);
			printf("%s(%d):[%s] option=%d\n", __FILE__, __LINE__, __FUNCTION__, option);
			if (option == -1)
			{
				return -5;
			}
			option |= O_NONBLOCK;
			ret = fcntl(m_socket, F_SETFL, option);
			if (ret == -1)return -6;
		}
		if (m_param.attr & SOCK_ISREUSE)
		{
			int option = 1;
			ret = setsockopt(m_socket,SOL_SOCKET ,SO_REUSEADDR, &option, sizeof(option));
			if (ret == -1)return -7;
		}
		if (m_status == 0)m_status = 1;
		printf("%s(%d):[%s] attr=%d  m_status=%d m_socket=%d\n", __FILE__, __LINE__, __FUNCTION__, m_param.attr & SOCK_ISSERVER ,m_status,m_socket);
		return 0;
	}
	//连接服务器 accept 客户端 connect  对于UDP这里可以忽略
	virtual int Link(CSocketBase** pClient = NULL)
	{
		if (m_status <= 0 || m_socket == -1)
		{
			return -1;
		}
		int ret = 0;
		printf("%s(%d):[%s] Lunk m_param.attr & SOCK_ISSERVER = %d\n", __FILE__, __LINE__, __FUNCTION__, m_param.attr & SOCK_ISSERVER);
		if (m_param.attr & SOCK_ISSERVER)
		{
			if (pClient == NULL)return -2;
			CSocketParam param;
			int client_fd = -1;
			socklen_t len = 0;
			if (m_param.attr & SOCK_ISIP)
			{
				param.attr |= SOCK_ISIP;
				len = sizeof(sockaddr_in);
				client_fd = accept(m_socket, param.addrin(), &len);
				
			}
			else
			{
				len = sizeof(sockaddr_un);
				client_fd = accept(m_socket, param.addrun(), &len);
			}
			if (client_fd == -1)return -3;
			*pClient = new CSocket(client_fd);
			if (*pClient == NULL)return -4;
			ret = (*pClient)->Init(param);
			if (ret != 0)
			{
				delete (*pClient);
				*pClient = NULL;
				return -5;
			}
			printf("%s(%d):[%s] Lunk  client_fd= %d  param.ip=%s\n", __FILE__, __LINE__, __FUNCTION__, client_fd,(char*)param.ip);
		}
		else
		{
			if (m_param.attr & SOCK_ISIP)
				ret = connect(m_socket, m_param.addrin(), sizeof(sockaddr_in));
			else ret = connect(m_socket, m_param.addrun(), sizeof(sockaddr_un));
			printf("%s(%d):[%s] connect ret = %d\n", __FILE__, __LINE__, __FUNCTION__, ret);
			if (ret != 0)return -6;
		}
		m_status = 2;
		return 0;
	}
	virtual int Send(const Buffer& data)
	{
		if (m_status < 2 || m_socket == -1)return -1;
		ssize_t index = 0;
		while (index < (ssize_t)data.size())
		{
			ssize_t len = write(m_socket, (char*)data + index, data.size() - index);
			if (len == 0)return -2;
			if (len < 0)return -3;
			index += len;
		}
		return 0;
	}
	//大于0表示接收成功 小于0表示失败
	virtual int Recv(Buffer& data)
	{
		if (m_status < 2 || m_socket == -1)return -1;
		data.resize(1024 * 1024);
		ssize_t len = read(m_socket, data, data.size());
		if (len > 0)
		{
			data.resize(len);
			return (int)len;//成功收到数据
		}
		data.clear();
		if (len < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
			{
				data.clear();
				return 0;//没有数据收到
			}
			return -2;//发送错误
		}
		return -3;//套接字被关闭
	}
	virtual int Close() 
	{
		return CSocketBase::Close();
	}
private:

};
