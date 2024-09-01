/* 
    Oracle数据库 插入表的数据
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

    // 静态SQL语句 一次性的插入，效率不如动态高，特殊字符不便处理，安全性（SQL注入）
    /* stmt.prepare("insert into girls(id,name,weight,btime,memo)\
                    values(2,'西施',48.5,to_date('2000-1-1 12:12:12','yyyy-mm-dd hh24:mi:ss'),'美女')");
    
    // 执行SQL语句
    if(stmt.execute()!=0)
    {
        printf("stmt.execute failed\n%s\n",stmt.message()); return -1;
    }
    else printf("成功插入%ld条记录\n",stmt.rpc()); */

    // 动态SQL语句
    // 定义变量结构体
    struct st_girls
    {
        long id;
        char name[31];
        double weight;
        char btime[20];
        char memo[301];
    } stgirls;

    stmt.prepare("insert into girls(id,name,weight,btime,memo)\
                    values(:1,:2,:3,to_date(:4,'yyyy-mm-dd hh24:mi:ss'),:5)"); // :1:2可理解为参数

    // 绑定变量
    stmt.bindin(1,stgirls.id);
    stmt.bindin(2,stgirls.name,30);
    stmt.bindin(3,stgirls.weight);
    stmt.bindin(4,stgirls.btime,19);
    stmt.bindin(5,stgirls.memo,300);

    // 变量赋值和执行
    for(int i=0;i<10;i++)
    {
        // 变量初始化
        memset(&stgirls,0,sizeof(st_girls));

        // 给变量赋值
        stgirls.id=i;
        sprintf(stgirls.name,"西施%05d",i); // sprintf将格式化数据写入字符串
        stgirls.weight=45.45+i;
        sprintf(stgirls.btime,"2000-1-1 12:12:%02d",i);
        sprintf(stgirls.memo,"这是第%02d个超女",i);
    

        // 执行SQL语句
        if(stmt.execute()!=0)
        {
            printf("stmt.execute failed\n%s\n",stmt.message()); return -1;
        }
        else printf("成功插入%ld条记录\n",stmt.rpc());
    }

    conn.commit(); // 提交事务
    return 0;
}