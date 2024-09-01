/* 
    数据抽取模块，将数据库中筛选出来的结果集数据提取到xml文件中
 */
#include "_public.h"
#include"_ooci.h"
using namespace idc;

// 程序运行结构体
struct st_arg
{
    char connstr[101];       // 数据源数据库的连接参数，格式：username/passwd@tnsname。
    char charset[51];        // 数据库的字符集
    char selectsql[1024];    // 从数据源数据库抽取数据的SQL语句，如果是增量抽取，一定要用递增字段作为查询条件，如where keyid>:1。
    char fieldstr[501];      // 抽取数据的SQL语句输出结果集的字段名列表，中间用逗号分隔，将作为xml文件的字段名
    char fieldlen[501];      // 抽取数据的SQL语句输出结果集字段的长度列表，中间用逗号分隔。fieldstr与fieldlen的字段必须一一对应
    char outpath[256];       // 输出xml文件存放的目录
    char bfilename[31];      // 输出xml文件的前缀。
    char efilename[31];      // 输出xml文件的后缀。
    int maxcount;            // 输出xml文件的最大记录数，缺省是0，表示无限制，如果本参数取值为0，注意适当加大timeout的取值，防止程序超时
    char starttime[52];      // 程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行
    char incfield[31];       // 递增字段名，它必须是fieldstr中的字段名，并且只能是整型，一般为自增字段。
    char incfilename[256];   // 已抽取数据的递增字段最大值存放的文件，如果该文件丢失，将重新抽取全部的数据
    char connstr1[101];      // 已抽取数据的递增字段最大值存放的数据库的连接参数。connstr1和incfilename二选一，connstr1优先
    int timeout;             // 本程序的超时时间
    char pname[51];          // 进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查
}starg;


void _help();
void EXIT(int sig); // 信号处理函数
bool xmltoarg(const char* xmlbuffer);       // 解析xml到结构体starg中
bool isstarttime(const char* starttime);    // 判断当前时间是否在运行时间区间内

long imaxincvalue;                          // 递增字段的最大值
int incvaluepos=-1;                         // 递增字段在结果集数组中的位置
bool readincfield();                        // 从数据库或者文件中加载上次已抽取数据的最大值
bool _dminingoracle();                      // 数据抽取主函数
bool writeincfield();                       // 将已抽取的数据的递增字段的最大值写入文件或数据库

cpactive pactive;   // 进程的心跳

ccmdstr fieldstr;   // 存储结果集的字段名的数组
ccmdstr fieldlen;   // 存储结果集的字段名长度的数组
clogfile logfile;   // 创建日志对象
connection conn;

