/* 
    采用TCP协议，实现文件上传的异步通讯客户端
 */
#include"_public.h"
using namespace idc;

// 程序运行参数结构体
struct st_arg
{
    int clienttype;         // 客户端类型，1-上传文件，2-下载文件
    char ip[31];            // 服务端IP地址
    int port;               // 服务端端口号
    char clientpath[256];   // 本地文件的存放目录
    int ptype;              // 文件上传成功后本地文件的处理方式，1-删除，2-备份
    char clientpathbak[256];// 本地文件的备份目录，仅当ptype=2时有效
    bool andchild;          // 是否上传clientpath的子目录中的文件
    char matchname[256];    // 文件匹配规则
    char srvpath[256];      // 服务端文件的存放目录
    int timetvl;            // 扫描本地文件的时间间隔，单位-秒
    int timeout;            // 心跳的超时时间
    char pname[51];         // 进程名，建议用tcpputfiles_后缀
}starg;

void _help();
bool _xmltoarg(const char* strxmlbuf); // 将xml解析到starg结构体

clogfile logfile;
ctcpclient tcpclient;
cpactive pactive;

bool login(const char* argv);// 登录
bool _tcpputfiles(bool &bcontinue);// 上传文件
bool sendfile(const string &filename,const int filesize);  // 发送文件
bool ackmessage(const string &strrecvbuf); // 上传完后处理本地文件
bool activetest(); // 心跳报文

string strrecvbuf; // 接收缓冲
string strsendbuf; // 发送缓冲

void EXIT(int sig);

