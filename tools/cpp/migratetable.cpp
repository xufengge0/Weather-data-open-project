/* 
    共享平台的公共功能模块，用于迁移表中的数据。
 */
#include"_tools.h"

// 程序运行参数的结构体。
struct st_arg
{
    char connstr[101];      // 数据库的连接参数。
    char tname[31];         // 待迁移的表名。
    char totname[31];       // 目的表名
    char keycol[31];        // 待迁移的表的唯一键字段名。
    char where[1001];       // 待迁移的数据需要满足的条件。
    int    maxcount;        // 执行一次SQL删除的记录数。
    char starttime[31];     // 程序运行的时间区间。
    int  timeout;           // 本程序运行时的超时时间。
    char pname[51];         // 本程序运行时的程序名。
} starg;

void _help();
void EXIT(int sig); // 信号处理函数
bool _xmltoarg(const char* xmlbuffer);      // 解析xml到结构体starg中
bool instarttime(); // 判断当前时间是否在程序运行的时间区间内。
bool _migratetable();// 业务处理主函数。

clogfile logfile;   // 创建日志对象
connection conn;    // 创建数据库连接对象
ctcols tcols;       // 获取表列信息的对象
cpactive pactive;   // 进程的心跳

int main(int argc,char* argv[])
{
    if(argc!=3) {_help(); return -1;}

    // 打开日志文件
    if(logfile.open(argv[1])==false)
    {
        cout<<"open logfile "<<argv[1]<<"failed.\n";
    }

    // 解析xml得到文件运行参数
    if(_xmltoarg(argv[2])==false) EXIT(-1);

    pactive.addpinfo(starg.timeout,starg.pname);

    // 连接数据库
    if(conn.connecttodb(starg.connstr,"Simplified Chinese_China.AL32UTF8")!=0)  // 字符集随便填
    {
        logfile.write("conn.connecttodb(%s) failed\n",starg.connstr); EXIT(-1);
    }

    // 业务处理主函数。
    _migratetable();

    return 0;
}
// 业务处理主函数。
bool _migratetable()
{
    ctimer timer;
    char tmpvalue[21];  // 暂定存放待删除行的键值

    // 1拼接选择SQL语句
    // select rowid from T_ZHOBTMIND1 where ddatetime<sysdate-1
    sqlstatement stmtsel(&conn);
    stmtsel.prepare("select %s from %s %s",starg.keycol,starg.tname,starg.where);
    stmtsel.bindout(1,tmpvalue,20);

    // 2拼接删除SQL语句
    // delete from T_ZHOBTMIND1 where rowid in (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10);
    string strsql = sformat("delete from %s where %s in(",starg.tname,starg.keycol);
    for (int i = 0; i < starg.maxcount; i++)
    {
        strsql = strsql+sformat(":%lu,",i+1);
    }
    deletelrchr(strsql,',');
    strsql = strsql+")";

    // 临时存储待删除行键值的数组
    char keyvalues[starg.maxcount][21];

    // 绑定删除语句的输入变量
    sqlstatement stmtdel(&conn);
    stmtdel.prepare(strsql);
    for (int i = 0; i < starg.maxcount; i++)
    {
        stmtdel.bindin(i+1,keyvalues[i],20);
    }
    
    // 3准备插入目的表的SQL语句，绑定输入参数
    // insert into T_ZHOBTMIND1_HIS(全部的字段) select 全部的字段 from T_ZHOBTMIND1 where rwoid in (:1,:2,:3,:4,:5,:6,:7,:8,:9,:10);
    tcols.allcols(conn,starg.tname); // 获取表全部的字段
    string strsql1 = sformat("insert into %s select %s from %s where %s in(",starg.totname,tcols.m_allcols.c_str(),starg.tname,starg.keycol);
    for (int i = 0; i < starg.maxcount; i++)
    {
        strsql1 = strsql1+sformat(":%lu,",i+1);
    }
    deletelrchr(strsql1,',');
    strsql1 = strsql1+")";

    // 绑定插入语句的输入变量
    sqlstatement stmtins(&conn);
    stmtins.prepare(strsql1);
    for (int i = 0; i < starg.maxcount; i++)
    {
        stmtins.bindin(i+1,keyvalues[i],20);
    }

    // 执行选择的SQL语句
    if(stmtsel.execute()!=0)
    {
        logfile.write("stmtsel.execute() failed\n%s",stmtsel.message()); return false;
    }
    
    int ccount=0;   // keyvalues中有效元素个数
    memset(keyvalues,0,sizeof(keyvalues));

    // 3处理结果集
    while (1)  // 每循环一次，从结果集中取出一条记录放入keyvalues
    {   
        memset(tmpvalue,0,sizeof(tmpvalue));
        if(stmtsel.next()!=0) break;
        strcpy(keyvalues[ccount],tmpvalue);
        ccount++;

        // 判断keyvalues是否达到了最大数量，达到则执行删除的SQL语句
        if(ccount==starg.maxcount)
        {
            if(stmtins.execute()!=0)
            {
                logfile.write("stmtins.execute() failed\n%s",stmtins.message()); return false;
            }

            if(stmtdel.execute()!=0)
            {
                logfile.write("stmtdel.execute() failed\n%s",stmtdel.message()); return false;
            }
            conn.commit();
            ccount=0;
            memset(keyvalues,0,sizeof(keyvalues));
            pactive.uptatime();
        }
    }
    // keyvalues仍有元素就再执行删除的SQL语句
    if(ccount>0)
    {
        if(stmtins.execute()!=0)
        {
            logfile.write("stmtins.execute() failed\n%s",stmtins.message()); return false;
        }

        if(stmtdel.execute()!=0)
        {
            logfile.write("stmtdel.execute() failed\n%s",stmtdel.message()); return false;
        }
        conn.commit();
    }
    // 消耗时间、和影响的记录数记录日志
    if(stmtsel.rpc()>0) logfile.write("migrate from %s to %s %d rows in %.02fsec.\n",starg.tname,starg.totname,stmtsel.rpc(),timer.elapsed());

    return true;
}
void EXIT(int sig){
    
    logfile.write("程序退出，信号=%d\n",sig);
    exit(0);
}
// 显示程序的帮助
void _help()
{
    printf("Using:/project/tools/bin/migratetable logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 3600 /project/tools/cpp/migratetable /log/idc/migratetable_ZHOBTMIND1.log "\
                         "\"<connstr>idc/idcpwd</connstr><tname>T_ZHOBTMIND1</tname><totname>T_ZHOBTMIND1_HIS</totname>"\
                         "<keycol>rowid</keycol><where>where ddatetime<sysdate-0.03</where>"\
                         "<maxcount>10</maxcount><starttime>22,23,00,01,02,03,04,05,06,13</starttime>"\
                         "<timeout>120</timeout><pname>migratetable_ZHOBTMIND1</pname>\"\n\n");

    printf("本程序是共享平台的公共功能模块，用于迁移表中的数据。\n");

    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("tname       待迁移数据表的表名。\n");
    printf("totname     目的数据表的表名。\n");
    printf("keycol      待迁移数据表的唯一键字段名，可以用记录编号，如keyid，建议用rowid，效率最高。\n");
    printf("where       待迁移的数据需要满足的条件，即SQL语句中的where部分。\n");
    printf("maxcount    执行一次SQL语句删除的记录数，建议在100-500之间。\n");
    printf("starttime   程序运行的时间区间，例如02,13表示：如果程序运行时，踏中02时和13时则运行，其它时间不运行。"\
                                "如果starttime为空，本参数将失效，只要本程序启动就会执行数据迁移，"\
                                "为了减少对数据库的压力，数据迁移一般在业务最闲的时候时进行。\n");
    printf("timeout     本程序的超时时间，单位：秒，建议设置120以上。\n");
    printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}

