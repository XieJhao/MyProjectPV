#include "Sqlite3Client.h"
#include"Logger.h"

int CSqlite3Client::Connect(const KeyValue& args)
{
    auto it = args.find("host");
    if (it == args.end())return -1;
    if (m_db != NULL)return -2;
    int ret = sqlite3_open(it->second, &m_db);
    if (ret != 0)
    {
        TRACEE("connect faild:%d [%s]", ret, sqlite3_errmsg(m_db));
        return -3;
    }
    return 0;
}

int CSqlite3Client::Exec(const Buffer& sql)
{
    if (m_db == NULL)return -1;
    int ret = sqlite3_exec(m_db, sql, NULL, this, NULL);
    if (ret != SQLITE_OK)
    {
        //TRACEE("sql: {%s}\n", (char*)sql);
        printf("sql: {%s}\n", (char*)sql);
        //TRACEE("Exec faild :%d [%s]\n", ret, sqlite3_errmsg(m_db));
        printf("Exec faild :%d [%s]\n", ret, sqlite3_errmsg(m_db));
        return -2;
    }
    return 0;
}

int CSqlite3Client::Exec(const Buffer& sql, Result& result, const _Table_& table)
{
    printf("sql: {%s}\n", (char*)sql);

    char* errmsg = NULL;
    if (m_db == NULL)return -1;
    ExecParam param(this, result, table);
    //会阻塞
    int ret = sqlite3_exec(m_db, sql, &CSqlite3Client::ExecCallback, (void*)&param, &errmsg);
    if (ret != SQLITE_OK)
    {
        //TRACEE("sql: {%s}\n", (char*)sql);
        printf("sql: {%s}\n", (char*)sql);
        //TRACEE("Exec faild :%d [%s]\n", ret, errmsg);
        printf("Exec faild :%d [%s]\n", ret, errmsg);
        if (errmsg)sqlite3_free(errmsg);
        return -2;
    }
    if (errmsg)sqlite3_free(errmsg);

    return 0;
}

int CSqlite3Client::StartTransaction()
{
    if (m_db == NULL)return -1;
    int ret = sqlite3_exec(m_db, "BEGIN TRANSACTION", 0, 0, NULL);
    if (ret != SQLITE_OK)
    {
        TRACEE("sql: {BEGIN TRANSACTION}");
        TRACEE("BEGIN faild :%d [%s]", ret, sqlite3_errmsg(m_db));
    }
    return 0;
}

int CSqlite3Client::CommitTransaction()
{
    if (m_db == NULL)return -1;
    int ret = sqlite3_exec(m_db, "COMMIT TRANSACTION", 0, 0, NULL);
    if (ret != SQLITE_OK)
    {
        TRACEE("sql: {COMMIT TRANSACTION}");
        TRACEE("COMMIT faild :%d [%s]", ret, sqlite3_errmsg(m_db));
    }
    return 0;
}

int CSqlite3Client::RollbackTransaction()
{
    if (m_db == NULL)return -1;
    int ret = sqlite3_exec(m_db, "ROLLBACK TRANSACTION", 0, 0, NULL);
    if (ret != SQLITE_OK)
    {
        TRACEE("sql: {ROLLBACK TRANSACTION}");
        TRACEE("ROLLBACK faild :%d [%s]", ret, sqlite3_errmsg(m_db));
    }
    return 0;
}

int CSqlite3Client::Close()
{
    if (m_db == NULL)return -1;
    int ret = sqlite3_close(m_db);
    if (ret != SQLITE_OK)
    {
        TRACEE("CLose faild :%d [%s]", ret, sqlite3_errmsg(m_db));
    }
    m_db = NULL;
    return 0;
}

bool CSqlite3Client::IsConnected()
{
    return m_db != NULL;
}

int CSqlite3Client::ExecCallback(void* arg, int count, char** values, char** names)
{
    ExecParam* param = (ExecParam*)arg;

    return param->obj->ExecCallback(param->result,param->table,count,names,values);
}

int CSqlite3Client::ExecCallback(Result& result, const _Table_& table, int count, char** names, char** values)
{
    PTable pTable = table.Copy();
    if (pTable == nullptr)
    {
        printf("table %s error!\n",(const char*)(Buffer)table);
        //TRACEE("table %s error!\n", (const char*)(Buffer)table);
        return -1;
    }
    for (int i = 0; i < count; i++)
    {
        Buffer name = names[i];
        auto it = pTable->Fields.find(name);
        if (it == pTable->Fields.end())
        {
            printf("table %s error!\n", (const char*)(Buffer)table);
            //TRACEE("table %s error!\n", (const char*)(Buffer)table);
            return -2;
        }
        if(values[i] != nullptr)
            it->second->LoadFromStr(values[i]);
    }
    result.push_back(pTable);
    return 0;
}