int main(int argc,char* argv[])
{
    if(argc!=3) {_help(); return -1;}

    // 打开日志文件
    if(logfile.open(argv[1])==false)
    {
        cout<<"open logfile "<<argv[1]<<"failed.\n";
    }

    // 解析xml得到文件运行参数
    if(xmltoarg(argv[2])==false) EXIT(-1);

    // 判断是否在运行时间区间内
    if(isstarttime(starg.starttime)==false) return 0;

    // 第一次心跳信息(放在连接数据库之前，是因为网络通信可能超时)
    pactive.addpinfo(starg.timeout,starg.pname);

    // 连接数据库
    if(conn.connecttodb(starg.connstr,starg.charset)!=0)
    {
        logfile.write("conn.connecttodb(%s) failed.\n",starg.connstr); EXIT(-1);
    }
    else logfile.write("conn.connecttodb(%s) ok.\n",starg.connstr);

    // 从数据库或者文件中加载上次已抽取数据的最大值
    readincfield();
    
    // 数据抽取主函数
    _dminingoracle();

    return 0;
}
// 将已抽取的数据的递增字段的最大值写入文件或数据库
bool writeincfield()
{
    // 不增量抽取，直接返回
    if(strlen(starg.incfield)==0) return true;

    // 更新递增字段
    if(strlen(starg.connstr1)!=0) // 优先更新数据库递增字段
    {
        connection conn1;
        if(conn1.connecttodb(starg.connstr1,starg.charset)!=0)
        {
            logfile.write("conn.connecttodb(%s) failed.\n",starg.connstr1); return false;
        }
        else logfile.write("conn.connecttodb(%s) ok.\n",starg.connstr1);

        sqlstatement stmt1(&conn1);
        stmt1.prepare("update T_MAXINCVALUE set maxincvalue=:1 where pname=:2");
        stmt1.bindin(1,imaxincvalue);
        stmt1.bindin(2,starg.pname,50);
        // 更新失败
        if(stmt1.execute()!=0)
        {   
            // 表不存在时，创建表,插入记录
            if(stmt1.rc()==942)
            {
                stmt1.execute("create table T_MAXINCVALUE (pname varchar2(50),maxincvalue number(15),primary key(pname))");
                stmt1.execute("insert into T_MAXINCVALUE values('%s',%ld)",starg.pname,imaxincvalue);
                conn1.commit();
                return true;
            }
            else // 其他错误，写日志
            {
                logfile.write("stmt1.execute() fail.\n%d,%s\n",stmt1.rc(),stmt1.message());
            }
        }
        else
        {
            // 如果影响的行数==0，表明记录不存在(不会返回错误)，就插入记录
            if(stmt1.rpc()==0)
            {
                stmt1.execute("insert into T_MAXINCVALUE values('%s',%ld)",starg.pname,imaxincvalue);
                
            }
            conn1.commit();
        }
    }
    else // 更新文件递增字段
    {
        cofile ofile;
        ofile.open(starg.incfilename);
        ofile.writeline("%ld",imaxincvalue);
        ofile.closeandrename();
    }

    return true;
}                
// 从数据库或者文件中加载上次已抽取数据的最大值
bool readincfield()
{
    imaxincvalue=0; 

    // 不增量抽取
    if(strlen(starg.incfield)==0) return true;

    // 增量抽取
    // 寻找递增字段在结果集数组中的位置
    for(int i=0;i<fieldstr.size();i++)
    {
        if(fieldstr[i]==starg.incfield) {incvaluepos=i; break;}
    }
    if(incvaluepos==-1) // 未找到递增字段名，写日志
    {
        logfile.write("递增字段名(%s)，不在列表(%s)中.\n",starg.incfield,starg.fieldstr); return false;
    }

    // 获取递增字段
    if(strlen(starg.connstr1)!=0) // 优先从数据库获取递增字段
    {
        connection conn1;
        if(conn1.connecttodb(starg.connstr1,starg.charset)!=0)
        {
            logfile.write("conn.connecttodb(%s) failed.\n",starg.connstr1); return false;
        }
        else logfile.write("conn.connecttodb(%s) ok.\n",starg.connstr1);

        sqlstatement stmt1(&conn1);
        stmt1.prepare("select maxincvalue from T_MAXINCVALUE where pname=:1");
        stmt1.bindin(1,starg.pname);
        stmt1.bindout(1,imaxincvalue);
        if(stmt1.execute()!=0)
        {
            logfile.write("stmt1.execute failed.\n%d,%s\n",stmt1.rc(),stmt1.message()); return false;
        }
        stmt1.next();

    }
    else // 从文件获取递增字段
    {
        string tmp;
        cifile ifile;
        ifile.open(starg.incfilename);
        ifile.readline(tmp);
        imaxincvalue = stoi(tmp);
    }

    logfile.write("上次已抽取数据的最大值，%s=%ld\n",starg.incfield,imaxincvalue);
    return true;
}            
// 数据抽取主函数
bool _dminingoracle()
{
    // 1)准备数据抽取的SQL语句
    sqlstatement stmt(&conn);
    stmt.prepare(starg.selectsql);

    // 2)绑定结果集变量
    string strfieldvalue[fieldstr.size()];
    for (int i = 1; i <= fieldstr.size(); i++)
    {
        stmt.bindout(i,strfieldvalue[i-1],stoi(fieldlen[i-1]));
    }
    
    // 如果是增量抽取，绑定自增字段名的值
    if(strlen(starg.incfield) > 0) stmt.bindin(1,imaxincvalue);

    // 3)执行抽取数据的SQL语句
    if(stmt.execute()!=0)
    {
        logfile.write("stmt.execute failed.\n%s\n",stmt.message()); return false;
    }
    
    // 更新心跳（抽取可能超时）
    pactive.uptatime();

    string xmlfilename; // 用于拼接的文件名
    int seq=1;           // 文件末尾的序列
    cofile ofile;
    
    // 4)获取结果集中的记录，写入xml文件
    while (1)   // 每循环一次，写入一行数据
    {
        if(stmt.next()!=0) break; // 从结果集中读取一行，失败跳出循环

        if(ofile.isopen()==false)
        {
            // 拼接文件名
            sformat(xmlfilename,"%s/%s_%s_%s_%d.xml",starg.outpath,starg.bfilename,ltime1("yyyymmddhh24miss").c_str(),starg.efilename,seq);

            // 打开xml文件
            if(ofile.open(xmlfilename)==false)
            {
                logfile.write("ofile.open(%s) failed.\n",xmlfilename.c_str()); return false;
            }
            ofile.writeline("<data>\n");
        }

        for (int i = 0; i < fieldstr.size(); i++)     // 每循环一次，写入一个标签数据
        {
            ofile.writeline("<%s>%s</%s>",fieldstr[i].c_str(),strfieldvalue[i].c_str(),fieldstr[i].c_str());
        }
        ofile.writeline("<endl/>\n");   // 写入每行数据结束的标志

        // 判断写入的行数是否已到达最大限制
        if(starg.maxcount>0 && stmt.rpc()%starg.maxcount==0)
        {
            ofile.writeline("</data>\n");
            if(ofile.isopen()==true)
            {
                if(ofile.closeandrename()==false)
                {
                    logfile.write("ofile.closeandrename(%s) failed.\n",xmlfilename.c_str()); return false;
                }
                else logfile.write("成功生成文件(%s),记录数=%d.\n",xmlfilename.c_str(),starg.maxcount);

                // 更新心跳（生成xml文件可能超时）
                pactive.uptatime();
            }
            seq++;
        }

        // 每写入一行后，将增量字段的最大值保存到临时变量中
        if(strlen(starg.incfield) > 0 && stoi(strfieldvalue[incvaluepos]) > imaxincvalue)
        {
            imaxincvalue = stoi(strfieldvalue[incvaluepos]);
        }
    }

    // 5)关闭xml文件
    ofile.writeline("</data>\n");
    
    // 无最大行数限制时
    if(starg.maxcount==0)
    {
        if(ofile.isopen()==true)
        {
            if(ofile.closeandrename()==false)
            {
                logfile.write("ofile.closeandrename(%s) failed.\n",xmlfilename.c_str()); return false;
            }
            else logfile.write("成功生成文件(%s),记录数=%d.\n",xmlfilename.c_str(),stmt.rpc());
        }
    }
    // 有最大行数限制时
    if(starg.maxcount>0)
    {
        if(ofile.isopen()==true)
        {
            if(ofile.closeandrename()==false)
            {
                logfile.write("ofile.closeandrename(%s) failed.\n",xmlfilename.c_str()); return false;
            }
            else logfile.write("成功生成文件(%s),记录数=%d.\n",xmlfilename.c_str(),stmt.rpc()%starg.maxcount);
        }
    }

    // 抽取数据后，把递增字段的值更新
    if(stmt.rpc()>0) writeincfield();
    
    return true;
}
// 判断是否在运行时间区间内
bool isstarttime(const char* starttime)
{
    if(strlen(starg.starttime)!=0)
    {
        string timenowhh24 = ltime1("hh24"); // 获取当前小时存到timenowhh24
        string stargstarttime(starg.starttime);
        // 若当前时间不在运行时间的区间内，返回false
        if(stargstarttime.find(timenowhh24)==string::npos) return false;
    }
    
    return true;
}
void _help()
{
    printf("Using:/project/tools/bin/dminingoracle logfilename xmlbuffer\n\n");
    printf("全量抽取：\n");
    printf("Sample:/project/tools/bin/procctl 3600 /project/tools/cpp/dminingoracle /log/idc/dminingoracle_ZHOBTCODE.log "
              "\"<connstr>idc/idcpwd</connstr><charset>Simplified Chinese_China.AL32UTF8</charset>"\
              "<selectsql>select obtid,cityname,provname,lat,lon,height from T_ZHOBTCODE where obtid like '5%%%%'</selectsql>"\
              "<fieldstr>obtid,cityname,provname,lat,lon,height</fieldstr><fieldlen>5,30,30,10,10,10</fieldlen>"\
              "<bfilename>ZHOBTCODE</bfilename><efilename>togxpt</efilename><outpath>/idcdata/dmindata</outpath>"\
              "<timeout>30</timeout><pname>dminingoracle_ZHOBTCODE</pname>\"\n\n");
    printf("增量抽取：\n");
    printf("/project/tools/bin/procctl 30 /project/tools/cpp/dminingoracle /log/idc/dminingoracle_ZHOBTMIND.log "\
              "\"<connstr>idc/idcpwd</connstr><charset>Simplified Chinese_China.AL32UTF8</charset>"\
              "<selectsql>select obtid,to_char(ddatetime,'yyyymmddhh24miss'),t,p,u,wd,wf,r,vis,keyid from T_ZHOBTMIND where keyid>:1 and obtid like '5%%%%'</selectsql>"\
              "<fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>5,19,8,8,8,8,8,8,8,15</fieldlen>"\
              "<bfilename>ZHOBTMIND</bfilename><efilename>togxpt</efilename><outpath>/idcdata/dmindata</outpath>"\
              "<starttime></starttime><incfield>keyid</incfield>"\
              "<incfilename>/idcdata/dmining/dminingoracle_ZHOBTMIND_togxpt.keyid</incfilename>"\
              "<timeout>30</timeout><pname>dminingoracle_ZHOBTMIND_togxpt</pname>"\
              "<maxcount>1000</maxcount><connstr1>scott/tiger</connstr1>\"\n\n");

    printf("本程序是共享平台的公共功能模块，用于从Oracle数据库源表抽取数据，生成xml文件。\n");
    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据源数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("selectsql   从数据源数据库抽取数据的SQL语句，如果是增量抽取，一定要用递增字段作为查询条件，如where keyid>:1。\n");
    printf("fieldstr    抽取数据的SQL语句输出结果集的字段名列表，中间用逗号分隔，将作为xml文件的字段名。\n");
    printf("fieldlen    抽取数据的SQL语句输出结果集字段的长度列表，中间用逗号分隔。fieldstr与fieldlen的字段必须一一对应。\n");
    printf("outpath     输出xml文件存放的目录。\n");
    printf("bfilename   输出xml文件的前缀。\n");
    printf("efilename   输出xml文件的后缀。\n"); 
    printf("maxcount    输出xml文件的最大记录数，缺省是0，表示无限制，如果本参数取值为0，注意适当加大timeout的取值，防止程序超时。\n");
    printf("starttime   程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行。"\
         "如果starttime为空，表示不启用，只要本程序启动，就会执行数据抽取任务，为了减少数据源数据库压力"\
         "抽取数据的时候，如果对时效性没有要求，一般在数据源数据库空闲的时候时进行。\n");
    printf("incfield    递增字段名，它必须是fieldstr中的字段名，并且只能是整型，一般为自增字段。"\
          "如果incfield为空，表示不采用增量抽取的方案。\n");
    printf("incfilename 已抽取数据的递增字段最大值存放的文件，如果该文件丢失，将重新抽取全部的数据。\n");
    printf("connstr1    已抽取数据的递增字段最大值存放的数据库的连接参数。connstr1和incfilename二选一，connstr1优先。\n");
    printf("timeout     本程序的超时时间，单位：秒。\n");
    printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}
