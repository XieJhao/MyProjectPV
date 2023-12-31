#pragma once
#include"http_parser.h"
#include"Public.h"
#include<map>

class CHttpParser
{
public:
	CHttpParser();
	~CHttpParser();
	CHttpParser(const CHttpParser& http);
	CHttpParser& operator=(const CHttpParser& http);
public:
	size_t Parser(const Buffer& data);
	//GET POST ... �ο�http_parser.h HTTP_METHOD_MAP��
	unsigned Method()const { return m_parser.method; }
	const std::map<Buffer, Buffer>& Headers() { return m_HreaderValues; }
	const Buffer& Status()const { return m_status; }
	const Buffer& Url()const { return m_url; }
	const Buffer& Body()const { return m_body; }
	unsigned Errno()const { return m_parser.http_errno; }
protected:
	static int OnMessageBegin(http_parser* parser);
	static int OnUrl(http_parser* parser,const char*at,size_t length);
	static int OnStatus(http_parser* parser, const char* at, size_t length);
	static int OnHeadersField(http_parser* parser, const char* at, size_t length);
	static int OnHeadersValue(http_parser* parser, const char* at, size_t length);
	static int OnHeadersComplete(http_parser* parser);
	static int OnBody(http_parser* parser, const char* at, size_t length);
	static int OnMessageComplete(http_parser* parser);

	int OnMessageBegin();
	int OnUrl( const char* at, size_t length);
	int OnStatus( const char* at, size_t length);
	int OnHeadersField(const char* at, size_t length);
	int OnHeadersValue(const char* at, size_t length);
	int OnHeadersComplete();
	int OnBody(const char* at, size_t length);
	int OnMessageComplete();
private:
	http_parser m_parser;
	http_parser_settings m_settings;
	std::map<Buffer, Buffer> m_HreaderValues;
	Buffer m_status;
	Buffer m_body;
	Buffer m_url;
	bool m_complete;
	Buffer m_lastField;
};


class UrlParser
{
public:
	UrlParser(const Buffer& url);
	~UrlParser(){}
	int Parser();
	Buffer operator[](const Buffer& name);
	Buffer Protocol()const { return m_protocol; }
	Buffer Host()const { return m_host; }
	//Ĭ�Ϸ���80
	int Port()const { return m_port; }
	void SetUrl(const Buffer& url);
	const Buffer Uri()const { return m_uri; }
private:
	Buffer m_url;
	Buffer m_protocol;
	Buffer m_host;
	Buffer m_uri;
	int m_port;
	std::map<Buffer, Buffer>  m_values;
};
