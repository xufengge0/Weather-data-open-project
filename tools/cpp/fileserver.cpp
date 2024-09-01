/* 
    采用TCP协议，实现文件上传&下载的服务端
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
    char srvpathbak[256];   // 服务端文件的备份目录
    int timetvl;            // 扫描本地文件的时间间隔，单位-秒
    int timeout;            // 心跳的超时时间
    char pname[51];         // 进程名，建议用tcpputfiles_后缀
}starg;

clogfile logfile;
ctcpserver tcpserver;
cpactive pactive;

void FathEXIT(int sig);// 父进程退出函数
void ChldEXIT(int sig);// 子进程退出函数

string strrecvbuf; // 接收缓冲
string strsendbuf; // 发送缓冲

bool clientlogin();  // 处理客户端登录的函数
void recvfilesmain();// 处理上传文件的主函数
bool recvfile(const string& filename,const string& mtime,const int filesize);

void sendfilesmain();// 处理下载文件的主函数
bool _tcpputfiles(bool &bcontinue);
bool sendfile(const string &filename,const int filesize);
bool ackmessage(const string& strrecvbuf);

int main(int argc,char* argv[])
{
    if(argc!=3)
    {
        cout<<"Using:./fileserver logfile port\n";
        cout<<"./fileserver /log/idc/fileserver.log 5000\n";
        return -1;
    }

    signal(SIGINT,FathEXIT);signal(SIGTERM,FathEXIT);

    // 打开日志
    if(logfile.open(argv[1])==false)
    {
        cout<<"logfile.open failed.\n"; return -1;
    }

    // 初始化服务端
    if(tcpserver.initserver(stoi(argv[2]))==false)
    {
        logfile.write("tcpserver.initserver %s failed.\n",argv[2]);
    }
    else logfile.write("tcpserver.initserver %s success.\n",argv[2]);

    while (1)
    {
        if(tcpserver.accept()==false)
        {
            logfile.write("tcpserver.accept failed.\n"); FathEXIT(-1);
        }
        else logfile.write("tcpserver.accept %s success.\n",tcpserver.getip());

        if(fork()>0) {tcpserver.closeclient(); continue;}

        tcpserver.closelisten();

        signal(SIGINT,ChldEXIT);signal(SIGTERM,ChldEXIT);
        signal(SIGCHLD,SIG_IGN);  // 处理僵尸进程

        // 处理登录报文的函数
        if(clientlogin()==false) ChldEXIT(-1);

        pactive.addpinfo(starg.timeout,starg.pname); // 每个子进程的心跳信息

        // 调用处理上传文件的主函数
        if(starg.clienttype==1) recvfilesmain();

        // 调用处理下载文件的主函数
        if(starg.clienttype==2) sendfilesmain();

        ChldEXIT(0);


    }
    



    return 0;
}
void FathEXIT(int sig)
{
    cout<<"父进程退出，sig="<<sig<<endl;

    tcpserver.closelisten();

    kill(0,15);

    exit(0);
}
void ChldEXIT(int sig)
{
    cout<<"子进程退出，sig="<<sig<<endl;

    tcpserver.closeclient();

    exit(0);
}
bool activetest()
{
    strsendbuf="<activetest>ok</activetest>";
    if(tcpserver.write(strsendbuf)==false)
    {
        logfile.write("tcpclient.write %s failed.\n",strsendbuf.c_str());
    }
    //xxxxxxx else logfile.write("tcpclient.write %s success.\n",strsendbuf.c_str());

    if(tcpserver.read(strrecvbuf,10)==false)
    {
        logfile.write("tcpclient.read %s failed.\n",strrecvbuf.c_str());
    }
    //xxxxxxx else logfile.write("tcpclient.read %s success.\n",strrecvbuf.c_str());

    return true;
}

void recvfilesmain()
{
    pactive.uptatime();
    
    while (1)
    {
        if(tcpserver.read(strrecvbuf,starg.timetvl+10)==false)
        {
            logfile.write("tcpserver.read %s failed.\n",strrecvbuf.c_str()); break;
        }
        //xxxxxxx else logfile.write("tcpserver.read %s success.\n",strrecvbuf.c_str());

        // 接收心跳报文并回复
        if (strrecvbuf=="<activetest>ok</activetest>")
        {
            strsendbuf="activetest ok!";
            if(tcpserver.write(strsendbuf)==false)
            {
                logfile.write("tcpserver.write %s failed.\n",strsendbuf.c_str());
            }
            //xxxxxxx else logfile.write("tcpserver.write %s success.\n",strsendbuf.c_str());
        }

        // 解析上传的报文的xml
        if(strrecvbuf.find("filename")!=string::npos)
        {
            string clientfilename;
            string mtime;
            int filesize;
            getxmlbuffer(strrecvbuf,"filename",clientfilename);
            getxmlbuffer(strrecvbuf,"mtime",mtime);
            getxmlbuffer(strrecvbuf,"size",filesize);

            // 接收文件内容
            string serverfilename = clientfilename;
            replacestr(serverfilename,starg.clientpath,starg.srvpath,false); // 将原文件名替换为新的服务端文件名
            
            logfile.write("recv %s(%d)...\n",serverfilename.c_str(),filesize);
            if(recvfile(serverfilename,mtime,filesize)==true)
            {
                logfile.write("recv %s(%d) ok.\n",serverfilename.c_str(),filesize);
                // 拼接确认报文
                sformat(strsendbuf,"<filename>%s</filename><result>ok</result>",clientfilename.c_str());
            }
            else
            {
                logfile.write("recv %s(%d) failed.\n",serverfilename.c_str(),filesize);
                // 拼接确认报文
                sformat(strsendbuf,"<filename>%s</filename><result>failed</result>",clientfilename.c_str());
            }
            
            // 发送确认报文
            if(tcpserver.write(strsendbuf)==false)
            {
                logfile.write("tcpserver.write %s failed.\n",strsendbuf.c_str());
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
      if (tcpserver.read(buffer,onread)==false) return false;

      // 把接收到的内容写入文件。
      ofile.write(buffer,onread);

      // 计算已接收文件的总字节数，如果文件接收完，跳出循环。
      totalbytes=totalbytes+onread;

      if (totalbytes==filesize) break;
    }

    ofile.closeandrename();
    return true;
}
void sendfilesmain()
{
    pactive.addpinfo(starg.timeout,starg.pname);
    while (1)
    {
        bool bcontinue=true; // 如果调用了_tcpputfiles发送了文件，置为true，缺省为true

        // 执行一次文件上传任务
        if(_tcpputfiles(bcontinue)==false) {logfile.write("_tcpputfiles failed.\n");ChldEXIT(-1);}

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
}
bool _tcpputfiles(bool &bcontinue)
{
    bcontinue=false;

    cdir dir;
    // 打开待发送目录
    if(dir.opendir(starg.srvpath,starg.matchname,10000,starg.andchild)==false)
    {
        logfile.write("opendir %s failed\n",starg.srvpath); return false;
    }

    int delay=0; // 未收到确认报文的文件数量，发送文件+1，接收报文-1

    while(dir.readdir())
    {
        bcontinue=true;

        // 拼接文件信息到发送缓冲中
        sformat(strsendbuf,"<filename>%s</filename><mtime>%s</mtime><size>%d</size>",
                dir.m_ffilename.c_str(),dir.m_mtime.c_str(),dir.m_filesize);
        // 发送文件信息
        if(tcpserver.write(strsendbuf)==false)
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
            if(tcpserver.read(strrecvbuf,-1)==false) break;
            //xxxxxxx logfile.write("tcpclient.read %s.\n",strrecvbuf.c_str());

            // 处理确认报文（删除原文件、备份原文件）
            ackmessage(strrecvbuf);
            delay--;
        }
        
        // 继续接收文件的确认报文
        while (delay>0)
        {
            if(tcpserver.read(strrecvbuf,10)==false) break;
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

        tcpserver.write(buffer,onread);

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
        replacestr(bakfilename,starg.srvpath,starg.srvpathbak,false);// 将bakfilename里的路径替换成备份路径
        // 备份文件
        if(renamefile(filename,bakfilename)==false)
        {
            logfile.write("备份文件 %s 失败.\n",bakfilename.c_str());
        }
        //xxxxxxx else logfile.write("备份文件 %s 成功.\n",bakfilename.c_str());
    }
    return true;
}

bool clientlogin()
{
    if(tcpserver.read(strrecvbuf,60)==false)
    {
        logfile.write("tcpserver.read %s failed.\n",strrecvbuf.c_str());
    }
    //xxxxxxx else logfile.write("tcpserver.read %s success.\n",strrecvbuf.c_str());

    // 解析客户端登录报文,不需要进行合法性判断，客户端已经判断过了
    memset(&starg,0,sizeof(st_arg));
    getxmlbuffer(strrecvbuf,"clienttype",starg.clienttype);
    getxmlbuffer(strrecvbuf,"clientpath",starg.clientpath);
    getxmlbuffer(strrecvbuf,"srvpath",starg.srvpath);
    getxmlbuffer(strrecvbuf,"matchname",starg.matchname);
    getxmlbuffer(strrecvbuf,"ptype",starg.ptype);
    getxmlbuffer(strrecvbuf,"timeout",starg.timeout);
    getxmlbuffer(strrecvbuf,"pname",starg.pname);
    getxmlbuffer(strrecvbuf,"timetvl",starg.timetvl);
    getxmlbuffer(strrecvbuf,"clientpathbak",starg.clientpathbak);
    getxmlbuffer(strrecvbuf,"srvpathbak",starg.srvpathbak);

    if(starg.clienttype!=1 && starg.clienttype!=2)
    {
        strsendbuf="failed";
    }
    else strsendbuf="ok";

    if(tcpserver.write(strsendbuf)==false)
    {
        logfile.write("tcpserver.write %s failed.\n",strsendbuf.c_str());
    }
    else logfile.write("%s login %s\n",tcpserver.getip(),strsendbuf.c_str());

    return true;
}
