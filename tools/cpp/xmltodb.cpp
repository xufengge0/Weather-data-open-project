/* 
    共享平台的公共功能模块，把xml文件入库到Oracle数据库中
 */
#include"_tools.h"

// 程序运行参数的结构体。
struct st_arg
{
    char connstr[101];          // 数据库的连接参数。
    char charset[51];           // 数据库的字符集。
    char inifilename[301];      // 数据入库的参数配置文件。
    char xmlpath[301];          // 待入库xml文件存放的目录。
    char xmlpathbak[301];       // xml文件入库后的备份目录。
    char xmlpatherr[301];       // 入库失败的xml文件存放的目录。
    int  timetvl;               // 本程序运行的时间间隔，本程序常驻内存。
    int  timeout;               // 本程序运行时的超时时间。
    char pname[51];             // 本程序运行时的程序名。
} starg;

void _help();
void EXIT(int sig);                        // 信号处理函数
bool xmltoarg(const char* xmlbuffer);      // 解析xml到结构体starg中
bool _xmltodb();                           // 业务处理主函数。
int _xmltodb(const string& ffilename, const string& filename); // 处理xml文件的主函数（参数为xml文件全路径文件名，纯文件名），成功返回0
bool xmltobakerr(const string &fullfilename,const string &srcpath,const string &destpath); // 入库（成功或失败）后转移xml文件到新目录

clogfile logfile;   // 创建日志对象
connection conn;    // 创建数据库连接对象
ctcols tcols;       // 获取表全部的列信息和主键的对象

ctimer timer;        // 创建计时器，处理每个xml文件的时间
int totalcount,inscount,uptcount;// 每个xml总记录数、插入记录数、更新记录数

// 数据入库参数的结构体。
struct st_xmltotable
{
    char filename[101];    // xml文件的匹配规则，用逗号分隔。
    char tname[31];        // 待入库的表名。
    int    uptbz;          // 更新标志：1-更新；2-不更新。
    char execsql[301];     // 处理xml文件之前，执行的SQL语句。

}stxmltotable;
vector<st_xmltotable> vxmltotable; // 数据入库的参数的容器。
bool loadxmltodb(); // 把数据入库的参数配置文件starg.inifilename加载到vxmltotable容器中。
bool findxmltotable(const string& filename); //根据xml文件名，从容器vxmltotable中，查找入库参数，保存到结构体stxmltotable
cpactive pactive;   // 进程的心跳

string strinsertsql;    // 拼接插入的sql语句
string strupdatesql;    // 更新的sql语句
void crtsql();          // 拼接插入和更新的sql语句

vector<string> vcolvalue;       // 存放xml每一行解析出来的字段的值
sqlstatement stmtins,stmtupt;   // 插入和更新表语句的对象
void preparesql();              // 准备SQL语句，绑定输入变量
bool execsql();                 // 打开文件前判断是否需要先执行execsql语句
void splitbuf(string &buf);     // 解析xml的一行字段的值到vcolvalue中