int main(int argc,char* argv[])
{
    if(argc!=3) { _help(); return -1;}

    _xmltoarg(argv[2]);
    
    signal(SIGINT,EXIT);signal(SIGTERM,EXIT);

    // 打开日志
    if(logfile.open(argv[1])==false)
    {
        cout<<"logfile.open failed.\n"; return -1;
    }

    pactive.addpinfo(starg.timeout,starg.pname); // 将心跳信息写入共享内存

    // 连接服务端
    if(tcpclient.connect(starg.ip,starg.port)==false)
    {
        logfile.write("tcpclient.connect %s %d failed.\n",starg.ip,starg.port);EXIT(-1);
    }
    else logfile.write("tcpclient.connect %s %d success.\n",starg.ip,starg.port);

    // 向服务端发送登录报文，将参数传递给服务端
    if(login(argv[2])==false) {logfile.write("login failed\n"); EXIT(-1);}

    bool bcontinue=true; // 如果调用了_tcpputfiles发送了文件，置为true，缺省为true

    while (1)
    {
        // 执行一次文件上传任务
        if(_tcpputfiles(bcontinue)==false) {logfile.write("_tcpputfiles failed.\n");EXIT(-1);}

        // 在发送文件时，不休眠
        if(bcontinue==false)
        {
            // 发送心跳报文
            activetest();

            // 休眠
            sleep(starg.timetvl); logfile.write("sleep %ds.\n",starg.timetvl);
        }

        pactive.uptatime();
    }
    


    return 0;
}
void _help()
{
    cout<<"Using:./tcpputfiles logfile ip port\n";
    string s = R"(Example:./tcpputfiles /log/idc/tcpputfiles.log "<ip>192.168.247.128</ip><port>5000</port>
<clientpath>/tmp/client</clientpath><ptype>1</ptype><srvpath>/tmp/server</srvpath><andchild>true</andchild>
<matchname>*.xml,*.csv,*.json</matchname><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>")";
    s.erase(remove(s.begin(), s.end(), '\n'), s.end()); // 移除字符串中的换行符，remove将\n移到末尾，erase删除\n
    cout<<s<<endl;
    
    cout<<"本程序是数据中心的公共功能模块，采用tcp协议把文件上传到服务器\n";
    cout<<"/log/idc/ftpputfiles.log:为日志保存目录\n";
    cout<<"IP:服务端ip\n";
    cout<<"port:服务端端口\n";
    cout<<"ptype:文件上传成功后本地文件的处理方式，1-删除，2-备份\n";
    cout<<"clientpath:本地文件的存放目录\n";
    cout<<"clientpathbak:本地文件的备份目录，仅当ptype=2时有效\n";
    cout<<"andchild:是否上传clientpath的子目录中的文件\n";
    cout<<"matchname:文件匹配规则\n";
    cout<<"srvpath:服务端文件的存放目录\n";
    cout<<"timetvl:扫描本地文件的时间间隔，单位-秒\n";
    cout<<"timeout:心跳的超时时间\n";
    cout<<"pname:进程名，建议用tcpputfiles_后缀\n";

}
void EXIT(int sig)
{
    cout<<"sig="<<sig<<endl;
    exit(0);
}
bool activetest()
{
    strsendbuf="<activetest>ok</activetest>";
    if(tcpclient.write(strsendbuf)==false)
    {
        logfile.write("tcpclient.write %s failed.\n",strsendbuf.c_str());
    }
    //xxxxxxx else logfile.write("tcpclient.write %s success.\n",strsendbuf.c_str());

    if(tcpclient.read(strrecvbuf,10)==false)
    {
        logfile.write("tcpclient.read %s failed.\n",strrecvbuf.c_str());
    }
    //xxxxxxx else logfile.write("tcpclient.read %s success.\n",strrecvbuf.c_str());

    return true;
}
bool _xmltoarg(const char* strxmlbuf)
{
    memset(&starg,0,sizeof(st_arg));

    getxmlbuffer(strxmlbuf,"ip",starg.ip);
    if(strlen(starg.ip)==0) {logfile.write("ip is null.\n"); return false;}

    getxmlbuffer(strxmlbuf,"port",starg.port);
    if((starg.port)==0) {logfile.write("port is null.\n"); return false;}

    getxmlbuffer(strxmlbuf,"ptype",starg.ptype);
    if(starg.ptype!=1 && starg.ptype!=2) {logfile.write("ptype is not in(1,2).\n"); return false;}

    getxmlbuffer(strxmlbuf,"clientpath",starg.clientpath);
    if(strlen(starg.clientpath)==0) {logfile.write("clientpath is null.\n"); return false;}

    getxmlbuffer(strxmlbuf,"clientpathbak",starg.clientpathbak);
    if(starg.ptype==2 && strlen(starg.clientpathbak)==0) {logfile.write("clientpathbak is null.\n"); return false;}

    getxmlbuffer(strxmlbuf,"andchild",starg.andchild);

    getxmlbuffer(strxmlbuf,"matchname",starg.matchname);
    if(strlen(starg.matchname)==0) {logfile.write("matchname is null.\n"); return false;}

    getxmlbuffer(strxmlbuf,"srvpath",starg.srvpath);
    if(strlen(starg.srvpath)==0) {logfile.write("srvpath is null.\n"); return false;}

    getxmlbuffer(strxmlbuf,"timetvl",starg.timetvl);
    if((starg.timetvl)==0) {logfile.write("timetvl is null.\n"); return false;}
    // 扫描本地目录的时间间隔一般<30s
    if(starg.timetvl>30) starg.timetvl=30;

    getxmlbuffer(strxmlbuf,"timeout",starg.timeout);
    if((starg.timeout)==0) {logfile.write("timeout is null.\n"); return false;}
    // 心跳超时时间不能小于更新目录的间隔时间
    if(starg.timeout<starg.timetvl) {logfile.write("timeout:%d < timetvl:%d.\n",starg.timeout,starg.timetvl); return false;}

    getxmlbuffer(strxmlbuf,"pname",starg.pname);
    if(strlen(starg.pname)==0) {logfile.write("pname is null.\n"); return false;}

    return true;
}
bool login(const char* argv)
{
    sformat(strsendbuf,"%s<clienttype>1</clienttype>",argv);
    //xxxxxxx logfile.write("发送：%s\n",strsendbuf.c_str());
    if(tcpclient.write(strsendbuf)==false) return false;

    if(tcpclient.read(strrecvbuf)==false) return false;
    //xxxxxxx logfile.write("接收：%s\n",strrecvbuf.c_str());

    return true;
}
bool _tcpputfiles(bool &bcontinue)
{
    bcontinue=false;

    cdir dir;
    // 打开待发送目录
    if(dir.opendir(starg.clientpath,starg.matchname,10000,starg.andchild)==false)
    {
        logfile.write("opendir %s failed\n",starg.clientpath); return false;
    }

    int delay=0; // 未收到确认报文的文件数量，发送文件+1，接收报文-1

    while(dir.readdir())
    {
        bcontinue=true;

        // 拼接文件信息到发送缓冲中
        sformat(strsendbuf,"<filename>%s</filename><mtime>%s</mtime><size>%d</size>",
                dir.m_ffilename.c_str(),dir.m_mtime.c_str(),dir.m_filesize);
        // 发送文件信息
        if(tcpclient.write(strsendbuf)==false)
        {
            logfile.write("tcpclient.write %s failed.\n",strsendbuf.c_str()); return false;
        }
    
        // 发送文件
        logfile.write("send %s(%d)...\n",dir.m_ffilename.c_str(),dir.m_filesize);
        if(sendfile(dir.m_ffilename,dir.m_filesize)==true)
        {
            logfile.write("send %s(%d) ok.\n",dir.m_ffilename.c_str(),dir.m_filesize);
            delay++;
        }
        else 
        {
            logfile.write("send %s(%d) failed.\n",dir.m_ffilename.c_str(),dir.m_filesize);
            return false;
        }

        pactive.uptatime();

        // 接收文件的确认报文（异步通讯）
        while (delay>0)
        {
            if(tcpclient.read(strrecvbuf,-1)==false) break;
            //xxxxxxx logfile.write("tcpclient.read %s.\n",strrecvbuf.c_str());

            // 处理确认报文（删除原文件、备份原文件）
            ackmessage(strrecvbuf);
            delay--;
        }
        
        // 继续接收文件的确认报文
        while (delay>0)
        {
            if(tcpclient.read(strrecvbuf,10)==false) break;
            //xxxxxxx logfile.write("tcpclient.read %s.\n",strrecvbuf.c_str());

            // 处理确认报文（删除原文件、备份原文件）
            ackmessage(strrecvbuf);
            delay--;
        }
    }
    return true;
}
bool sendfile(const string &filename,const int filesize)
{
    int  onread=0;        // 每次调用fin.read()时打算读取的字节数。  每次应搬砖头数。
    int  totalbytes=0;    // 从文件中已读取的字节总数。 已搬砖头数。
    char buffer[4096];    // 存放读取数据的buffer。     每次搬七块砖头。

    cifile ifile;
    ifile.open(filename,ios::in|ios::binary);

    while (totalbytes<filesize)
    {
        memset(buffer,0,sizeof(buffer));

        if(filesize-totalbytes > 4096) onread=4096;
        else onread = filesize-totalbytes;

        ifile.read(buffer,onread);

        tcpclient.write(buffer,onread);

        totalbytes += onread;
    }
    return true;
}
bool ackmessage(const string& strrecvbuf)
{
    string filename,result;
    getxmlbuffer(strrecvbuf,"filename",filename);
    getxmlbuffer(strrecvbuf,"result",result);

    if(result!="ok") return true; // 若未成功直接返回，等下一次重新上传

    // 若上传成功，判断文件处理方式
    if(starg.ptype==1) // 删除
    {
        remove(filename.c_str());
    }
    if(starg.ptype==2) // 备份
    {
        // 生成备份的文件名
        string bakfilename=filename;
        replacestr(bakfilename,starg.clientpath,starg.clientpathbak,false);// 将bakfilename里的路径替换成备份路径
        // 备份文件
        if(renamefile(filename,bakfilename)==false)
        {
            logfile.write("备份文件 %s 失败.\n",bakfilename.c_str());
        }
        //xxxxxxx else logfile.write("备份文件 %s 成功.\n",bakfilename.c_str());
    }
    return true;
}
