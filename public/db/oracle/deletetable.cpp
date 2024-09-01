/* 
    Oracle数据库 删除表的数据
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

    sqlstatement stmt(&conn); // 创建SQL语句的对象，并指定使用的数据库连接 

    // 静态SQL语句
    stmt.prepare("delete from girls where id=4");

    // 执行SQL语句
    if(stmt.execute()!=0)
    {
        printf("stmt.execute failed\n%s\n",stmt.message()); return -1;
    }
    else printf("成功删除了%ld条记录\n",stmt.rpc());

    // 动态SQL语句
    stmt.prepare("delete from girls where id=:1");

    // 绑定变量
    int min=5;
    int max=7;
    stmt.bindin(1,min);
    stmt.bindin(2,max);

    // 执行SQL语句
    if(stmt.execute()!=0)
    {
        printf("stmt.execute failed\n%s\n",stmt.message()); return -1;
    }
    else printf("成功删除了%ld条记录\n",stmt.rpc());

    conn.commit();
    return 0;
}