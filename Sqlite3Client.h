#pragma once
#include"Public.h"
#include"DatabaseHelper.h"
#include"sqlite3/sqlite3.h"

class CSqlite3Client : public CDatabaseClient
{
public:
	CSqlite3Client(const CSqlite3Client&) = delete;
	CSqlite3Client& operator=(const CSqlite3Client&) = delete;
public:
	CSqlite3Client() 
	{
		m_db = NULL;
		m_stmt = NULL;
	}
	virtual ~CSqlite3Client() 
	{
		Close();
	}
public:
	virtual int Connect(const KeyValue& args) ;
	virtual int Exec(const Buffer& sql) ;
	//带结果的执行
	virtual int Exec(const Buffer& sql, Result& result, const _Table_& table) ;
	//开启事务
	virtual int StartTransaction();
	//提交事务
	virtual int CommitTransaction();
	//回滚事务
	virtual int RollbackTransaction();
	//关闭连接
	virtual int Close();
	//是否连接
	virtual bool IsConnected() ;
private:
	static int ExecCallback(void* arg, int count, char** names, char** values);
	int ExecCallback(Result&result,const _Table_& table, int count, char** names, char** values);
private:
	sqlite3_stmt* m_stmt;
	sqlite3* m_db;
	class ExecParam
	{
	public:
		ExecParam(CSqlite3Client* obj, Result& result, const _Table_& table)
			:obj(obj),result(result),table(table)
		{}
		CSqlite3Client* obj;
		Result& result;
		const _Table_& table;
	};
};

class _sqlite3_table : public _Table_
{
public:
	_sqlite3_table():_Table_(){}
	_sqlite3_table(const _sqlite3_table& table);
	virtual ~_sqlite3_table(){}
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
	virtual operator const Buffer() const ;
private:

};



class _sqlite3_field : public _Field_
{
public:
	_sqlite3_field() :_Field_()
	{
		nType = TYPE_NULL;
		Value.Double_ = 0.0;
	}
	_sqlite3_field(
		int ntype,
		const Buffer& name,
		unsigned attr,
		const Buffer& type,
		const Buffer& size,
		const Buffer& default_,
		const Buffer& check);
	virtual ~_sqlite3_field();
	_sqlite3_field(const _sqlite3_field& field);
	virtual Buffer Create();
	virtual void LoadFromStr(const Buffer& str);
	//where 语句使用的
	virtual Buffer toEqualExp() const;
	virtual Buffer toSqlStr() const;
	//列的全名
	virtual operator const Buffer() const ;
private:
	Buffer Str2Hex(const Buffer& data) const;
	
};

#define DECLARE_TABLE_CLASS(name,base) class name : public base{\
public:\
virtual PTable Copy() const {return PTable(new name(*this));}\
name():base(){Name=#name;


#define DECLARE_FIELD(ntype,name,attr,type,size,default_,check)\
{PField field(new _sqlite3_field(ntype, #name,attr, type, size, default_, check));FieldDefine.push_back(field);\
Fields[#name] = field; }

#define DECLARE_TABLE_CLASS_END() }};

//class user_test : public _sqlite3_table
//{
//public:
//	user_test() :_sqlite3_table()
//	{
//		Name = "user_test";
//		{
//			PField field(new _sqlite3_field(TYPE_INT, "user_id", 
//				NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INT", "", "", ""));
//			FieldDefine.push_back(field);
//			Fields["user_id"] = field;
//		}
//		{
//			PField field(new _sqlite3_field(TYPE_VARCHAR, "user_qq",
//				NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "VARCHAR", "(15)", "", ""));
//			FieldDefine.push_back(field);
//			Fields["user_qq"] = field;
//		}
//	}
//	virtual PTable Copy() const
//	{
//		return PTable(new user_test(*this));
//	}
//};
