#include"_public.h"
using namespace idc;


cpactive pactive;

int main(int argc,char* argv[])
{
    // 帮助信息
    if(argc != 4)
    {
        cout<<"Using:./gzipfiles pathname matchstr timeout\n";
        cout<<"Example:./gzipfiles /tmp/idc/durfdata \"*.csv,*.xml,*.json\" 0.04\n";
        cout<<R"(./gzipfiles /tmp/idc/durfdata "*.csv,*.xml,*.json" 0.04)"<<endl;

        cout<<"这是一个工具程序，用于压缩文件\n";
        cout<<"pathname:文件的所处目录，全路径名\n";
        cout<<"matchstr:文件的匹配规则\n";
        cout<<"timeout:需要压缩多久之前的文件，可以是小数，单位-天\n";

        return 0;
    }

    // 添加心跳信息
    pactive.addpinfo(120,"gzipfiles"); // 压缩文件比较耗时，超时时间应设置大一些

    // 忽略全部信号，关闭IO
    closeioandsignal();

    // 获取被归为压缩文件数据的时间点（例如60分钟前这个时间点）
    string timepoint = ltime1("yyyymmddhh24miss",-(int)(stof(argv[3])*24*60*60));
    cout<<"历史时间:"<<timepoint<<endl;

    // 打开目录
    cdir dir;
    dir.opendir(argv[1],argv[2],10000,true,false);

    // 遍历目录，压缩历史文件
    while (dir.readdir()) // 读取一个文件信息
    {
        if((dir.m_mtime < timepoint) && (matchstr(dir.m_filename,"*.gz")==false)) // 判断是否为历史文件，并且不是压缩文件
        {
            // 拼接压缩命令    注意"gzip -f "命令的空格！！！
            string cmd = "gzip -f " + dir.m_ffilename + "1>/dev/null2>/dev/null"; // 不显示任何东西
            system(cmd.c_str());
        }
    
        // 更新进程的心跳
        pactive.uptatime();

    }
    
    return 0;
}
