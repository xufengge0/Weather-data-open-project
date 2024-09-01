/* 
    ftp客户端的文件上传模块
 */
#include"_public.h"
#include"_ftp.h"

using namespace idc;

// 程序运行结构体
struct st_arg
{
    char host[31];          // ftp服务端ip及端口
    int mode;               // ftp传输模式:1-被动模式，2-主动模式，缺省被动模式
    char username[31];      // ftp用户名
    char password[31];      // ftp密码
    char remotepath[256];   // 服务端的目录名
    char localpath[256];    // 本地的目录名
    char matchname[256];    // 文件匹配规则
    int ptype;              // 文件下载完成后，服务端文件的处理方式
    char localpathbak[256];// 文件的备份目录
    char okfilename[256];   // 存储已下载文件名的文件
    int timeout;            // 心跳的超时时间
    char pname[51];             // 进程名
}starg;

// 解析xml到结构体starg中
bool xmltoarg(const char* xmlbuffer);

void EXIT(int sig);

void _help();

// 文件信息结构体
struct st_fileinfo
{
    string filename;    // 文件名
    string mtime;       // 文件时间
    st_fileinfo()=default;
    st_fileinfo(const string& in_filename,const string& in_mtime):filename(in_filename),mtime(in_mtime){}
    void clear(){filename.clear();mtime.clear();}
}stfileinfo;

map<string,string> mfromok;  // 容器一存放已下载好的文件信息
list<st_fileinfo> vfilelist; // 容器二存放下载前nlist文件信息
list<st_fileinfo> vtook;     // 容器三存放本次不需要下载的文件信息
list<st_fileinfo> vdownload; // 容器四存放本次需要下载的文件信息

bool loadokfile();      // 把okfilename文件内容加载到mfromok中
bool loadlistfile();    // 把nlist文件加载到容器vfilelist中
bool compmap();         // 比较mfromok和vfilelist，得到vtook和vdownload
bool writetookfile();   // 把容器vtook中的数据写入okfilename，覆盖之前文件
bool appendtookfile(st_fileinfo i);  // 把下载完成的文件信息追加到okfilename

cftpclient ftp;     // ftp客户端对象
clogfile logfile;   // 日志对象
cpactive pactive;   // 进程心跳对象

int main(int argc,char* argv[])
{
    
    if(argc!=3) { _help(); return -1;}

    signal(SIGINT,EXIT);signal(SIGTERM,EXIT);

    // 第一部分 从服务器某个目录下载文件，可指定文件的匹配规则

    // 打开日志文件
    if(logfile.open(argv[1])==false)
    {
        cout<<"open logfile "<<argv[1]<<"failed.\n";
    }

    // 解析xml得到文件运行参数
    if(xmltoarg(argv[2])==false) return false;

    pactive.addpinfo(starg.timeout,starg.pname); // 添加心跳信息

    // 登录远程ftp服务器
    if (ftp.login(starg.host,starg.username,starg.password,starg.mode) == false)
    {
        logfile.write("ftp login failed:%s\n",ftp.response()); return -1;
    }
    logfile.write("ftp login ok.\n");

    // 进入ftp存放文件的目录（先进入目录的原因是: 避免nlist读取时带上目录名，减少TCP报文传递负担！！）
    if(ftp.chdir(starg.remotepath)==false)
    {
        logfile.write("ftp chdir %s failed:%s\n",starg.remotepath,ftp.response()); return -1;
    }

    // 调用nlist方法保存，目录下的文件名到本地文件中
    if (ftp.nlist("",sformat("/tmp/nlist/ftpputfiles_%d.nlist",getpid()))==false)
    {
        logfile.write("ftp nlist %s failed:%s\n",starg.remotepath,ftp.response()); return -1;
    }
    logfile.write("ftp nlist %s ok.\n",starg.remotepath);

    pactive.uptatime(); // 更新心跳信息
    
    // 把nlist方法获取到的list文件加载到容器vfilelist中
    if(loadlistfile()==false)
    {
        logfile.write("ftp loadlistfile  failed:%s\n",ftp.response()); return -1;
    }
    logfile.write("ftp loadlistfile ok.\n");

    if (starg.ptype==1)
    {
        // 把okfilename文件内容加载到mfromok中
        loadokfile();

        // 比较mfromok和vfilelist，得到vtook和vdownload
        compmap();

        // 把容器vtook中的数据写入okfilename，覆盖之前文件
        writetookfile();
    }
    // 当ptype不为1时，交换容器二和四的内容，使下载文件部分的代码不改动
    else swap(vfilelist,vdownload);

    pactive.uptatime(); // 更新心跳信息
    
    // 调用get方法下载文件
    string strremotefilename, strlocalfilename;
    for (auto i:vdownload)
    {
        sformat(strremotefilename,"%s/%s",starg.remotepath,i.filename.c_str()); // 拼接服务端文件全路径名
        sformat(strlocalfilename,"%s/%s",starg.localpath,i.filename.c_str());   // 拼接客户端文件全路径名

        logfile.write("ftp get %s begin...\n",strremotefilename.c_str()); // 开始下载，写入日志

        if(ftp.put(strremotefilename,strlocalfilename,true)==false)
        {
            logfile.write("ftp get failed:%s\n",ftp.response()); return -1;
        }
        logfile.write("ftp get ok.\n");
    
        pactive.uptatime(); // 更新心跳信息

        // 文件下载完后处理服务端的文件

        // 如果ptype=1,把下载完成的文件信息追加到okfilename
        if(starg.ptype==1) appendtookfile(i);

        // ptype=2需要删除文件时
        if(starg.ptype==2)
        {
            if(ftp.ftpdelete(strremotefilename)==false)
            {
                logfile.write("ftp ftpdelete failed:%s\n",ftp.response()); return -1;
            }
            logfile.write("ftp ftpdelete ok.\n");
        }

        // ptype=3需要备份文件时
        if(starg.ptype==3)
        {   
            // 备份文件的全路径名
            string strremotefilebak = sformat("%s/%s",starg.localpathbak,i.filename.c_str());
            if(ftp.ftprename(strremotefilename,strremotefilebak)==false) // 改为新的文件名（相当于移动了文件）
            {
                logfile.write("ftp ftpfilebak failed:%s\n",ftp.response()); return -1;
            }
            logfile.write("ftp ftpfilebak ok.\n");
        }
    
    }
    return 0;
}

