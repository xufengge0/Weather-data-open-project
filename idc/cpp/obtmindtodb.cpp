/* 
    本程序用于把气象观测数据文件入库到T_ZHOBTMIND表中，支持xml和csv两种文件格式
 */
#include"idcapp.h"

clogfile logfile; // 创建日志对象
connection conn; // 创建数据库连接类的对象
cpactive pactive;// 进程的心跳

void EXIT(int sig); // 信号处理函数
// 业务处理主函数
bool _obtmindtodb(const char* pathname,const char* connstr,const char* charset);

int main(int argc,char* argv[])
{
    if(argc!=5)
    {
        printf("Using:./obtmindtodb pathname connstr charset logfile\n");
        printf("Example:/project/tools/cpp/procctl 10 /project/idc/cpp/obtmindtodb /idcdata/surfdata \"idc/idcpwd\" \"Simplified Chinese_China.AL32UTF8\" /log/idc/obtmindtodb.log\n");

        printf("本程序用于把全国气象站点观测数据文件入库到数据库T_ZHOBTMIND中，支持xml和csv两种文件格式，只插入不更新\n");
        printf("pathname:全国气象站点观测数据的存放目录（全路径）\n");
        printf("connstr:数据库的连接参数:用户名/密码@tnsname\n");
        printf("charset:数据库的字符集\n");
        printf("logfile:日志文件名（全路径）\n");
        printf("程序由procctl调度，每10s运行一次\n");

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

    // 业务处理主函数
    _obtmindtodb(argv[1],argv[2],argv[3]);


    return 0;
}
void EXIT(int sig){
    
    logfile.write("程序退出，信号=%d\n",sig);
    exit(0);
}
bool _obtmindtodb(const char* pathname,const char* connstr,const char* charset)
{
    // 1)打开气象观测数据文件目录
    cdir dir;
    if(dir.opendir(pathname,"*.xml,*.csv")==false)
    {
        logfile.write("open %s failed.\n",pathname); return false;
    }

    // 2)用循环读取每个文件
    while (1)
    {
        // 读取一个气象观测数据文件（xml,csv）
        if(dir.readdir()==false) break;

        // 如果文件需要处理，判断与数据库的连接状态，如果未连接就连接数据库
        if(conn.isopen()==false)
        {
            if(conn.connecttodb(connstr,charset)!=0)
            {
                logfile.write("conn.connecttodb(%s) failed.\n",conn.message()); return false;
            }
            else logfile.write("conn.connecttodb(%s) ok.\n",connstr);
        }

        // 操作观测数据表的对象
        CZHOBTMIND ZHOBTMIND(conn,logfile);

        // 打开文件，读取文件的一行，插入数据库中
        cifile ifile;
        if(ifile.open(dir.m_ffilename)==false)
        {
            logfile.write("ifile.open(%s) failed.\n",dir.m_ffilename); return false;
        }

        int totalcount=0;   // 总记录数
        int insertcount=0;  // 插入记录数
        ctimer timer;       // 计时器，记录每个文件耗时,创建即开始计时

        bool bisxml;        // 文件格式的临时变量，true为xml,false为csv
        if(matchstr(dir.m_ffilename,"*.xml")==true) bisxml=true;
        else bisxml = false;

        string buf;
        if(bisxml==false) ifile.readline(buf); // 扔掉csv文件的第一行

        while (1)
        {
            if(bisxml==true)
            {
                // 从文件读取一行
                if(ifile.readline(buf,"<endl/>")==false) break; // ！！注意是break
            }
            else
            {
                if(ifile.readline(buf)==false) break; // csv文件没有行结束的标志
            }
            

            totalcount++; // 成功读取，总记录数+1

            // 拆分一行到结构体中
            ZHOBTMIND.splitbuffer(buf,bisxml);

            // 向表中插入数据
            if(ZHOBTMIND.inserttable()==true)
            {
                insertcount++; // 成功插入记录数+1
            }
        }

        ifile.closeandremove();
        conn.commit();

        // 把总记录数、插入记录数、消耗时长记录日志
        logfile.write("总记录数=%d,插入记录数=%d,消耗时长=%.02f秒\n",totalcount,insertcount,timer.elapsed());

    }
    return true;
}
