/* 
    守护模块
 */
#include"_public.h"
using namespace idc;

int main(int argc,char* argv[])
{
    if (argc!=2)
    {
        cout<<"Using:/checkproc logfilename\n";
        cout<<"Example:/project/tools/bin/procctl 10 /project/tools/cpp/checkproc /tmp/log/checkproc.log\n";
    
        cout<<"本程序用检查后台程序是否超时，如果超时，就终止他\n";
        cout<<"注意：本程序由调度程序procctl启动，周期建议为10秒\n";
        cout<<"注意：为避免被普通程序误杀，本程序应该用root用户启动\n";
        cout<<"注意：如果要终止本程序，只能killall -9杀死\n";
        
        return 0;
    }

    // 打开日志文件
    clogfile logfile;
    if(logfile.open(argv[1])==false)
    {
        cout << "logfile.open "<<argv[1]<<" failed.\n"; return -1;
    }

    // 获取共享内存,键值为SHMKEYP,大小为MAXNUMP个st_procinfo个结构体
    int shmid = shmget(SHMKEYP,MAXNUMP*sizeof(st_procinfo),0666|IPC_CREAT); // 先删除原申请的共享内存！！！
    if(shmid==-1)
    {
        logfile.write("获取共享内存%x失败.\n",SHMKEYP); // 注意输出16进制！！
    }

    // 将共享内存链接到当前进程
    st_procinfo *shm = (st_procinfo *)shmat(shmid,0,0);

    // 调试代码 用于显示已分配的共享内存信息
    for (int i = 0; i < 1000; i++)
    {
        if(shm[i].pid!=0)
        {
            cout<<"i="<<i<<" pid:"<<shm[i].pid<<" name:"<<shm[i].pname<<" timeout:"<<shm[i].timeout<<" atime:"<<shm[i].atime<<endl;
        }
    }

    
    // 检查共享内存中的进程，若进程已超时，则终止他
    for (int i = 0; i < MAXNUMP; i++)
    {   
        // 进程不存在时，跳到下次循环
        if(shm[i].pid==0) continue;

        // 当进程不存在了，但在共享内存中还残留心跳信息
        // 向进程发送0的信号，判断是否存在，不存在就删掉他的心跳信息
        if(kill(shm[i].pid,0)==-1)
        {
            logfile.write("进程不存在:pid=%d,name=%s;已清理残留心跳信息\n",shm[i].pid,shm[i].pname);
            memset(&shm[i],0,sizeof(st_procinfo));
            continue;
        }

        // 进程存在时，判断是否超时
        time_t now = time(0);

        if ((now-shm[i].atime) > shm[i].timeout)
        {
            // 将进程的结构体备份出来，防止之后可能kill -9误杀掉自己
            st_procinfo tmp =shm[i];

            logfile.write("进程超时:pid=%d,name=%s\n",shm[i].pid,shm[i].pname);

            // 如果已超时，尝试发送信号15终止进程
            kill(tmp.pid,15);

            int iret;
            for(int j=0;j<5;j++)  // 每5秒判断一次进程是否被终止
            {
                sleep(1);
                iret = kill(tmp.pid,0);
                if(iret==-1) break;
            }

            if (iret==-1) // 成功终止，写入日志
            {
                logfile.write("成功终止进程:pid=%d,name=%s\n",tmp.pid,tmp.pname);
            }
            else  // 若还没终止，使用kill -9强行杀死
            {
                kill(tmp.pid,9);
                logfile.write("进程被强制杀死:pid=%d,name=%s\n",tmp.pid,tmp.pname);
                // 删除共享内存中的心跳信息
                memset(&shm[i],0,sizeof(st_procinfo));
            }
            
            
        }
        
        
        
    }
    

    // 将共享内存从当前进程中分离
    shmdt(shm);
    
    
    return 0;
}