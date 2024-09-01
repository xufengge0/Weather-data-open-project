/* 
    Oracle数据库 创建表
 */
#include"_ooci.h"

using namespace idc;

int main(int argc,char* argv[])
{
    connection conn; // 创建数据库连接类的对象
    if(conn.connecttodb("scott/tiger","Simplified Chinese_China.AL32UTF8")!=0)
    {
        printf("conn.connecttodb failed\n%s\n",conn.message());
    }
    else printf("conn.connecttodb ok\n");

    sqlstatement stmt; // 创建SQL语句的对象
    stmt.connect(&conn);

    // 准备SQL语句
    stmt.prepare("\
            create table girls(\
                id number(10),\
                name varchar2(30),\
                weight number(8,2),\
                btime date,\
                memo varchar2(300),\
                pic blob,\
                primary key(id))");
                
    // 执行SQL语句
    if(stmt.execute()!=0)
    {
        printf("stmt.execute failed\n%s\n",stmt.message()); return -1;
    }
    else printf("stmt.execute ok\n");


    return 0;
}