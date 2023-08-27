#pragma once
#include"Logger.h"
#include"CServer.h"
#include"MysqlClient.h"
#include"CThttpParser.h"
#include"Crypto.h"
#include"Jsoncpp/json.h"
#include<map>

DECLARE_TABLE_CLASS(Login_user_mysql, _mysql_table)

DECLARE_MYSQL_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_phone, DEFAULT, "VARCHAR", "(12)", "1888888", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_name, NOT_NULL, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_nick, NOT_NULL, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat_id, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_address, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_province, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_country, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_age, DEFAULT | CHECK, "INTEGER", "", "18", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_male, DEFAULT, "BOOL", "", "1", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_flags, DEFAULT, "TEXT", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_experience, DEFAULT, "REAL", "", "0.0", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_level, DEFAULT | CHECK, "INTEGER", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_class_priority, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_time_per_viewer, DEFAULT, "REAL", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_career, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_password, NOT_NULL, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_brithday, NONE, "INTEGER", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_describe, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_education, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_register_time, DEFAULT, "DATETIME", "", "CURRENT_TIMESTAMP", "")//LOCALTIME()
DECLARE_TABLE_CLASS_END()

/*
* 1.�ͻ��˵ĵ�ַ����
* 2.���ӻص��Ĳ�������
* 3.���ջص��Ĳ�������
*/


#define ERR_RETURN(ret,err) if(ret !=0){TRACEE("ret= %d  errno= %d  msg=[%s]", ret, errno, strerror(errno)); return err;}


#define WARN_RETURN(ret) if(ret !=0){TRACEW("ret= %d  errno= %d  msg=[%s]", ret, errno, strerror(errno));continue;}


