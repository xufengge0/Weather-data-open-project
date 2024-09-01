/* 
    Oracle数据库 将二进制数据存入数据库的blob字段
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

    // 插入一条数据
    stmt.prepare("insert into girls(id,name,pic) values(1,'冰冰',empty_blob())");
    if(stmt.execute()!=0)
    {
        printf("stmt.execute failed\n%s\n",stmt.message()); return -1;
    }

    // 使用游标提取记录的pic字段
    stmt.prepare("select pic from girls where id=1 ");
    stmt.bindblob(); // 绑定
    if(stmt.execute()!=0)
    {
        printf("stmt.execute failed\n%s\n",stmt.message()); return -1;
    }
    // 从结果集获取一条记录
    if(stmt.next()!=0) return 0;
    // 将文件写入clob字段
    if(stmt.filetolob("/project/public/db/oracle/pic_in.jpeg")!=0)
    {
        printf("stmt.filetolob failed\n%s\n",stmt.message()); return -1;
    }
    else printf("二进制文件导入成功\n");

    conn.commit();
    return 0;
}