#pragma once
#include"Public.h"
#include"DatabaseHelper.h"
#include<mysql/mysql.h>

class CMysqlClient : public CDatabaseClient
{
public:
	CMysqlClient(const CMysqlClient&) = delete;
	CMysqlClient& operator=(const CMysqlClient&) = delete;
public:
	CMysqlClient()
	{
		bzero(&m_db, sizeof(m_db));
		m_bInit = false;
	}
	virtual ~CMysqlClient()
	{
		Close();
	}
public:
	virtual int Connect(const KeyValue& args);
	virtual int Exec(const Buffer& sql);
	//带结果的执行
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table);
	//开启事务
	virtual int StartTransaction();
	//提交事务
	virtual int CommitTransaction();
	//回滚事务
	virtual int RollbackTransaction();
	//关闭连接
	virtual int Close();
	//是否连接
	virtual bool IsConnected();
private:
	MYSQL m_db;
	//默认false 表示没有初始化 初始化之后为true 表示已连接
	bool m_bInit;
private:
	class ExecParam
	{
	public:
		ExecParam(CMysqlClient* obj, Result& result, const _Table_& table)
			:obj(obj), result(result), table(table)
		{}
		CMysqlClient* obj;
		Result& result;
		const _Table_& table;
	};
};

class _mysql_table : public _Table_
{
public:
	_mysql_table() :_Table_() {}
	_mysql_table(const _mysql_table& table);
	virtual ~_mysql_table() {}
public:
	//返回创建的SQL语句
	virtual Buffer Create();
	virtual Buffer Drop();
	//增删改查
	virtual Buffer Insert(const _Table_& values);//TODO:参数进行优化
	virtual Buffer Delete(const _Table_& values);
	virtual Buffer Modify(const _Table_& values);//TODO:参数进行优化
	virtual Buffer Query(const Buffer& condition = "");
	//创建一个基于表的对象
	virtual PTable Copy()const;
	virtual void ClearFieldUsed();
public:
	//获取表的全名
	virtual operator const Buffer() const;
private:

};



class _mysql_field : public _Field_
{
public:
	_mysql_field() :_Field_()
	{
		nType = TYPE_NULL;
		Value.Double_ = 0.0;
	}
	_mysql_field(
		int ntype,
		const Buffer& name,
		unsigned attr,
		const Buffer& type,
		const Buffer& size,
		const Buffer& default_,
		const Buffer& check);
	virtual ~_mysql_field();
	_mysql_field(const _mysql_field& field);
	virtual Buffer Create();
	virtual void LoadFromStr(const Buffer& str);
	//where 语句使用的
	virtual Buffer toEqualExp() const;
	virtual Buffer toSqlStr() const;
	//列的全名
	virtual operator const Buffer() const;
private:
	Buffer Str2Hex(const Buffer& data) const;
};


#define DECLARE_TABLE_CLASS(name,base) class name : public base{\
public:\
virtual PTable Copy() const {return PTable(new name(*this));}\
name():base(){Name=#name;

#define DECLARE_MYSQL_FIELD(ntype,name,attr,type,size,default_,check)\
{PField field(new _mysql_field(ntype, #name,attr, type, size, default_, check));FieldDefine.push_back(field);\
Fields[#name] = field; }

#define DECLARE_TABLE_CLASS_END() }};
