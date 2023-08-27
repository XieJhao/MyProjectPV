#include "MysqlClient.h"
#include<sstream>
#include"Logger.h"

int CMysqlClient::Connect(const KeyValue& args)
{
    if (m_bInit)return -1;
    MYSQL* ret = mysql_init(&m_db);
    if (ret == NULL)return -2;
    ret = mysql_real_connect(&m_db,
        args.at("host"), args.at("user"), args.at("password"), args.at("db"), atoi(args.at("port")), NULL, 0);
    unsigned int err = mysql_errno(&m_db);
    if (ret == NULL && err != 0)
    {
        printf("%s %d %s\n",__FUNCTION__, err, mysql_error(&m_db));
        TRACEE("%d %s\n", err,mysql_error(&m_db));
        mysql_close(&m_db);
        bzero(&m_db, sizeof(m_db));
        return -3;
    }
    m_bInit = true;
    if (args.at("db").size() > 0)
    {
        int r = Exec("use " + args.at("db") + " ;");
        if (r != 0)return -4;
    }
    return 0;
}

int CMysqlClient::Exec(const Buffer& sql)
{
    printf("Exec sql=%s", (char*)sql);
    if (!m_bInit)return -1;
    int ret = mysql_real_query(&m_db, sql, sql.size());
    if (ret != 0)
    {
        printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
        TRACEE("%d %s\n", mysql_errno(&m_db), mysql_error(&m_db));
        return -2;
    }
    return 0;
}

int CMysqlClient::Exec(const Buffer& sql, Result& result, const _Table_& table)
{
    if (!m_bInit)return -1;
    int ret = mysql_real_query(&m_db, sql, sql.size());
    if (ret != 0)
    {
        printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
        TRACEE("%d %s\n", mysql_errno(&m_db), mysql_error(&m_db));
        return -2;
    }
    MYSQL_RES* res = mysql_store_result(&m_db);
    MYSQL_ROW row;
    unsigned num_fields = mysql_num_fields(res);
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        PTable pt = table.Copy();
        for (unsigned i = 0; i < num_fields; i++)
        {
            if (row[i] != NULL)
            {
                pt->FieldDefine[i]->LoadFromStr(row[i]);
            }
        }
        result.push_back(pt);
    }
    return 0;
}

int CMysqlClient::StartTransaction()
{
    if (!m_bInit)return -1;
    int ret = mysql_real_query(&m_db, "BEGIN", 6);
    if (ret != 0)
    {
        printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
        TRACEE("%d %s\n", mysql_errno(&m_db), mysql_error(&m_db));
        return -2;
    }
    return 0;
}

int CMysqlClient::CommitTransaction()
{
    if (!m_bInit)return -1;
    int ret = mysql_real_query(&m_db, "COMMIT", 7);
    if (ret != 0)
    {
        printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
        TRACEE("%d %s\n", mysql_errno(&m_db), mysql_error(&m_db));
        return -2;
    }
    return 0;
}

int CMysqlClient::RollbackTransaction()
{
    if (!m_bInit)return -1;
    int ret = mysql_real_query(&m_db, "ROLLBACK", 9);
    if (ret != 0)
    {
        printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
        TRACEE("%d %s\n", mysql_errno(&m_db), mysql_error(&m_db));
        return -2;
    }
    return 0;
}

int CMysqlClient::Close()
{
    if (m_bInit)
    {
        m_bInit = false;
        mysql_close(&m_db);
        bzero(&m_db, sizeof(m_db));
    }
    return 0;
}

bool CMysqlClient::IsConnected()
{
    return m_bInit;
}

_mysql_table::_mysql_table(const _mysql_table& table)
{
    Database = table.Database;
    Name = table.Name;
    for (unsigned i = 0; i < table.FieldDefine.size(); i++)
    {
        PField filed = PField(new _mysql_field(*(_mysql_field*)table.FieldDefine[i].get()));
        FieldDefine.push_back(filed);
        Fields[filed->Name] = filed;
    }
}

Buffer _mysql_table::Create()
{
    //CREATE TABLE IF NOT EXISTS 表全名(列定义...),PRIMARY KEY '主键列名',UNIQUE INDEX  '列名_UNIAUE' (列名ASC) VISBLE）；
    Buffer sql = "CREATE TABLE IF NOT EXISTS " + (Buffer)*this + " (\r\n";
    for (size_t i = 0; i < FieldDefine.size(); i++)
    {
        if (i > 0)sql += ",\r\n";
        sql += FieldDefine[i]->Create();
        if (FieldDefine[i]->Attr & PRIMARY_KEY)
        {
            sql += ",\r\n PRIMARY KEY (`" + FieldDefine[i]->Name + "`)";
        }
        if (FieldDefine[i]->Attr & UNIQUE)
        {
            sql += ",\r\n UNIQUE INDEX '" + FieldDefine[i]->Name + "_UNIQUE' (";
            sql += (Buffer)*FieldDefine[i] + " ASC) VISBLE";
        }
    }
    sql += ");";
    printf("sql= %s\n", (char*)sql);
    return sql;
}