class CPlayerServer : public CBusiness
{
public:
	CPlayerServer(unsigned count)
		:CBusiness()
	{
		m_count = count;
		
	}
	~CPlayerServer()
	{
		if (m_db)
		{
			CDatabaseClient* db = m_db;
			m_db = NULL;
			db->Close();
			delete db;
		}
		m_epoll.Close();
		m_pool.Close();
		for (auto it : m_mapClients)
		{
			if (it.second)
			{
				delete it.second;
			}
		}
		m_mapClients.clear();
	}
	virtual int BusinessProcass(CProcass* proc)
	{
		int sock = 0;
		int ret = 0;
		m_db = new CMysqlClient();
		if (m_db == NULL)
		{
			TRACEE("no more memory!");
			return -1;
		}
		KeyValue args;
		args["host"] = "localhost";
		args["user"] = "debian-sys-maint";
		args["password"] = "0ZaRwExruKSQPWuV";
		args["port"] = "3306";
		args["db"] = "testDB";
		ret = m_db->Connect(args);
		ERR_RETURN(ret, -2);
		Login_user_mysql user;
		ret = m_db->Exec(user.Create());
		ERR_RETURN(ret, -3);
		ret = setConnectedCallback(&CPlayerServer::Connected, this, std::placeholders::_1);
		ERR_RETURN(ret, -4);
		ret = setRecvCallback(&CPlayerServer::Received, this, std::placeholders::_1, std::placeholders::_2);
		ERR_RETURN(ret, -5);
		ret = m_epoll.Create(m_count);
		ERR_RETURN(ret, -6);
		ret = m_pool.Start(m_count);
		ERR_RETURN(ret, -7);
		for (unsigned i = 0; i < m_count; i++)
		{
			ret = m_pool.AddTask(&CPlayerServer::ThreadFunc, this);
			ERR_RETURN(ret, -8);
		}
		sockaddr_in addrin;
		while (m_epoll != -1)
		{
			ret = proc->RecvSocket(sock, &addrin);
			TRACEI("RecvSocket ret=%d", ret);
			if (ret < 0 || sock == 0)break;
			CSocketBase* pClient = new CSocket(sock);
			if (pClient == NULL)continue;
			ret = pClient->Init(CSocketParam(&addrin, SOCK_ISIP));
			WARN_RETURN(ret);
			ret = m_epoll.Add(sock, EpollData((void*)pClient));
			if (m_connectedcallback)
			{
				(*m_connectedcallback)(pClient);
			}
			WARN_RETURN(ret);

		}
		return 0;
	}
private:
	int Connected(CSocketBase* pClient)
	{
		//TODO:�ͻ������ӣ��򵥴�ӡ�ͻ�����Ϣ
		sockaddr_in* paddr = *pClient;
		TRACEI("client connectd  addr:%s  port:%d", inet_ntoa(paddr->sin_addr), paddr->sin_port);
		return 0;
	}
	int Received(CSocketBase* pClient, const Buffer& data)
	{
		TRACEI("Received ���յ�����");
		//TODO:��Ҫҵ���ڴ˴���
		//Http����
		int ret = 0;
		Buffer response = "";
		ret = HttpParser(data);
		TRACEI("HttpParser  ret=%d", ret);
		//��֤����ķ���
		if (ret != 0)//��֤ʧ��
		{
			TRACEE("http parser failed!%d", ret);
		}
		response = MakeResponse(ret);
		ret = pClient->Send(response);
		if (ret != 0)
		{
			TRACEE("http response failed!%d [%s]", ret, (char*)response);

		}
		else
		{
			TRACEI("http response success!%d", ret);

		}
		return 0;
	}
	int HttpParser(const Buffer& data)
	{
		CHttpParser parser;
		size_t size = parser.Parser(data);
		if (size == 0 || parser.Errno() != 0)
		{
			TRACEE("size %llu errno:%s", size, parser.Errno());
			return -1;
		}
		if (parser.Method() == HTTP_GET)
		{
			//get ����
			UrlParser url("https://192.168.31.230" + parser.Url());
			int ret = url.Parser();
			if (ret != 0)
			{
				TRACEE("ret = %d  url[%s]", ret, "https://192.168.31.230" + parser.Url());
				return -2;
			}
			Buffer uri = url.Uri();
			TRACEI("*********uri = %s", (char*)uri);
			if (uri == "login")
			{
				//������¼
				Buffer time = url["time"];
				Buffer salt = url["salt"];
				Buffer user = url["user"];
				Buffer sign = url["sign"];
				TRACEI("time:%s salt:%s user:%s sign:%s", (char*)time, (char*)salt, (char*)user, (char*)sign);
				//���ݿ�Ĳ�ѯ
				Login_user_mysql dbuser;
				Result result;
				Buffer sql = dbuser.Query("user_name=\"" + user + "\"");
				ret = m_db->Exec(sql,result,dbuser);
				if (ret != 0)
				{
					TRACEE("sql=%s  ret=%d",sql,ret);
					return -3;
				}
				if (result.size() == 0)
				{
					TRACEE("no result sql=%s  ret=%d", sql, ret);
					return -4;
				}
				if (result.size() != 1)
				{
					TRACEE("more than one sql=%s  ret=%d", sql, ret);
					return -5;
				}
				auto user1 = result.front();
				Buffer pwd = *user1->Fields["user_password"]->Value.String_;
				TRACEE("password = %s", (char*)pwd);
				//��¼������֤
				const char* MD5_KEY = "*&^%$#@b.v+h-b*g/h@n!h#n$d^ssx,.kl<kl";
				Buffer md5str = time + MD5_KEY + pwd + salt;
				Buffer md5 = Crypto::MD5(md5str);
				TRACEE("md5 = %s", (char*)md5);
				if (md5 == sign)
				{
					return 0;
				}
				return -6;
			}
		}
		else if (parser.Method() == HTTP_POST)
		{
			//post ����
		}
		return -7;
	}
	Buffer MakeResponse(int ret)
	{
		Json::Value root;
		root["status"] = ret;
		if (ret != 0)
		{
			root["message"] = "��¼ʧ�ܣ��������û������������";

		}
		else
		{
			root["message"] = "seccress";
		}
		Buffer json = root.toStyledString();
		Buffer result = "HTTP/1.1 200 OK\r\n";
		time_t t;
		time(&t);
		tm* ptm = localtime(&t);
		char temp[64] = "";
		strftime(temp, sizeof(temp), "%a, %d, %b, %G, %T, GMT\r\n", ptm);
		Buffer Date = Buffer("Date: ") + temp;
		Buffer Server = "Server: Edoyun/1.0\r\nContent-Type: text/html; chatset=utf-8\r\nX-Frame-Options: DENY\r\n";
		snprintf(temp, sizeof(temp), "%d", json.size());
		Buffer Length = Buffer("Content-Length: ") + temp + "\r\n";
		Buffer Stub = "X-Content-Type-Options: nosniff\r\nReferrer-Policy: same-origin\r\n\r\n";
		result += Date + Server + Length + Stub + json;
		TRACEI("response: %s", (char*)result);
		return result;
	}
private:
	int ThreadFunc()
	{
		int ret = 0;
		EPEvents events;
		while (m_epoll != -1)
		{
			ssize_t size = m_epoll.WaitEvents(events);
			if (size < 0)break;
			if (size > 0)
			{
				for (ssize_t i = 0; i < size; i++)
				{
					if (events[i].events & EPOLLERR)
					{
						break;
					}
					else if (events[i].events & EPOLLIN)
					{
						CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
						if (pClient)
						{
							Buffer data;
							ret = pClient->Recv(data);
							TRACEI("recv data size=%d", ret);
							if (ret <= 0)
							{ 
								TRACEW("ret= %d  errno= %d  msg=[%s]", ret, errno, strerror(errno)); 
								m_epoll.Del(pClient->getInt());
								continue; 
							}
							if (m_recvcallback)
							{
								(*m_recvcallback)(pClient,data);
							}
						}
					}
				}
			}
		}
		return 0;
	}
private:
	CEpoll m_epoll;
	CThreadPool m_pool;
	std::map<int, CSocketBase*>m_mapClients;
	unsigned m_count;
	CDatabaseClient* m_db;
};