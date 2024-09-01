/* 

 */
#include "_public.h"
#include"_ooci.h"
using namespace idc;

// 获取表全部的列信息和主键的类
class ctcols
{
private:
    // 表的列信息结构体
    struct st_columns
    {
        char colname[31];   // 列名
        char datatype[31];  // 列的数据类型,number、date、char
        int collen;         // 列的长度，number固定22，date固定14，char根据表结构决定
        int pkseq;          // 不是主键 置为0，是主键 从1开始增加

    } stcolumns;

public:
    vector<st_columns> m_vallcols;  // 存放全部字段信息的容器
    vector<st_columns> m_vpkcols;   // 存放全部主键信息的容器

    string m_allcols;   // 存放全部字段名 obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid
    string m_pkcols;    // 存放全部主键名 obtid,ddatetime
    
    bool allcols(connection &conn,char* tablename);     // 获取表的全部字段
    bool pkcols(connection &conn,char* tablename);      // 获取表的全部主键
};
