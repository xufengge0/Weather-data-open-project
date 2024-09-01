#include"_tools.h"

// 获取表的全部字段
bool ctcols::allcols(connection &conn,char* tablename)
{
    m_allcols.clear(); m_vallcols.clear();

    sqlstatement stmt(&conn);
    // 从数据字典user_tab_columns查找，表名为tablename的column_name,data_type,data_length列
    stmt.prepare("select lower(column_name),lower(data_type),data_length from user_tab_columns\
                  where table_name=upper(:1) order by column_id"); // 表名大写、数据字段小写
    stmt.bindin(1,tablename);
    stmt.bindout(1,stcolumns.colname,30);
    stmt.bindout(2,stcolumns.datatype,30);
    stmt.bindout(3,stcolumns.collen);
    if(stmt.execute()!=0) return false;

    while (1) // 每循环一次获取一个字段的列信息
    {
        memset(&stcolumns,0,sizeof(st_columns));
        if(stmt.next()!=0) break;

        // 列的数据类型分为number、char、date
        // 如果业务需要可修改下面代码
        if(strcmp(stcolumns.datatype,"char")==0)        strcpy(stcolumns.datatype,"char");
        if(strcmp(stcolumns.datatype,"varchar2")==0)    strcpy(stcolumns.datatype,"char");
        if(strcmp(stcolumns.datatype,"nchar")==0)       strcpy(stcolumns.datatype,"char");
        if(strcmp(stcolumns.datatype,"nvarchar2")==0)   strcpy(stcolumns.datatype,"char");
        if(strcmp(stcolumns.datatype,"rowid")==0)       {strcpy(stcolumns.datatype,"char"); stcolumns.collen=18;}

        if(strcmp(stcolumns.datatype,"date")==0)        stcolumns.collen=14;

        if(strcmp(stcolumns.datatype,"number")==0)      strcpy(stcolumns.datatype,"number");
        if(strcmp(stcolumns.datatype,"integer")==0)     strcpy(stcolumns.datatype,"number");
        if(strcmp(stcolumns.datatype,"float")==0)       strcpy(stcolumns.datatype,"number");
        if(strcmp(stcolumns.datatype,"number")==0)      stcolumns.collen=22;

        // 不属于上面类型，不处理
        if((strcmp(stcolumns.datatype,"char")!=0) &&
           (strcmp(stcolumns.datatype,"number")!=0) &&
           (strcmp(stcolumns.datatype,"date")!=0))
           continue;

        m_allcols = m_allcols + stcolumns.colname +","; // 拼接字段 obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid,
        m_vallcols.push_back(stcolumns);
 
        
    }
    if(stmt.rpc()>0) deleterchr(m_allcols,','); // 删除最后一个','

    return true;
}
// 获取表的全部主键
bool ctcols::pkcols(connection &conn,char* tablename)
{
    m_pkcols.clear(); m_vpkcols.clear();

    sqlstatement stmt(&conn);
    // 从USER_CONS_COLUMNS和USER_CONSTRAINTS字典获取表的主键
    stmt.prepare("select lower(column_name),position from USER_CONS_COLUMNS\
                where table_name=upper(:1)\
                and constraint_name=(select constraint_name from USER_CONSTRAINTS\
                            where table_name=upper(:2) and constraint_type='P' and generated='USER NAME')\
                order by position");

    stmt.bindin(1,tablename,30);
    stmt.bindin(2,tablename,30);
    stmt.bindout(1,stcolumns.colname,30);
    stmt.bindout(2,stcolumns.pkseq);

    if(stmt.execute()!=0) return false;

    while (1) // 每循环一次，获取一个主键信息 
    {
        memset(&stcolumns,0,sizeof(st_columns));
        if(stmt.next()!=0) break;

        m_pkcols = m_pkcols + stcolumns.colname + ",";
        m_vpkcols.push_back(stcolumns);
    }
    if(stmt.rpc()>0) deleterchr(m_pkcols,','); // 删除最后一个','

    // 把m_vallcols容器中的pkseq字段更新
    for(st_columns &i:m_vallcols)
    {
        for(st_columns &j:m_vpkcols)
        {
            if(strcmp(i.colname,j.colname)==0) // 列名字符串完全一样
            {
                i.pkseq = j.pkseq;
                break;
            }
        }
    }
    
    return true;
}