void EXIT(int sig){
    
    logfile.write("程序退出，信号=%d\n",sig);
    exit(0);
}
bool xmltoarg(const char* xmlbuffer)
{
    memset(&starg,0,sizeof(st_arg));

    getxmlbuffer(xmlbuffer,"connstr",starg.connstr,100);
    if(strlen(starg.connstr)==0) {logfile.write("connstr is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"charset",starg.charset,50);
    if(strlen(starg.charset)==0) {logfile.write("charset is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"selectsql",starg.selectsql,1000);
    if(strlen(starg.selectsql)==0) {logfile.write("selectsql is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"fieldstr",starg.fieldstr,500);
    if(strlen(starg.fieldstr)==0) {logfile.write("fieldstr is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"fieldlen",starg.fieldlen,500);
    if(strlen(starg.fieldlen)==0) {logfile.write("fieldlen is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"outpath",starg.outpath,255);
    if(strlen(starg.outpath)==0) {logfile.write("outpath is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"bfilename",starg.bfilename,30);
    if(strlen(starg.bfilename)==0) {logfile.write("bfilename is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"efilename",starg.efilename,30);
    if(strlen(starg.efilename)==0) {logfile.write("efilename is null.\n"); return false;}

    // 可选参数

    getxmlbuffer(xmlbuffer,"maxcount",starg.maxcount);
    getxmlbuffer(xmlbuffer,"starttime",starg.starttime,50);
    getxmlbuffer(xmlbuffer,"incfield",starg.incfield,30);
    getxmlbuffer(xmlbuffer,"incfilename",starg.incfilename,255);
    getxmlbuffer(xmlbuffer,"connstr1",starg.connstr1,100);

    getxmlbuffer(xmlbuffer,"timeout",starg.timeout);
    if(starg.timeout==0) {logfile.write("timeout is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"pname",starg.pname,50);
    if(strlen(starg.pname)==0) {logfile.write("pname is null.\n"); return false;}

    // 将结果集的字段名和字段名长度拆分到数组中，并判断元素个数是否相同
    fieldstr.splittocmd(starg.fieldstr,",");
    fieldlen.splittocmd(starg.fieldlen,",");
    if(fieldlen.size() != fieldstr.size())
    {
        logfile.write("fieldstr和fieldlen中元素个数不一致.\n"); return false;
    }

    // 如果是增量抽取，connstr1和incfilename二选一
    if(strlen(starg.incfield)>0)
    {
        if(strlen(starg.connstr1)==0 && strlen(starg.incfilename)==0)
        {
            logfile.write("增量抽取，connstr1和incfilename必须二选一.\n"); return false;
        }
    }

    return true;
}