void EXIT(int sig)
{
    cout<<"sig:"<<sig<<endl;
    exit(0);
}
void _help()
{
    string s = R"(Example:./ftpputfiles /log/idc/ftpputfiles.log "<host>192.168.247.128:21</host><mode>1</mode>
<username>mysql</username><password>12345</password><remotepath>/tmp/idc/durfdata</remotepath>
<localpath>/home/mysql/client</localpath><matchname>*.csv,*.xml</matchname><ptype>1</ptype>
<localpathbak>/tmp/idc/surfdatabak</localpathbak><okfilename>/home/mysql/client/okfilename.xml</okfilename>
<timeout>100</timeout><pname>ftpfileget</pname>")";
    s.erase(remove(s.begin(), s.end(), '\n'), s.end()); // 移除字符串中的换行符，remove将\n移到末尾，erase删除\n
    cout<<s<<endl;

    cout<<"本程序是通用的功能模块，用于从远程ftp服务器下载文件\n";
    cout<<"/log/idc/ftpputfiles.log:为日志保存目录\n";
    cout<<"<host>192.168.247.128:21</host>:ftp服务端ip及端口\n";
    cout<<"<mode>1</mode>:ftp传输模式\n";
    cout<<"<username>mysql</username>:ftp用户名\n";
    cout<<"<password>12345</password>:ftp密码\n";
    cout<<"<remotepath>/data/server/surfdata</remotepath>:服务端的目录名\n";
    cout<<"<localpath>/data/client/surfdata</localpath>:本地的目录名\n";
    cout<<"<matchname>*.xml,*csv</matchname>:文件名的匹配规则\n";
    cout<<"<ptype>1</ptype>:文件下载完成后，服务端文件的处理方式，1-什么都不做，2-删除，3-备份\n";
    cout<<"<localpathbak>/tmp/idc/surfdatabak</localpathbak>:文件的备份目录，ptype=3时生效\n";
    cout<<"<okfilename></okfilename>:存储已下载文件名的文件,ptype=1时生效\n";
    cout<<"<timeout>100</timeout>:心跳的超时时间\n";
    cout<<"<pname>ftpfileget</pname>:进程名\n";
}

