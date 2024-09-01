/* 
    Oracle数据库 修改表的数据
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
    /* stmt.prepare("update girls set name='冰冰',weight=44.4,btime=to_date('1999-1-1 11:11:11','yyyy-mm-dd hh24:mi:ss') where id=1");
    
    // 执行SQL语句
    if(stmt.execute()!=0)
    {
        printf("stmt.execute failed\n%s\n",stmt.message()); return -1;
    }
    else printf("成功修改了%ld条记录\n",stmt.rpc()); */

    // 动态SQL语句
    stmt.prepare("update girls set name=:1,weight=:2,btime=to_date(:3,'yyyy-mm-dd hh24:mi:ss') where id=:4");

    // 定义变量结构体
    struct st_girls
    {
        long id;
        char name[31];
        double weight;
        char btime[20];
        char memo[301];
    } stgirls;

    // 绑定变量
    stmt.bindin(1,stgirls.name);
    stmt.bindin(2,stgirls.weight);
    stmt.bindin(3,stgirls.btime,19);
    stmt.bindin(4,stgirls.id);

    sprintf(stgirls.name,"mm"); // sprintf将格式化数据写入字符串
    stgirls.weight=45.45;
    sprintf(stgirls.btime,"2000-1-1 12:12:21");
    stgirls.id=2;

    // 执行SQL语句
    if(stmt.execute()!=0)
    {
        printf("stmt.execute failed\n%s\n",stmt.message()); return -1;
    }
    else printf("成功修改了%ld条记录\n",stmt.rpc());

    conn.commit();
    return 0;
}