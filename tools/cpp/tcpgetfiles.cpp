/* 
    采用TCP协议，实现文件下载的异步通讯客户端
 */
#include"_public.h"
using namespace idc;

// 程序运行参数结构体
struct st_arg
{
    int clienttype;         // 客户端类型，1-上传文件，2-下载文件
    char ip[31];            // 服务端IP地址
    int port;               // 服务端端口号
    char srvpath[256];      // 服务端文件的存放目录
    int ptype;              // 文件上传成功后本地文件的处理方式，1-删除，2-备份
    char srvpathbak[256];   // 服务端文件的备份目录，仅当ptype=2时有效
    bool andchild;          // 是否上传srvpath的子目录中的文件
    char matchname[256];    // 文件匹配规则
    char clientpath[256];   // 本地文件的存放目录
    int timetvl;            // 扫描本地文件的时间间隔，单位-秒
    int timeout;            // 心跳的超时时间
    char pname[51];         // 进程名，建议用tcpputfiles_后缀
} starg;

void _help();
bool _xmltoarg(const char* strxmlbuf); // 将xml解析到starg结构体

clogfile logfile;
ctcpclient tcpclient;
cpactive pactive;

bool login(const char* argv);// 登录
bool activetest(); // 心跳报文
void recvfilesmain();// 接收文件
bool recvfile(const string& filename,const string& mtime,const int filesize);

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

    recvfilesmain();

    return 0;
}
void _help()
{
    cout<<"Using:./tcpgetfiles logfile ip port\n";
    string s = R"(Example:./tcpgetfiles /log/idc/tcpgetfiles.log "<ip>192.168.247.128</ip><port>5000</port>
<clientpath>/tmp/client</clientpath><ptype>1</ptype><srvpath>/tmp/server</srvpath><andchild>true</andchild>
<matchname>*.xml,*.csv,*.json</matchname><timetvl>10</timetvl><timeout>50</timeout><pname>tcpgetfiles_surfdata</pname>")";
    s.erase(remove(s.begin(), s.end(), '\n'), s.end()); // 移除字符串中的换行符，remove将\n移到末尾，erase删除\n
    cout<<s<<endl;
    
    cout<<"本程序是数据中心的公共功能模块，采用tcp协议把文件上传到服务器\n";
    cout<<"/log/idc/tcpgetfiles.log:为日志保存目录\n";
    cout<<"IP:服务端ip\n";
    cout<<"port:服务端端口\n";
    cout<<"ptype:文件上传成功后本地文件的处理方式，1-删除，2-备份\n";
    cout<<"srvpath:服务端文件的存放目录\n";
    cout<<"srvpathbak:服务端文件的备份目录，仅当ptype=2时有效\n";
    cout<<"andchild:是否上传clientpath的子目录中的文件\n";
    cout<<"matchname:文件匹配规则\n";
    cout<<"clientpath:本地文件的存放目录\n";
    cout<<"timetvl:扫描服务端文件的时间间隔，单位-秒\n";
    cout<<"timeout:心跳的超时时间\n";
    cout<<"pname:进程名，建议用tcpgetfiles_后缀\n";

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

    getxmlbuffer(strxmlbuf,"srvpathbak",starg.srvpathbak);
    if(starg.ptype==2 && strlen(starg.srvpathbak)==0) {logfile.write("srvpathbak is null.\n"); return false;}

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
    sformat(strsendbuf,"%s<clienttype>2</clienttype>",argv);
    //xxxxxxx logfile.write("发送：%s\n",strsendbuf.c_str());
    if(tcpclient.write(strsendbuf)==false) return false;

    if(tcpclient.read(strrecvbuf)==false) return false;
    //xxxxxxx logfile.write("接收：%s\n",strrecvbuf.c_str());

    return true;
}
void recvfilesmain()
{
    pactive.uptatime();
    
    while (1)
    {
        if(tcpclient.read(strrecvbuf,starg.timetvl+10)==false)
        {
            logfile.write("tcpclient.read %s failed.\n",strrecvbuf.c_str()); break;
        }
        //xxxxxxx else logfile.write("tcpserver.read %s success.\n",strrecvbuf.c_str());

        // 接收心跳报文并回复
        if (strrecvbuf=="<activetest>ok</activetest>")
        {
            strsendbuf="activetest ok!";
            if(tcpclient.write(strsendbuf)==false)
            {
                logfile.write("tcpclient.write %s failed.\n",strsendbuf.c_str());
            }
            //xxxxxxx else logfile.write("tcpserver.write %s success.\n",strsendbuf.c_str());
        }

        // 解析上传的报文的xml
        if(strrecvbuf.find("filename")!=string::npos)
        {
            string serverfilename;
            string mtime;
            int filesize;
            getxmlbuffer(strrecvbuf,"filename",serverfilename);
            getxmlbuffer(strrecvbuf,"mtime",mtime);
            getxmlbuffer(strrecvbuf,"size",filesize);

            // 接收文件内容
            string clientfilename = serverfilename;
            replacestr(clientfilename,starg.srvpath,starg.clientpath,false); // 将原文件名替换为新的服务端文件名
            
            logfile.write("recv %s(%d)...\n",clientfilename.c_str(),filesize);
            if(recvfile(clientfilename,mtime,filesize)==true)
            {
                logfile.write("recv %s(%d) ok.\n",clientfilename.c_str(),filesize);
                // 拼接确认报文
                sformat(strsendbuf,"<filename>%s</filename><result>ok</result>",serverfilename.c_str());
            }
            else
            {
                logfile.write("recv %s(%d) failed.\n",clientfilename.c_str(),filesize);
                // 拼接确认报文
                sformat(strsendbuf,"<filename>%s</filename><result>failed</result>",serverfilename.c_str());
            }
            
            // 发送确认报文
            if(tcpclient.write(strsendbuf)==false)
            {
                logfile.write("tcpclient.write %s failed.\n",strsendbuf.c_str());
            }
            //xxxxxxx else logfile.write("tcpserver.write %s success.\n",strsendbuf.c_str());
        }
    }
}
bool recvfile(const string& filename,const string& mtime,const int filesize)
{
    cofile ofile;
    ofile.open(filename,ios::out|ios::binary);

    int  totalbytes=0;        // 已接收文件的总字节数。
    int  onread=0;            // 本次打算接收的字节数。
    char buffer[4096];           // 接收文件内容的缓冲区。

    while (true)
    {
      memset(buffer,0,sizeof(buffer));
      
      // 计算本次应该接收的字节数。
      if (filesize-totalbytes>4096) onread=4096;
      else onread=filesize-totalbytes;

      // 接收文件内容。
      if (tcpclient.read(buffer,onread)==false) return false;

      // 把接收到的内容写入文件。
      ofile.write(buffer,onread);

      // 计算已接收文件的总字节数，如果文件接收完，跳出循环。
      totalbytes=totalbytes+onread;

      if (totalbytes==filesize) break;
    }

    ofile.closeandrename();
    return true;
}
