/* 
    用于模拟网上银行的客户端
 */
#include"_public.h"
using namespace idc;

ctcpclient tcpclient;

string strsendbuf;
string strrecvbuf;

bool biz001(); // 登录操作
bool biz002(); // 查询操作
bool biz003(); // 转账操作

int main(int argc,char* argv[])
{
    if (argc!=3)
    {
        cout<<"Using:./demo03 ip port\n";
        cout<<"Example:./demo03 192.168.247.128 5000\n";
        return -1;
    }

    if(tcpclient.connect(argv[1],stoi(argv[2]))==false)
    {
        cout<<"tcpclient.connect failed.\n"; return -1;
    }
    
    biz001(); // 登录操作

    biz002(); // 查询操作

    biz003(); // 转账操作

    return 0;
}

bool biz001()
{   
    strsendbuf = "<bizid>1</bizid><username>13912345678</username><password>12345</password>";
    if(tcpclient.write(strsendbuf)==false)
    {
        cout<<"tcpclient.write failed.\n"; return false;
    }
    cout<<"发送："<<strsendbuf<<endl;
    if(tcpclient.read(strrecvbuf)==false)
    {
        cout<<"tcpclient.read failed.\n"; return false;
    }
    cout<<"接收："<<strrecvbuf<<endl;
    return true;
}
bool biz002()
{
    strsendbuf = "<bizid>2</bizid><cardid>600300010000</cardid>";
    if(tcpclient.write(strsendbuf)==false)
    {
        cout<<"tcpclient.write failed.\n"; return false;
    }
    cout<<"发送："<<strsendbuf<<endl;
    if(tcpclient.read(strrecvbuf)==false)
    {
        cout<<"tcpclient.read failed.\n"; return false;
    }
    cout<<"接收："<<strrecvbuf<<endl;
    return true;
}
bool biz003()
{
    strsendbuf = "<bizid>3</bizid><cardid1>600300010000</cardid1><cardid2>600300010001</cardid2><je>8.8</je>";
    if(tcpclient.write(strsendbuf)==false)
    {
        cout<<"tcpclient.write failed.\n"; return false;
    }
    cout<<"发送："<<strsendbuf<<endl;
    if(tcpclient.read(strrecvbuf)==false)
    {
        cout<<"tcpclient.read failed.\n"; return false;
    }
    cout<<"接收："<<strrecvbuf<<endl;
    return true;
}