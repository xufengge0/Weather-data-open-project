/* 
    用于模拟网上银行的客户端 增加了心跳报文
 */
#include"_public.h"
using namespace idc;

ctcpclient tcpclient;

string strsendbuf;
string strrecvbuf;

bool biz000(string timeout); // 心跳报文
bool biz001(string timeout); // 登录操作
bool biz002(string timeout); // 查询操作
bool biz003(string timeout); // 转账操作

int main(int argc,char* argv[])
{
    if (argc!=4)
    {
        cout<<"Using:./demo03 ip port timeout\n";
        cout<<"Example:./demo03 192.168.247.128 5000 30\n";
        return -1;
    }

    if(tcpclient.connect(argv[1],stoi(argv[2]))==false)
    {
        cout<<"tcpclient.connect failed.\n"; return -1;
    }
    
    biz001(argv[3]); // 登录操作

    biz002(argv[3]); // 查询操作
    sleep(29);
    biz000(argv[3]); // 心跳报文

    biz003(argv[3]); // 转账操作


    return 0;
}

bool biz000(string timeout)
{
    strsendbuf = "<bizid>0</bizid>";
    if(tcpclient.write(strsendbuf)==false)
    {
        cout<<"tcpclient.write failed.\n"; return false;
    }
    cout<<"发送："<<strsendbuf<<endl;
    if(tcpclient.read(strrecvbuf,stoi(timeout))==false)
    {
        cout<<"tcpclient.read failed.\n"; return false;
    }
    cout<<"接收："<<strrecvbuf<<endl;
    return true;
}
bool biz001(string timeout)
{   
    strsendbuf = "<bizid>1</bizid><username>13912345678</username><password>12345</password>";
    if(tcpclient.write(strsendbuf)==false)
    {
        cout<<"tcpclient.write failed.\n"; return false;
    }
    cout<<"发送："<<strsendbuf<<endl;
    if(tcpclient.read(strrecvbuf,stoi(timeout))==false)
    {
        cout<<"tcpclient.read failed.\n"; return false;
    }
    cout<<"接收："<<strrecvbuf<<endl;
    return true;
}
bool biz002(string timeout)
{
    strsendbuf = "<bizid>2</bizid><cardid>600300010000</cardid>";
    if(tcpclient.write(strsendbuf)==false)
    {
        cout<<"tcpclient.write failed.\n"; return false;
    }
    cout<<"发送："<<strsendbuf<<endl;
    if(tcpclient.read(strrecvbuf,stoi(timeout))==false)
    {
        cout<<"tcpclient.read failed.\n"; return false;
    }
    cout<<"接收："<<strrecvbuf<<endl;
    return true;
}
bool biz003(string timeout)
{
    strsendbuf = "<bizid>3</bizid><cardid1>600300010000</cardid1><cardid2>600300010001</cardid2><je>8.8</je>";
    if(tcpclient.write(strsendbuf)==false)
    {
        cout<<"tcpclient.write failed.\n"; return false;
    }
    cout<<"发送："<<strsendbuf<<endl;
    if(tcpclient.read(strrecvbuf,stoi(timeout))==false)
    {
        cout<<"tcpclient.read failed.\n"; return false;
    }
    cout<<"接收："<<strrecvbuf<<endl;
    return true;
}