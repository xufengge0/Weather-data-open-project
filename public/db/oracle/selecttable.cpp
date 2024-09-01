/* 
    Oracle数据库 查询表的数据
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

    stmt.prepare("select id,name,weight,btime,memo from girls where id>=:1 and id<=:2");

    // 绑定输入变量
    int min=1;
    int max=3;
    stmt.bindin(1,min);
    stmt.bindin(2,max);

    // 定义变量结构体
    struct st_girls
    {
        long id;
        char name[31];
        double weight;
        char btime[20];
        char memo[301];
    } stgirls;

    // 绑定结果集变量
    stmt.bindout(1,stgirls.id);
    stmt.bindout(2,stgirls.name,30);
    stmt.bindout(3,stgirls.weight);
    stmt.bindout(4,stgirls.btime,19);
    stmt.bindout(5,stgirls.memo,300);

    // 执行SQL语句
    if(stmt.execute()!=0)
    {
        printf("stmt.execute failed\n%s\n",stmt.message()); return -1;
    }

    // 输出缓冲区中的结果集
    while (1)
    {   
        // 从结果集中取一条记录
        if(stmt.next()!=0) break;

        // 打印获取的记录
        printf("id=%ld,name=%s,weight=%.02f,btime=%s,memo=%s\n",stgirls.id,stgirls.name,stgirls.weight,stgirls.btime,stgirls.memo);
    }

    // 获取SQL被执行后的记录数
    printf("成功选择了%ld条记录\n",stmt.rpc());

    conn.commit();
    return 0;
}