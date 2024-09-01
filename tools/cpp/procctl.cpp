/* 
    服务调度程序
 */
#include "_public.h"
#include <sys/wait.h> // wait函数头文件
//using namespace idc;

int main(int argc,char* argv[])
{
    if(argc<3)
    {
        cout<<"Using:./procctl timetvl program argv...\n";
        cout<<"Example:/project/tools/bin/procctl 10 /user/bin/tar \n";
        cout<<"Example:/project/tools/cpp/procctl 60 /project/idc/bin/crtsurfdata /project/idc/ini/stcode.ini /tmp/idc/durfdata /log/idc/crtsurfdata.log csv,xml,json\n \n";

        cout<<"本程序是服务程序的调度程序,周期性启动服务程序或shell脚本\n";
        cout<<"timetvl为运行周期,单位秒;被调度程序结束后c秒后重新启动\n";
        cout<<"周期性程序:timetvl为运行周期\n";
        cout<<"常驻程序:timetvl需要小于5秒\n";
        cout<<"program为被调度程序的全路径名\n";
        cout<<"argv...为被调度程序的参数\n";
        cout<<"本程序不会被kill杀死,可以用kill -9强行杀死\n";
        return -1;

    }

    // 忽略所有信号，关闭所有IO，表示不希望被打扰
    // 1）防止调度程序被杀死，不处理退出信号；2）关闭调度程序的IO，被调度的程序IO也会被关闭
    for (int i = 0; i < 64; i++)
    {
        signal(i,SIG_IGN);close(i);
    }

    // 父进程退出，子进程由1号进程管理，不受shell干预
    if(fork()!=0) exit(0);

    // 恢复子进程被中断的信号，使父进程可以用wait处理
    signal(SIGCHLD,SIG_DFL); // SIGCHLD信号的默认动作是忽略，但它可以被父进程捕获并处理，

    // 定义一个指针数组，存放被调度的命令行参数
    char *pargv[argc];
    for (int i = 2; i < argc; i++)
    {
        pargv[i-2] = argv[i];
    }
    pargv[argc-2]=nullptr; // 用于参数结束标志

    // 调度程序的关键代码
    while (1)
    {
        if(fork()==0) // 创建子进程用于调度程序
        {
            execv(argv[2],pargv);  // 如果execv成功，子进程将被新程序替换。替换后新程序退出会通知父进程
            exit(0); // 调度失败时执行
        }
        else // 父进程等待子进程中止
        {
            int status;
            wait(&status); // 阻塞等待子进程终止信号
            sleep(stoi(argv[1])); // 等待timetvl秒后重启

        }
    }
    

    return 0;
}