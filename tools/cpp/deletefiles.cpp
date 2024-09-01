#include"_public.h"
using namespace idc;

void EXIT(int sig);

cpactive pactive;

int main(int argc,char* argv[])
{
    // 帮助信息
    if(argc != 4)
    {
        cout<<"Using:./deletefiles pathname matchstr timeout\n";
        cout<<"Example:./deletefiles /tmp/idc/durfdata \"*.csv,*.xml,*.json\" 0.04\n";
        cout<<R"(./deletefiles /tmp/idc/durfdata "*.csv,*.xml,*.json" 0.04)"<<endl;

        cout<<"这是一个工具程序，用于删除历史文件数据\n";
        cout<<"pathname:历史文件的所处目录，全路径名\n";
        cout<<"matchstr:文件的匹配规则\n";
        cout<<"timeout:需要删除多久之前的文件，可以是小数，单位-天\n";

        return 0;
    }

    // 添加心跳信息
    pactive.addpinfo(10,"deletefiles");

    // 忽略全部信号，关闭IO
    closeioandsignal();

    // 获取被归为历史文件数据的时间点（例如60分钟前这个时间点）
    string timepoint = ltime1("yyyymmddhh24miss",-(int)(stof(argv[3])*24*60*60));
    cout<<"历史时间:"<<timepoint<<endl;

    // 打开目录
    cdir dir;
    dir.opendir(argv[1],argv[2],10000,true,false);

    // 遍历目录，删除历史文件
    while (dir.readdir()) // 读取一个文件信息
    {
        if(dir.m_mtime < timepoint) // 判断是否为历史文件
        {
            remove(dir.m_ffilename.c_str()); // 删除文件
        }
    }
    
    return 0;
}
void EXIT(int sig)
{
    cout<<"sig:"<<sig<<endl;
    exit(0);
}