bool xmltoarg(const char* xmlbuffer)
{
    memset(&starg,0,sizeof(st_arg));

    getxmlbuffer(xmlbuffer,"host",starg.host);
    if(strlen(starg.host)==0)
    {
        logfile.write("host is null.\n"); return false;
    }

    getxmlbuffer(xmlbuffer,"mode",starg.mode);
    if(starg.mode!=2) starg.mode=1;

    getxmlbuffer(xmlbuffer,"username",starg.username);
    if(strlen(starg.username)==0)
    {
        logfile.write("username is null.\n"); return false;
    }

    getxmlbuffer(xmlbuffer,"password",starg.password);
    if(strlen(starg.password)==0)
    {
        logfile.write("password is null.\n"); return false;
    }

    getxmlbuffer(xmlbuffer,"remotepath",starg.remotepath);
    if(strlen(starg.remotepath)==0)
    {
        logfile.write("remotepath is null.\n"); return false;
    }

    getxmlbuffer(xmlbuffer,"localpath",starg.localpath);
    if(strlen(starg.localpath)==0)
    {
        logfile.write("localpath is null.\n"); return false;
    }

    getxmlbuffer(xmlbuffer,"matchname",starg.matchname);
    if(strlen(starg.matchname)==0)
    {
        logfile.write("matchname is null.\n"); return false;
    }
    getxmlbuffer(xmlbuffer,"ptype",starg.ptype);
    if(starg.ptype!=1 && starg.ptype!=2 && starg.ptype!=3)
    {
        logfile.write("ptype is error.\n"); return false;
    }
    if(starg.ptype==3)
    {
        getxmlbuffer(xmlbuffer,"localpathbak",starg.localpathbak);
        if(strlen(starg.localpathbak)==0)
        {
            logfile.write("localpathbak is null.\n"); return false;
        }
    }
    if(starg.ptype==1)
    {
        getxmlbuffer(xmlbuffer,"okfilename",starg.okfilename);
        if(strlen(starg.okfilename)==0)
        {
            logfile.write("okfilename is null.\n"); return false;
        }
        
    }
    getxmlbuffer(xmlbuffer,"timeout",starg.timeout);
    if(starg.timeout==0)
    {
        logfile.write("timeout is error.\n"); return false;
    }
    getxmlbuffer(xmlbuffer,"pname",starg.pname);
    if(strlen(starg.pname)==0)
    {
        logfile.write("pname is null.\n"); return false;
    }


    return true;
}
bool loadlistfile()
{
    cifile ifile;
    if(ifile.open(sformat("/tmp/nlist/ftpputfiles_%d.nlist",getpid()))==false)
    {
        cout<<"ifile.open failed\n";
    }

    string buf;
    while (ifile.readline(buf))
    {
        if(matchstr(buf,starg.matchname))
        {   
            if(starg.ptype==1 )
            {
                ftp.mtime(buf);
            }
            vfilelist.emplace_back(buf,ftp.m_mtime);
        }
    }

    ifile.closeandremove(); // 删除nlist文件
    /* for (auto i: vfilelist) // 调试代码
    {
        cout<<i.filename<<i.mtime<<endl;
    } */
    
    return true;
}
bool loadokfile()
{   
    mfromok.clear();

    cifile ifile;
    if(ifile.open(starg.okfilename)==false) return true; // 第一次打开时为空文件，不是错误，返回true

    string buf;
    st_fileinfo filenifo; filenifo.clear();
    while (ifile.readline(buf))
    {
        getxmlbuffer(buf,"filename",filenifo.filename);
        getxmlbuffer(buf,"mtime",filenifo.mtime);

        mfromok[filenifo.filename]=filenifo.mtime; // 向容器中放入元素（键值对）
    }
    return true;
}
bool compmap()
{
    vtook.clear();
    vdownload.clear();

    for(auto i:vfilelist)
    {
        // 在mfromok中查找相同文件名的文件
        auto it = mfromok.find(i.filename);

        if(it==mfromok.end()) // 未找到，直接放入vdownload容器
        {
            vdownload.push_back(i);
        }
        else // 找到
        {   
            // 修改时间一样，放入vtook容器
            if(i.mtime==it->second) vtook.push_back(i);
            // 修改时间不一样，放入vdownload容器
            else vdownload.push_back(i);
        }
    }
    return true;
}
bool writetookfile()
{
    cofile ofile;
    if(ofile.open(starg.okfilename)==false)
    {
        logfile.write("open okfilename %s failed.\n",starg.okfilename); return false;
    }
    for(auto i:vtook)
    {
        ofile.writeline("<filename>%s</filename><mtime>%s</mtime>\n",i.filename.c_str(),i.mtime.c_str());
    }
    ofile.closeandrename();
    return true;
}
bool appendtookfile(st_fileinfo i)
{
    cofile ofile;
    ofile.open(starg.okfilename,false,ios::app); // 追加的模式打开，第二个参数添false
    
    ofile.writeline("<filename>%s</filename><mtime>%s</mtime>\n",i.filename.c_str(),i.mtime.c_str());

    ofile.closeandrename();

    return true;
}