int main(int argc,char* argv[])
{
    if(argc!=3) {_help(); return -1;}

    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程。
    // 但请不要用 "kill -9 +进程号" 强行终止。
    // closeioandsignal(true); 
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    // 打开日志文件
    if(logfile.open(argv[1])==false)
    {
        cout<<"open logfile "<<argv[1]<<"failed.\n";
    }

    // 解析xml得到文件运行参数
    if(xmltoarg(argv[2])==false) EXIT(-1);

    pactive.addpinfo(starg.timeout,starg.pname); // 解析运行参数后增加心跳

    // 业务处理主函数。
    if(_xmltodb()==false) EXIT(-1);

    return 0;
}
// 入库（成功或失败）后转移xml文件到新目录
bool xmltobakerr(const string &fullfilename,const string &srcpath,const string &destpath)
{
    string destfullfilename=fullfilename; 

    replacestr(destfullfilename,srcpath,destpath,false); // 获得目标文件名
    if(renamefile(fullfilename,destfullfilename.c_str())==false)
    {
        logfile.write("renamefile(%s,%s)failed",fullfilename.c_str(),destfullfilename.c_str()); return false;
    }
    return true;
}
// 解析xml的一行字段的值到vcolvalue中
void splitbuf(string &buf)
{
    //vcolvalue.clear();

    string tmp;
    for(int i=0;i<tcols.m_vallcols.size();i++)
    {
        getxmlbuffer(buf,tcols.m_vallcols[i].colname,tmp,tcols.m_vallcols[i].collen);

        if(strcmp(tcols.m_vallcols[i].datatype,"date")==0)
        {
            picknumber(tmp,tmp,false,false);
        }
        if(strcmp(tcols.m_vallcols[i].datatype,"number")==0)
        {
            picknumber(tmp,tmp,true,true);
        }
        
        vcolvalue[i] = tmp.c_str();
    }
    return;
}
// 打开文件前判断是否需要先执行execsql语句
bool execsql()
{
    // 不需要执行时，直接返回
    if(strlen(stxmltotable.execsql)==0) return true;

    sqlstatement stmt(&conn);
    stmt.prepare(stxmltotable.execsql);
    if(stmt.execute()!=0)
    {
        logfile.write("stmt.execute(%s) failed\n%s",stxmltotable.execsql,stmt.message()); return false;
    }
    return true;
}
// 准备SQL语句，绑定输入变量
void preparesql()
{
    vcolvalue.resize(tcols.m_vallcols.size());

    // 1插入表的SQL语句，绑定输入变量
    stmtins.connect(&conn);
    stmtins.prepare(strinsertsql);
    int colseq=1;
    
    for(int i=0;i<tcols.m_vallcols.size();i++)
    {
        // keyid和upttime字段不需要绑定输入参数，跳过
        if(strcmp(tcols.m_vallcols[i].colname,"keyid")==0 || strcmp(tcols.m_vallcols[i].colname,"upttime")==0) continue;

        stmtins.bindin(colseq,vcolvalue[i],tcols.m_vallcols[i].collen);
        //logfile.write("stmtins.bindin(%d,vcolvalue[%d],%d)\n",colseq,i,tcols.m_vallcols[i].collen);
        colseq++;
    }


    // 2更新表的SQL语句，绑定输入变量
    if(stxmltotable.uptbz!=1) return; // 不需要更新时直接返回
    stmtupt.connect(&conn);
    stmtupt.prepare(strupdatesql);
    colseq=1;
    // 绑定where前面的非主键字段
    for(int i=0;i<tcols.m_vallcols.size();i++)
    {
        // 跳过主键字段
        if(tcols.m_vallcols[i].pkseq!=0) continue;
        // keyid和upttime字段不需要绑定输入参数，跳过
        if(strcmp(tcols.m_vallcols[i].colname,"keyid")==0 || strcmp(tcols.m_vallcols[i].colname,"upttime")==0) continue;

        stmtupt.bindin(colseq,vcolvalue[i],tcols.m_vallcols[i].collen);
        //logfile.write("stmtupt.bindin(%d,vcolvalue[%d],%d)\n",colseq,i,tcols.m_vallcols[i].collen);
        colseq++;
    }
    // 绑定where后面的主键字段
    for(int i=0;i<tcols.m_vallcols.size();i++)
    {
        // 跳过非主键字段
        if(tcols.m_vallcols[i].pkseq==0) continue;

        stmtupt.bindin(colseq,vcolvalue[i],tcols.m_vallcols[i].collen);
        //logfile.write("stmtupt.bindin(%d,vcolvalue[%d],%d)\n",colseq,i,tcols.m_vallcols[i].collen);
        colseq++;
    }

    return;
}
// 拼接插入和更新表的sql语句
void crtsql()
{
    // 插入表的SQL语句 insert into T_ZHOBTMIND(obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid)\
                        values(:1,to_date(:2,'yyyymmddhh24miss'),:3*10,:4*10,:5,:6,:7*10,:8*10,:9*10,SEQ_ZHOBTCODE.nextval)
    string strinsert1,strinsert2;
    int colseq=1;   // 绑定变量的序列号

    for(auto &i:tcols.m_vallcols) // 每循环一次拼接一个字段
    {
        // 配置第一段 obtid,cityname,provname,lat,lon,height,keyid
        // 跳过upttime字段，采用系统默认值
        if(strcmp(i.colname,"upttime")==0) continue;
        // 其他字段
        strinsert1 = strinsert1 + i.colname + ",";

        // 配置第二段 :1,to_date(:2,'yyyymmddhh24miss'),:3*10,:4*10,:5,:6,:7*10,:8*10,:9*10,SEQ_ZHOBTCODE.nextval
        // keyid字段
        if(strcmp(i.colname,"keyid")==0) {strinsert2 = strinsert2 + sformat("SEQ_%s.nextval",stxmltotable.tname+2) + ","; continue;}
        // 日期字段
        if(strcmp(i.datatype,"date")==0) {strinsert2 = strinsert2 + sformat("to_date(:%d,'yyyymmddhh24miss')",colseq) + ","; colseq++; continue;}
        // 其他字段
        strinsert2 = strinsert2 + sformat(":%d",colseq) + ","; colseq++;
    }
    deleterchr(strinsert1,','); deleterchr(strinsert2,',');
    strinsertsql =sformat("insert into %s(%s) values(%s)",stxmltotable.tname,strinsert1.c_str(),strinsert2.c_str());
    

    // 更新表的SQL语句 update T_ZHOBTMIND1 set t=:1,p=:2,u=:3,wd=:4,wf=:5,r=:6,vis=:7\
                            where obtid=:8 and ddatetime=to_date(:9,'yyyymmddhh24miss')
    if(stxmltotable.uptbz!=1) return; // 不需要更新时直接返回

    // 拼接set前的字段
    strupdatesql = sformat("update %s set ",stxmltotable.tname); 

    // 拼接set后的字段
    colseq=1;   // 绑定变量的序列号
    for(auto &i:tcols.m_vallcols) // 每循环一次拼接一个字段
    {
        // 跳过主键字段
        if(i.pkseq!=0) continue;

        // keyid字段,保持不变
        if(strcmp(i.colname,"keyid")==0) continue;
        // upttime字段，采用系统默认值（当前时间）
        if(strcmp(i.colname,"upttime")==0) {strupdatesql = strupdatesql + "upttime=sysdate,"; continue;}
        // 日期字段
        if(strcmp(i.datatype,"date")==0) {strupdatesql = strupdatesql + sformat("%s=to_date(:%d,'yyyymmddhh24miss')",i.colname,colseq); colseq++; continue;}
        // 其他字段
        strupdatesql = strupdatesql + sformat("%s=:%d",i.colname,colseq)+ ","; colseq++;
    }
    deleterchr(strupdatesql,',');

    // 拼接where后的字段
    strupdatesql = strupdatesql + " where 1=1 "; // 注意1=1拼接技巧！！
    for(auto &i:tcols.m_vallcols) // 每循环一次拼接一个字段
    {
        // 跳过非主键字段
        if(i.pkseq==0) continue;

        // 日期字段
        if(strcmp(i.datatype,"date")==0) {strupdatesql = strupdatesql + sformat(" and %s=to_date(:%d,'yyyymmddhh24miss')",i.colname,colseq); colseq++; continue;}
        // 其他字段
        strupdatesql = strupdatesql + sformat(" and %s=:%d",i.colname,colseq); colseq++;
    }
    // logfile.write("strupdatesql=%s",strupdatesql.c_str()); // 调试代码
    return;
}
//根据xml文件名，从容器vxmltotable中，查找入库参数,保存到结构体stxmltotable中
bool findxmltotable(const string& filename)
{
    for(auto i:vxmltotable)
    {
        // 判断文件名和容器中的入库参数是否匹配
        if(matchstr(filename,i.filename)==true)
        {
            stxmltotable = i; return true;
        }
    }
    return false;
}
// 处理xml文件的主函数，成功返回0
int _xmltodb(const string& ffilename, const string& filename)
{
    timer.start();
    totalcount=inscount=uptcount=0;

    // 1)根据待入库的xml文件名，得到表名
    if(findxmltotable(filename)==false) return 1;

    // 2)根据表名，查找数据字典，得到所有字段名、主键
    if(tcols.allcols(conn,stxmltotable.tname)==false) return 2; // 数据库系统有问题，或网络断开、或连接超时
    if(tcols.pkcols(conn,stxmltotable.tname)==false) return 2;  // 数据库系统有问题，或网络断开、或连接超时
    
    if(tcols.m_allcols.size()==0) return 3; // 如果字段名m_allcols为空，表示第二部参数配置错误，直接返回

    // 3)根据字段名、主键拼接插入、更新SQL语句
    crtsql();
    // 绑定输入变量
    preparesql();

    // 4)打开xml文件
    // 打开文件前判断是否需要先执行execsql语句
    if(execsql()==false) return 4;

    cifile ifile;
    if(ifile.open(ffilename)==false){conn.rollback(); return 5;} // 打开失败回滚事务，execsql语句回退

    string buf;
    while (1)   // 每循环一次插入一行数据
    {
        // 读取一行数据
        if(ifile.readline(buf,"<endl/>")==false) break;
        totalcount++;

        // 解析一行数据字段的值
        splitbuf(buf);

        // 执行插入、更新语句
        if(stmtins.execute()!=0)
        {
            if(stmtins.rc()==1) // 表中已有记录，违反唯一约束
            {
                if(stxmltotable.uptbz==1) // 执行更新语句
                {
                    if(stmtupt.execute()!=0)
                    {
                        logfile.write("%s\n",buf.c_str());
                        logfile.write("stmtupt.execute(%s) failed\n%s\n",stmtupt.sql(),stmtupt.message());
                    }
                    else uptcount++;
                }
            }
            else 
            {
                logfile.write("%s\n",buf.c_str());
                logfile.write("stmtins.execute(%s) failed\n%s\n",stmtins.sql(),stmtins.message());

                // 数据库系统常见问题：3113通信通道文件结尾，3114数据库关闭，3135连接失去联系，16014归档失败
                if(stmtins.rc()==3113 || stmtins.rc()==3114 || stmtins.rc()==3135 || stmtins.rc()==16014) return 2;
            }
        }
        else inscount++;
    }
    // 提交事务
    conn.commit();

    return 0;
}
// 业务处理主函数。
bool _xmltodb()
{
    // 打开待入库xml文件的目录
    cdir dir;

    int icount=50; // 计数器

    while(1) // 每循环一次入库一个目录
    {
        // 每循环30次，加载一次数据入库的参数配置文件,防止参数变动后更新不及时
        if(icount>30)
        {
            icount=0;
            if(loadxmltodb()==false) return false;
        } 
        else icount++; 

        if(dir.opendir(starg.xmlpath,"*.xml",10000,false,true)==false) // 打开时按文件名排序，先生成的文件先入库
        {
            logfile.write("dir.opendir failed(%s).\n",starg.xmlpath); return false;
        }

        // 有文件需要入库时，才连接数据库
        if(conn.isopen()==false)
        {
            // 连接数据库
            if(conn.connecttodb(starg.connstr,starg.charset)!=0)
            {
                logfile.write("conn.connecttodb(%s) failed.\n%d,%s\n",starg.connstr,conn.rc(),conn.message()); return false;
            }
            else logfile.write("conn.connecttodb(%s) ok.\n",starg.connstr);
        }

        while(1) // 每次循环 入库一个xml文件
        {
            if(dir.readdir()==false) break;
            logfile.write("文件%s入库中...",dir.m_ffilename.c_str());
            
            // 入库代码,处理xml文件的主函数
            int ret = _xmltodb(dir.m_ffilename,dir.m_filename);

            pactive.uptatime(); // 每入库一个xml文件，更新心跳

            // 处理_xmltodb函数错误的代码
            // 0-成功，备份已入库的xml文件
            if(ret==0)
            {
                logfile << "ok(消耗时间="<<timer.elapsed()<<",总记录数="<<totalcount<<",插入记录数="<<totalcount<<",更新记录数="<<uptcount<<")\n";
                if(xmltobakerr(dir.m_ffilename,starg.xmlpath,starg.xmlpathbak)==false) return false;
            }

            // 1-入库参数不正确；3-表不存在；4-打开文件前执行execsql语句失败，把xml文件放到错误目录
            if(ret==1) {logfile << "failed,1-入库参数不正确\n"; if(xmltobakerr(dir.m_ffilename,starg.xmlpath,starg.xmlpatherr)==false) return false;}

            if(ret==3) {logfile << "failed,3-表("<<stxmltotable.tname<<")不存在\n"; if(xmltobakerr(dir.m_ffilename,starg.xmlpath,starg.xmlpatherr)==false) return false;}

            if(ret==4) {logfile << "failed,4-打开文件前执行execsql语句失败\n"; if(xmltobakerr(dir.m_ffilename,starg.xmlpath,starg.xmlpatherr)==false) return false;}


            // 2-数据库错误
            if(ret==2) {logfile << "failed,2-数据库错误\n"; return false;}

            // 5-xml文件打开失败
            if(ret==5) {logfile << "failed,5-xml文件("<<dir.m_ffilename<<")打开失败\n"; return false;}
            
        }
        // 若没有文件入库则休眠，否则不休眠
        if(dir.size()==0) sleep(starg.timetvl);

        pactive.uptatime();
    }

    return true;
}
// 把数据入库的参数配置文件starg.inifilename加载到vxmltotable容器中。
bool loadxmltodb()
{
    vxmltotable.clear();

    cifile ifile;
    if(ifile.open(starg.inifilename)==false)
    {
        logfile.write("ifile.open fail(%s).\n",starg.inifilename); return false;
    }

    string buf;
    // 读取inifilename文件的一行，放入容器vxmltotable
    while(1)
    {
        memset(&stxmltotable,0,sizeof(st_xmltotable));

        if(ifile.readline(buf,"<endl/>")==false) break;

        getxmlbuffer(buf,"filename",stxmltotable.filename,100);
        getxmlbuffer(buf,"tname",stxmltotable.tname,30);
        getxmlbuffer(buf,"uptbz",stxmltotable.uptbz);
        getxmlbuffer(buf,"execsql",stxmltotable.execsql,300);

        vxmltotable.push_back(stxmltotable);
    }
    logfile.write("loadxmltotable(%s) ok.\n",starg.inifilename);

    return true;
}
void EXIT(int sig){
    
    logfile.write("程序退出，信号=%d\n",sig);
    exit(0);
}
// 显示程序的帮助
void _help()
{
    printf("Using:/project/tools/cpp/xmltodb logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/cpp/procctl 10 /project/tools/cpp/xmltodb /log/idc/xmltodb_vip.log "\
              "\"<connstr>idc/idcpwd</connstr><charset>Simplified Chinese_China.AL32UTF8</charset>"\
              "<inifilename>/project/idc/ini/xmltodb.xml</inifilename>"\
              "<xmlpath>/idcdata/xmltodb/vip</xmlpath><xmlpathbak>/idcdata/xmltodb/vipbak</xmlpathbak>"\
              "<xmlpatherr>/idcdata/xmltodb/viperr</xmlpatherr>"\
              "<timetvl>5</timetvl><timeout>50</timeout><pname>xmltodb_vip</pname>\"\n\n");

    printf("本程序是共享平台的公共功能模块，用于把xml文件入库到Oracle的表中。\n");
    printf("logfilename   本程序运行的日志文件。\n");
    printf("xmlbuffer     本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：username/passwd@tnsname。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("inifilename 数据入库的参数配置文件。\n");
    printf("xmlpath     待入库xml文件存放的目录。\n");
    printf("xmlpathbak  xml文件入库后的备份目录。\n");
    printf("xmlpatherr  入库失败的xml文件存放的目录。\n");
    printf("timetvl     扫描xmlpath目录的时间间隔（执行入库任务的时间间隔），单位：秒，视业务需求而定，2-30之间。\n");
    printf("timeout     本程序的超时时间，单位：秒，视xml文件大小而定，建议设置30以上。\n");
    printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}
bool xmltoarg(const char* xmlbuffer)
{
    memset(&starg,0,sizeof(st_arg));

    getxmlbuffer(xmlbuffer,"connstr",starg.connstr,100);
    if(strlen(starg.connstr)==0) {logfile.write("connstr is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"charset",starg.charset,50);
    if(strlen(starg.charset)==0) {logfile.write("charset is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"inifilename",starg.inifilename,300);
    if(strlen(starg.inifilename)==0) {logfile.write("inifilename is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"xmlpath",starg.xmlpath,300);
    if(strlen(starg.xmlpath)==0) {logfile.write("xmlpath is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"xmlpathbak",starg.xmlpathbak,300);
    if(strlen(starg.xmlpathbak)==0) {logfile.write("xmlpathbak is null.\n"); return false;}
    getxmlbuffer(xmlbuffer,"xmlpatherr",starg.xmlpatherr,300);
    if(strlen(starg.xmlpatherr)==0) {logfile.write("xmlpatherr is null.\n"); return false;}

    getxmlbuffer(xmlbuffer,"timetvl",starg.timetvl);
    if (starg.timetvl< 2) starg.timetvl=2;   
    if (starg.timetvl>30) starg.timetvl=30;

    getxmlbuffer(xmlbuffer,"timeout",starg.timeout);
    if(starg.timeout<0) {logfile.write("timeout小于0.\n"); return false;}
    getxmlbuffer(xmlbuffer,"pname",starg.pname,50);
    if(strlen(starg.pname)==0) {logfile.write("pname is null.\n"); return false;}

    return true;
}
