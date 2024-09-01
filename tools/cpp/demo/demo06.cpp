/* 
    用于模拟网上银行的服务端 增加了心跳报文
 */
#include"_public.h"
using namespace idc;

ctcpserver tcpserver;
clogfile logfile;

string strsendbuf;
string strrecvbuf;

void FathEXIT(int sig);
void ChildEXIT(int sig);

bool bizmain();// 主函数
bool biz001(); // 登录操作
bool biz002(); // 查询操作
bool biz003(); // 转账操作

int main(int argc,char* argv[])
{
    if (argc!=4)
    {
        cout<<"Using:./demo04 port logfile timeout\n";
        cout<<"Example:./demo04 5000 /log/idc/demo04.log 30\n";
        return -1;
    }

    signal(SIGINT,FathEXIT);signal(SIGTERM,FathEXIT);

    if(logfile.open(argv[2])==false)
    {
        cout<<"logfile.open failed.\n"; return -1;
    }

    if(tcpserver.initserver(stoi(argv[1]))==false)
    {
        logfile.write("tcpserver.initserver failed.\n"); return -1;
    }
    
    while (1)
    {
        if(tcpserver.accept()==false)
        {
            logfile.write("tcpserver.accept failed.\n"); FathEXIT(-1);
        }
        logfile.write("连接到IP：%s.\n",tcpserver.getip());

        if(fork()>0) {tcpserver.closeclient(); continue;}

        signal(SIGINT,ChildEXIT);signal(SIGTERM,ChildEXIT);

        tcpserver.closelisten();

        while (1)
        {
            if(tcpserver.read(strrecvbuf,stoi(argv[3]))==false)
            {
                logfile.write("tcpserver.read failed.\n"); ChildEXIT(0);
            }
            logfile.write("接收：%s.\n",strrecvbuf.c_str());

            bizmain();

            if(tcpserver.write(strsendbuf)==false)
            {
                logfile.write("tcpserver.write failed.\n"); ChildEXIT(0);
            }
            logfile.write("发送：%s.\n",strsendbuf.c_str());
            
        }
        ChildEXIT(0);
        
    }

    return 0;
}

void FathEXIT(int sig)
{
    signal(SIGINT,SIG_IGN);signal(SIGTERM,SIG_IGN);

    logfile.write("父进程退出，sig=%d\n",sig);

    tcpserver.closelisten();

    kill(0,15);

    exit(0);
}
void ChildEXIT(int sig)
{
    signal(SIGINT,SIG_IGN);signal(SIGTERM,SIG_IGN);

    logfile.write("子进程退出，sig=%d\n",sig);

    tcpserver.closeclient();
    
    exit(0);
}
bool bizmain()
{
    int bized;
    getxmlbuffer(strrecvbuf,"bizid",bized);
    switch (bized)
    {
    case 0:
        strsendbuf="<bizid>0</bizid>";
        break;
    case 1:
        biz001();
        break;
    case 2:
        biz002();
        break;
    case 3:
        biz003();
        break;
    default:
        strsendbuf="<retcode>9</retcode><message>非法报文</message>";
        break;
    }
    return true;
}
bool biz001()
{
    string username,password;
    getxmlbuffer(strrecvbuf,"username",username);
    getxmlbuffer(strrecvbuf,"password",password);
    if((username=="13912345678") && (password=="12345"))
    {
        strsendbuf="<retcode>0</retcode><message>登录成功</message>";
    }
    else strsendbuf="<retcode>-1</retcode><message>登录失败，用户名或密码不正确</message>";

    return true;
}
bool biz002()
{
    string cardid;
    getxmlbuffer(strrecvbuf,"cardid",cardid);
    if (cardid=="600300010000")
    {
        strsendbuf="<retcode>0</retcode><message>余额为：8.8</message>";
    }

    return true;
}
bool biz003()
{
    string cardid1,cardid2; double je;
    getxmlbuffer(strrecvbuf,"cardid1",cardid1);
    getxmlbuffer(strrecvbuf,"cardid2",cardid2);
    getxmlbuffer(strrecvbuf,"je",je);

    if(je<100.1)
    {
        strsendbuf="<retcode>0</retcode><message>转账成功</message>";
    }else strsendbuf="<retcode>-1</retcode><message>转账失败，余额不足</message>";

    return true;
}