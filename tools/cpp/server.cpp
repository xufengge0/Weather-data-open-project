/* 
    进程心跳程序
 */

#include<_public.h>
using namespace idc;

// 进程心跳的结构体（用于放入共享内存中）
struct stprocinfo
{
    int pid=0;          // 进程id
    char name[21]={0};  // 进程名称
    int timeout=0;      // 超时时间
    time_t atime=0;     // 最后更新时间

    // 增加构造函数时，会自动关闭默认构造函数，需要时手动启用
    stprocinfo()=default;
    stprocinfo(const int pid,const string in_name,const int timeout,const time_t atime)
        :pid(pid),timeout(timeout),atime(atime)
        {
        strcpy(name,in_name.c_str());
        }
};

int shmid=-1; // 共享内存的id
stprocinfo *m_shm=nullptr; // 结构体在共享内存中的地址
int m_pos=-1; // 结构体在共享内存中的位置

void EXIT(int sig);

int main()
{
    // 处理进程退出信号
    signal(SIGINT,EXIT);signal(SIGTERM,EXIT);

    // 创建共享内存           数组长度设为1000
    shmid=shmget(0x5095, 1000*sizeof(stprocinfo), 0640|IPC_CREAT);
    if ( shmid ==-1 )
    {
        cout << "shmget(0x5005) failed.\n"; return -1;
    }

    // 将共享内存链接到当前进程的地址空间
    m_shm = (stprocinfo*)shmat(shmid,0,0);
    if ( m_shm==(void *)-1 )
    {
        cout << "shmat() failed\n"; return -1;
    }

    // 把当前进程信息填入结构体中
    stprocinfo procinfo(getpid(),"server1",30,time(0));

    // 当程序意外终止（段错误、kill -9），没有清理心跳信息，进程信息会残留在共享内存中
    // 当系统分配的进程id（会循环分配）与残留的进程id相同时，应该重用这个位置
    for (int i = 0; i < 1000; i++)
    {
        if(m_shm[i].pid=procinfo.pid)
        {
            cout<<"分配的地址为"<<i<<endl;
            memcpy(&m_shm[i],&procinfo,sizeof(procinfo));
            m_pos = i;
            break;
        }
    }
    csemp csemp;
    // 一般情况时，共享内存中没有残留的信息
    if(m_pos=-1)
    {
        // 在共享内存中找一个位置，把结构体存入其中
        for (int i = 0; i < 1000; i++)
        {
            if(m_shm[i].pid==0)
            {
                cout<<"分配的地址为"<<i<<endl;
                memcpy(&m_shm[i],&procinfo,sizeof(procinfo));
                m_pos = i;
                break;
            }
        }
        if(m_pos==-1) cout<<"分配地址失败"<<endl;
    }
    

    // 调试代码 用于显示已分配的共享内存信息
    for (int i = 0; i < 1000; i++)
    {
        if(m_shm[i].pid!=0)
        {
            cout<<"i="<<i<<" pid:"<<m_shm[i].pid<<" name:"<<m_shm[i].name<<" timeout:"<<m_shm[i].timeout<<" atime:"<<m_shm[i].atime<<endl;
        }
    }

    while (1)
    {
        cout<<"程序运行中..."<<endl;
        sleep(20);

        // 更新进程的心跳信息
        m_shm[m_pos].atime=time(0);
    }


    return 0;
}
void EXIT(int sig)
{   
    cout<<"信号:"<<sig<<endl;

    // 从共享内存中删除心跳信息
    if(m_pos!=-1) memset(&m_shm[m_pos],0,sizeof(stprocinfo));

    // 将共享内存分离
    if(shmid!=-1) shmdt(m_shm);

    exit(0);
}