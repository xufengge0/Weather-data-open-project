/* 
    本程序用于把全国气象站点参数文件的数据入库到数据库T_ZHOBTCODE中
 */

#include"_public.h" // 开发框架
#include"_ooci.h"   // 数据库操作
using namespace idc;

struct st_code // 存放站点数据结构体
{
    char province[31];
    char obtid[6];
    char obtname[31];
    char lat[11];
    char lon[11];
    char height[11];
}stcode;

list<st_code> stlist; // 存放站点参数的容器
bool loadstcode(const string &inifile); // 将站点参数放入容器

clogfile logfile; // 创建日志对象
connection conn; // 创建数据库连接类的对象
cpactive pactive;// 进程的心跳

void EXIT(int sig); // 信号处理函数

int main(int argc,char* argv[])
{
    if(argc!=5)
    {
        printf("Using:./obtcodetodb inifile connstr charset logfile\n");
        printf("Example:/project/tools/cpp/procctl 120 /project/idc/cpp/obtcodetodb /project/idc/ini/stcode.ini \"idc/idcpwd\" \"Simplified Chinese_China.AL32UTF8\" /log/idc/obtcodetodb.log\n");

        printf("本程序用于把全国气象站点参数文件的数据入库到数据库T_ZHOBTCODE中，如果站点不存在则插入，存在则更新\n");
        printf("inifile:全国气象站点参数的文件名（全路径）\n");
        printf("connstr:数据库的连接参数:用户名/密码@tnsname\n");
        printf("charset:数据库的字符集\n");
        printf("logfile:日志文件名（全路径）\n");
        printf("程序由procctl调度，每120s运行一次\n");
        return -1;
    }

    // 1)处理退出的信号，打开日志文件
    // 忽略所有信号、关闭IO
    closeioandsignal(true);
    // 处理打断信号ctrl c（2）、killall（15）
    signal(SIGINT,EXIT);signal(SIGTERM,EXIT);

    if(logfile.open(argv[4]) == false)
    {
        cout<<"日志打开失败。"<<endl; return false;
    }
    logfile.write("程序开始运行...\n");

    // 进程的心跳，10秒足够
    pactive.addpinfo(10,"obtcodetodb");

    /* logfile.write("%s\n",argv[1]);
    logfile.write("%s\n",argv[2]);
    logfile.write("%s\n",argv[3]); */

    // 2)把气象站点数据加载到容器中
    if(loadstcode(argv[1])==false) {logfile.write("loadstcode(%s) failed\n",argv[1]);EXIT(-1);}

    // 3)连接数据库
    if(conn.connecttodb(argv[2],argv[3])!=0)
    {
        logfile.write("conn.connecttodb failed\n%s\n",conn.message());
    }
    else logfile.write("conn.connecttodb(%s) ok\n",argv[2]);

    // 4)准备和更新表的SQL语句
    sqlstatement stmtins(&conn); // 创建插入SQL语句的对象
    stmtins.prepare("\
        insert into T_ZHOBTCODE(obtid,cityname,provname,lat,lon,height,keyid) \
                                       values(:1,:2,:3,:4*100,:5*100,:6*10,SEQ_ZHOBTCODE.nextval)");
    //stmtins.prepare("insert into T_ZHOBTCODE(obtid,cityname,provname,lat,lon,height,keyid) \
                    values(:1,:2,:3,:4*100,:5*100,:6*10,SEQ_ZHOBTCODE.nextval)"); 
    // 绑定变量
    stmtins.bindin(1,stcode.obtid,5);
    stmtins.bindin(2,stcode.obtname,30);
    stmtins.bindin(3,stcode.province,30);
    stmtins.bindin(4,stcode.lat,10);
    stmtins.bindin(5,stcode.lon,10);
    stmtins.bindin(6,stcode.height,10);

    sqlstatement stmtupt(&conn); // 创建更新SQL语句的对象
    stmtupt.prepare("update T_ZHOBTCODE set cityname=:1,provname=:2,lat=:3*100,lon=:4*100,height=:5*10,uptime=sysdate where obtid=:6");
    // 绑定变量
    stmtupt.bindin(1,stcode.obtname,30);
    stmtupt.bindin(2,stcode.province,30);
    stmtupt.bindin(3,stcode.lat,10);
    stmtupt.bindin(4,stcode.lon,10);
    stmtupt.bindin(5,stcode.height,10);
    stmtupt.bindin(6,stcode.obtid,5);
    
    int inscount=0,uptcount=0; // 插入记录数、更新记录数
    ctimer timer; // 数据库的操作时间计数器

    // 5)遍历容器，向表中插入/更新记录
    for(auto i:stlist)
    {   
        stcode = i;

        // 执行插入SQL语句
        if(stmtins.execute()!=0)
        {
            if(stmtins.rc()==1) // 错误代码==1,违反唯一约束，表中已有记录
            {   
                // 表中已有记录，执行更新语句
                if(stmtupt.execute()!=0)
                {
                    logfile.write("stmtupd.execute failed\n%s\n",stmtupt.message()); EXIT(-1);
                }
                else uptcount++;
            }
            else
            {
                // 其他错误，程序退出
                logfile.write("stmtins.execute failed\n%s\n",stmtins.message()); EXIT(-1);
            }
        }
        else inscount++;
    }
    // 把总记录数、插入记录数、更新记录数、消耗时长记录日志
    logfile.write("总记录数=%d,插入记录数=%d,更新记录数=%d,消耗时长=%.02f秒\n",stlist.size(),inscount,uptcount,timer.elapsed());

    // 6)提交事务
    conn.commit();

    return 0;
}
void EXIT(int sig){

    logfile.write("程序退出，信号=%d\n",sig);
    exit(0);
}
// 加载站点参数到容器中
bool loadstcode(const string &inifile)
{
    cifile ifile; // 读文件的对象
    if (ifile.open(inifile)==false)
    {
        logfile.write("open %s failed.",inifile.c_str()); // 注意C风格字符串
    }

    string strbuffer; // 存放读取的数据
    ccmdstr cmdstr; // 字符串拆分的类
    //st_code stcode; // 存放拆分数据的结构体
    ifile.readline(strbuffer); // 读取标题 舍弃

    while (ifile.readline(strbuffer)) // 读取整个文件
    {
        // logfile.write("strbuffer=%s\n",strbuffer.c_str()); // 调试代码

        // 把每一行进行拆分 安徽,58424,安庆,30.37,116.58,62
        cmdstr.splittocmd(strbuffer,",");
        // 初始化结构体
        memset(&stcode,0,sizeof(st_code));
        // 将一行数据写入结构体中
        cmdstr.getvalue(0,stcode.province,30); // 省
        cmdstr.getvalue(1,stcode.obtid,5);    // 站号
        cmdstr.getvalue(2,stcode.obtname,30);  // 站名
        cmdstr.getvalue(3,stcode.lon,10);      // 经度
        cmdstr.getvalue(4,stcode.lat,10);      // 纬度
        cmdstr.getvalue(5,stcode.height,10);   // 海拔

        // 将结构体存入容器
        stlist.push_back(stcode);

    }
    /* for (auto i:stlist) // 调试代码
    {
        logfile.write("province=%s obtname=%s obtid=%s lon=%f lat=%f heigth=%f\n",i.province,
        i.obtname,i.obtid,i.lon,i.lat,i.heigth);
    } */
    return true;
}