// 把xml解析到参数starg结构中
bool _xmltoarg(const char *strxmlbuffer)
{
    memset(&starg,0,sizeof(struct st_arg));

    getxmlbuffer(strxmlbuffer,"connstr",starg.connstr,100);
    if (strlen(starg.connstr)==0) { logfile.write("connstr is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"tname",starg.tname,30);
    if (strlen(starg.tname)==0) { logfile.write("tname is null.\n"); return false; }
    getxmlbuffer(strxmlbuffer,"totname",starg.totname,30);
    if (strlen(starg.totname)==0) { logfile.write("totname is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"keycol",starg.keycol,30);
    if (strlen(starg.keycol)==0) { logfile.write("keycol is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"where",starg.where,1000);
    if (strlen(starg.where)==0) { logfile.write("where is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"starttime",starg.starttime,30);

    getxmlbuffer(strxmlbuffer,"maxcount",starg.maxcount);
    if (starg.maxcount==0) { logfile.write("maxcount is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"timeout",starg.timeout);
    if (starg.timeout==0) { logfile.write("timeout is null.\n"); return false; }

    getxmlbuffer(strxmlbuffer,"pname",starg.pname,50);
    if (strlen(starg.pname)==0) { logfile.write("pname is null.\n"); return false; }

    return true;
}
// 判断当前时间是否在程序运行的时间区间内。
bool instarttime()
{
    if(strlen(starg.starttime)!=0)
    {
        if(strstr(starg.starttime,ltime1("hh24").c_str())==nullptr) return false;
    }
    return true;
}