Buffer _mysql_table::Drop()
{
    return "DROP TABLE " + (Buffer)*this;
}

Buffer _mysql_table::Insert(const _Table_& values)
{
    //INSERT INFO 表全名 (列名...)VALUES(值....);
    Buffer sql = "INSERT INTO " + (Buffer)*this + " (";
    bool isfrist = true;
    for (size_t i = 0; i < values.FieldDefine.size(); i++)
    {
        if (values.FieldDefine[i]->Condition & SQL_INSERT)
        {
            if (!isfrist)sql += ",";
            else isfrist = false;
            sql += (Buffer)*values.FieldDefine[i];
        }
    }
    sql += ") VALUES (";
    isfrist = true;
    for (size_t i = 0; i < values.FieldDefine.size(); i++)
    {
        if (values.FieldDefine[i]->Condition & SQL_INSERT)
        {
            if (!isfrist)sql += ",";
            else isfrist = false;
            sql += values.FieldDefine[i]->toSqlStr();
        }
    }
    sql += " )";
    printf("sql = %s\n", (char*)sql);

    return sql;
}

Buffer _mysql_table::Delete(const _Table_& values)
{
    Buffer sql = "DELETE FROM " + (Buffer)*this + " ";
    Buffer Where = "";
    bool isfrist = true;
    for (unsigned i = 0; i < FieldDefine.size(); i++)
    {
        if (FieldDefine[i]->Condition & SQL_CONDITION)
        {
            if (!isfrist)Where += " AND ";
            else isfrist = false;
            Where += (Buffer)*FieldDefine[i] + "=" + FieldDefine[i]->toSqlStr();
        }
    }
    if (Where.size() > 0)
    {
        sql += " WHERE " + Where;
    }
    sql += ";";
    printf("sql = %s\r\n", (char*)sql);
    return sql;
}

Buffer _mysql_table::Modify(const _Table_& values)
{
    Buffer sql = "UPDATE " + (Buffer)*this + " SET ";
    bool isfrist = true;
    for (unsigned i = 0; i < values.FieldDefine.size(); i++)
    {
        if (values.FieldDefine[i]->Condition & SQL_MODIFY)
        {
            if (!isfrist)sql += ",";
            else isfrist = false;
            sql += (Buffer)*values.FieldDefine[i] + "=" + values.FieldDefine[i]->toSqlStr();
        }
    }
    Buffer Where = "";
    for (unsigned i = 0; i < values.FieldDefine.size(); i++)
    {
        if (values.FieldDefine[i]->Condition & SQL_CONDITION)
        {
            if (!isfrist)Where += " AND ";
            else isfrist = false;
            Where += (Buffer)*values.FieldDefine[i] + "=" + values.FieldDefine[i]->toSqlStr();
        }
    }
    if (Where.size() > 0)
    {
        sql += " WHERE " + Where;
    }
    sql += " ;";
    printf("sql = %s\r\n", (char*)sql);

    return sql;
}

Buffer _mysql_table::Query(const Buffer& condition)
{
    Buffer sql = "SELECT";
    for (size_t i = 0; i < FieldDefine.size(); i++)
    {
        if (i > 0)sql += ',';
        sql += '`' + FieldDefine[i]->Name + '`';
    }
    sql += "  FROM  " + (Buffer)*this + " ";
    if (condition.size() > 0)
    {
        sql += " WHERE " + condition;
    }
    sql += ";";
    printf("sql = %s\r\n", (char*)sql);

    return sql;
}

PTable _mysql_table::Copy() const
{
    return PTable(new _mysql_table(*this));
}

void _mysql_table::ClearFieldUsed()
{
    for (size_t i = 0; i < FieldDefine.size(); i++)
    {
        FieldDefine[i]->Condition = 0;
    }
}

_mysql_table::operator const Buffer() const
{
    Buffer Head;
    if (Database.size())
    {
        Head = '`' + Database + "`.";
    }
    return Head + "`" + Name + "`";
}