_sqlite3_table::_sqlite3_table(const _sqlite3_table& table)
{
    Database = table.Database;
    Name = table.Name;
    for (unsigned i = 0; i < table.FieldDefine.size(); i++)
    {
        PField filed = PField(new _sqlite3_field(*(_sqlite3_field*)table.FieldDefine[i].get()));
        FieldDefine.push_back(filed);
        Fields[filed->Name] = filed;
    }
}

Buffer _sqlite3_table::Create()
{
    //CREATE TABLE IF NOT EXISTS 表的全名【数据库.表名】（列的定义,。。）;
    Buffer sql = "CREATE TABLE IF NOT EXISTS" + (Buffer)*this + "(\r\n";
    for (unsigned i = 0; i < FieldDefine.size(); i++)
    {
        if (i > 0)sql += ",";
        sql += FieldDefine[i]->Create();
    }
    sql += ");";
    TRACEI("sql = %s",(char*)sql);
    return sql;
}

Buffer _sqlite3_table::Drop()
{
    Buffer sql;
    sql = "DROP TABLE " + (Buffer)*this + ";";
    TRACEI("sql = %s", (char*)sql);

    return sql;
}

Buffer _sqlite3_table::Insert(const _Table_& values)
{
    Buffer sql = "INSERT INTO " + (Buffer)*this +" (";
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
    TRACEI("sql = %s", (char*)sql);

    return sql;
}

Buffer _sqlite3_table::Delete(const _Table_& values)
{
    Buffer sql = "DELETE FROM " + (Buffer) * this + " ";
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
    TRACEI("sql = %s", (char*)sql);
    return sql;
}

Buffer _sqlite3_table::Modify(const _Table_& values)
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
    TRACEI("sql = %s", (char*)sql);

    return sql;
}

Buffer _sqlite3_table::Query(const Buffer& condition)
{
    Buffer sql = "SELECT";
    for (size_t i = 0; i < FieldDefine.size(); i++)
    {
        if (i > 0)sql += ',';
        sql += '"' + FieldDefine[i]->Name + '\"';
    }
    sql += "  FROM  " + (Buffer)*this + " ";
    if (condition.size() > 0)
    {
        sql += " WHERE " + condition;
    }
    sql += ";";
    TRACEI("sql = %s", (char*)sql);

    return sql;
}

PTable _sqlite3_table::Copy() const
{
    return PTable(new _sqlite3_table(*this));
}

void _sqlite3_table::ClearFieldUsed()
{
    for (unsigned i = 0; i < FieldDefine.size(); i++)
    {
        FieldDefine[i]->Condition = 0;
    }
}

_sqlite3_table::operator const Buffer() const
{
    Buffer Head;
    if (Database.size())
    {
        Head = '"' + Database + "\".";
    }
    return Head + '"' + Name + '"';
}

_sqlite3_field::_sqlite3_field(int ntype, const Buffer& name, unsigned attr, const Buffer& type, const Buffer& size, const Buffer& default_, const Buffer& check)
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

_sqlite3_field::~_sqlite3_field()
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

_sqlite3_field::_sqlite3_field(const _sqlite3_field& field)
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

Buffer _sqlite3_field::Create()
{
    //"名称" 类型 属性
    Buffer sql = '"' + Name +"\" " + Type + "  ";
    if (Attr & NOT_NULL)
    {
        sql += " NOT NULL ";
    }
    if (Attr & DEFAULT)
    {
        sql += " DEFAULT " + Default +" ";
    }
    if (Attr & UNIQUE)
    {
        sql += " UNIQUE ";
    }
    if (Attr & PRIMARY_KEY)
    {
        sql += " PRIMARY KEY ";
    }
    if (Attr & CHECK)
    {
        sql += " CHECK ( "+ Check + ") ";
    }
    if (Attr & AUTOINCREMENT)
    {
        sql += " AUTOINCREMENT ";
    }
    return sql;
}

void _sqlite3_field::LoadFromStr(const Buffer& str)
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
        TRACEW("type=%d", nType);
        break;
    }
}

Buffer _sqlite3_field::toEqualExp() const
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
        sql += ss.str() +" ";
        break;
    case TYPE_REAL:
        ss<< Value.Double_;
        sql += ss.str() + " ";
        break;
    case TYPE_VARCHAR:
    case TYPE_TEXT:
    case TYPE_BLOB:
        sql += '"' + *Value.String_ + "\" ";
        break;
    default:
        TRACEW("type=%d", nType);
        break;
    }
    return sql;
}

Buffer _sqlite3_field::toSqlStr() const
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
        TRACEW("type=%d", nType);
        break;
    }
    return sql;
}

_sqlite3_field::operator const Buffer() const
{
    return '"' + Name + '"';
}

Buffer _sqlite3_field::Str2Hex(const Buffer& data) const
{
    const char* hex = "0123456789ABCDEF";
    std::stringstream ss;
    for (auto ch : data)
    {
        ss << hex[(unsigned char)ch >> 4] << hex[(unsigned char)ch & 0xF];
    }
    return ss.str();
}