_mysql_field::_mysql_field(int ntype, const Buffer& name, unsigned attr, const Buffer& type, const Buffer& size, const Buffer& default_, const Buffer& check)
{
    nType = ntype;
    switch (ntype)
    {
    case TYPE_VARCHAR:
    case TYPE_TEXT:
    case TYPE_BLOB:
        Value.String_ = new Buffer();
        break;
    }
    Name = name;
    Attr = attr;
    Type = type;
    Size = size;
    Default = default_;
    Check = check;
}

_mysql_field::~_mysql_field()
{
    switch (nType)
    {
    case TYPE_VARCHAR:
    case TYPE_TEXT:
    case TYPE_BLOB:
        if (Value.String_)
        {
            Buffer* p = Value.String_;
            Value.String_ = nullptr;
            delete p;
        }
        break;
    }
}

_mysql_field::_mysql_field(const _mysql_field& field)
{
    nType = field.nType;
    switch (nType)
    {
    case TYPE_VARCHAR:
    case TYPE_TEXT:
    case TYPE_BLOB:
        Value.String_ = new Buffer();
        *Value.String_ = *field.Value.String_;
        break;
    }
    Name = field.Name;
    Attr = field.Attr;
    Type = field.Type;
    Size = field.Size;
    Default = field.Default;
    Check = field.Check;
}

Buffer _mysql_field::Create()
{
    Buffer sql = "`" + Name + "` " + Type + Size + " ";
    if (Attr & NOT_NULL)
    {
        sql += " NOT NULL ";
    }
    else
    {
        sql += " NULL ";
    }
    //BLOB TEXT GEOMETRY JSON 等类型不能有默认值
    if (Type == "DATETIME")
    {
        sql += " DEFAULT " + Default + " ";
    }
    else if ((Attr & DEFAULT) && (Default.size() > 0) && 
        (Type != "BLOB") && (Type != "TEXT") && (Type != "GEOMETRY") && (Type != "JSON"))
    {
        sql += " DEFAULT \"" + Default + "\" ";
    }
    //UNIQUE PRIMARY_KEY 外面处理
    //CHECK mysql 不支持
    if (Attr & AUTOINCREMENT)
    {
        sql += " AUTO_INCREMENT ";
    }
    return sql;
}

void _mysql_field::LoadFromStr(const Buffer& str)
{
    switch (nType)
    {
    case TYPE_NULL:
        break;
    case TYPE_BOOL:
    case TYPE_INT:
    case TYPE_DATETIME:
        Value.Integer_ = atoi(str);
        break;
    case TYPE_REAL:
        Value.Double_ = atof(str);
        break;
    case TYPE_VARCHAR:
    case TYPE_TEXT:
        *Value.String_ = str;
        break;
    case TYPE_BLOB:
        *Value.String_ = Str2Hex(str);
        break;
    default:
        printf("type=%d\n", nType);
        break;
    }
}

Buffer _mysql_field::toEqualExp() const
{
    Buffer sql = (Buffer)*this + " = ";
    std::stringstream ss;
    switch (nType)
    {
    case TYPE_NULL:
        sql += " NULL ";
        break;
    case TYPE_BOOL:
    case TYPE_INT:
    case TYPE_DATETIME:
        ss << Value.Integer_;
        sql += ss.str() + " ";
        break;
    case TYPE_REAL:
        ss << Value.Double_;
        sql += ss.str() + " ";
        break;
    case TYPE_VARCHAR:
    case TYPE_TEXT:
    case TYPE_BLOB:
        sql += '"' + *Value.String_ + "\" ";
        break;
    default:
        printf("type=%d", nType);
        break;
    }
    return sql;
}

Buffer _mysql_field::toSqlStr() const
{
    Buffer sql = "";
    std::stringstream ss;
    switch (nType)
    {
    case TYPE_NULL:
        sql += " NULL ";
        break;
    case TYPE_BOOL:
    case TYPE_INT:
    case TYPE_DATETIME:
        ss << Value.Integer_;
        sql += ss.str() + " ";
        break;
    case TYPE_REAL:
        ss << Value.Double_;
        sql += ss.str() + " ";
        break;
    case TYPE_VARCHAR:
    case TYPE_TEXT:
    case TYPE_BLOB:
        sql += '"' + *Value.String_ + "\" ";
        break;
    default:
        printf("type=%d", nType);
        break;
    }
    return sql;
}

_mysql_field::operator const Buffer() const
{
    return '`' + Name + '`';
}

Buffer _mysql_field::Str2Hex(const Buffer& data) const
{
    const char* hex = "0123456789ABCDEF";
    std::stringstream ss;
    for (auto ch : data)
    {
        ss << hex[(unsigned char)ch >> 4] << hex[(unsigned char)ch & 0xF];
    }
    return ss.str();
}
