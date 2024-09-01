/* 
    用于生成气象站点观测的分钟数据
*/
#include "_public.h"
using namespace idc;

struct st_code // 存放站点数据结构体
{
    char province[31];
    char obtid[11];
    char obtname[31];
    double lat;
    double lon;
    double heigth;
};
list<st_code> stlist; // 存放站点参数的容器
bool loadstcode(const string &inifile); // 将站点参数放入容器

// 站点观测数据的结构体
struct st_surfdata
{
    char obtid[11]; // 站点编号
    char ddatetime[15]; // 数据时间：格式yyyymmddhh24miss,精确到分钟，秒固定为00
    int t; // 气温：0.1摄氏度
    int p; // 气压：0.1百帕
    int u; // 相对湿度 0-100
    int wd; // 风向 0-360
    int wf; // 风速 单位0.1m/s
    int r; // 降雨量 0.1mm
    int vis; // 能见度 0.1m
};
list<st_surfdata> datalist; // 存放站点观测数据的容器
void crtsurfdata(); // 根据站点数据生成观测数据的函数

// 获取当前时间，精确到分钟
char strddatetime[15];

// 将datalist中的数据写入文件，outpath-文件存放目录；datafmt-文件存储格式，支持csv、xml、json
bool crtsurffile(const string& outpath,const string& datafmt);

clogfile logfile; // 创建日志对象

void EXIT(int sig); // 信号处理函数

cpactive pactive; // 创建进程心跳对象

int main(int argc,char* argv[])
{
    if(argc!=5)
    {
        cout<<"Using:./crtsurfdata infile outpath logfile datafmt\n";
        cout<<"Sample:/project/tools/cpp/procctl 60 /project/idc/bin/crtsurfdata /project/idc/ini/stcode.ini /tmp/idc/durfdata /log/idc/crtsurfdata.log csv,xml,json\n";

        cout<<"本程序由调度程序procctl启动, 每分钟更新一次\n";
        cout<<"infile 气象站点文件名\n";
        cout<<"outpath 气象站点数据文件存放目录\n";
        cout<<"logfile 本程序运行日志文件名\n";
        cout<<"datafmt 观测数据文件格式(csv,xml,json)\n";

        return -1;
    }
    // 把心跳加入共享内存
    pactive.addpinfo(10,"crtsurfdata");

    // 忽略所有信号、关闭IO
    closeioandsignal(true);
    // 处理打断信号ctrl c（2）、killall（15）
    signal(SIGINT,EXIT);signal(SIGTERM,EXIT);

    if(logfile.open(argv[3]) == false)
    {
        cout<<"日志打开失败。"<<endl; return false;
    }
    logfile.write("程序开始运行...\n");

    // 编写处理业务代码
    // 1)加载站点参数到stlist容器中
    if(loadstcode(argv[1])==false) EXIT(-1);
    
    // 获取当前时间，精确到分钟
    memset(&strddatetime,0,sizeof(strddatetime));
    ltime(strddatetime,"yyyymmddhh24miss");
    strncpy(strddatetime+12,"00",2); // 最后两位替换为00
    
    // 2)根据stlist容器中站点数据，生成观测数据（随机数）， 生成的数据放到datalist容器中
    crtsurfdata();

    // 3)将datalist中的数据写入文件，outpath-文件存放目录；datafmt-文件存储格式，支持csv、xml、json
    // strstr用于判断argv[4]中是否存在相同字符串
    if(strstr(argv[4],"csv")) crtsurffile(argv[2],"csv");
    if(strstr(argv[4],"xml")) crtsurffile(argv[2],"xml");
    if(strstr(argv[4],"json")) crtsurffile(argv[2],"json");

    logfile.write("程序运行结束。\n");

    return 0;
}


void EXIT(int sig){

    logfile.write("程序退出，信号=%d\n",sig);
}
// 加载站点参数到容器中
bool loadstcode(const string &inifile)
{
    cifile ifile; // 读文件的对象
    if (ifile.open(inifile)==false)
    {
        logfile.write("open %s failed.",inifile.c_str()); // 注意C风格字符串
    }

    string strbuffer; // 存放读取的数据
    ccmdstr cmdstr; // 字符串拆分的类
    st_code stcode; // 存放拆分数据的结构体
    ifile.readline(strbuffer); // 读取标题 舍弃

    while (ifile.readline(strbuffer)) // 读取整个文件
    {
        // logfile.write("strbuffer=%s\n",strbuffer.c_str()); // 调试代码

        // 把每一行进行拆分 安徽,58424,安庆,30.37,116.58,62
        cmdstr.splittocmd(strbuffer,",");
        // 初始化结构体
        memset(&stcode,0,sizeof(stcode));
        // 将一行数据写入结构体中
        cmdstr.getvalue(0,stcode.province); // 省
        cmdstr.getvalue(1,stcode.obtid);    // 站号
        cmdstr.getvalue(2,stcode.obtname);  // 站名
        cmdstr.getvalue(3,stcode.lon);      // 经度
        cmdstr.getvalue(4,stcode.lat);      // 纬度
        cmdstr.getvalue(5,stcode.heigth);   // 海拔
        // 将结构体存入容器
        stlist.push_back(stcode);

    }
    /* for (auto i:stlist) // 调试代码
    {
        logfile.write("province=%s obtname=%s obtid=%s lon=%f lat=%f heigth=%f\n",i.province,
        i.obtname,i.obtid,i.lon,i.lat,i.heigth);
    } */
    return true;
}
// 根据站点数据生成观测数据的函数
void crtsurfdata()
{   
    srand(time(0)); // 播随机数种子

    st_surfdata stsurfdata; // 观测数据的结构体

    for(auto i:stlist)
    {
        memset(&stsurfdata,0,sizeof(stsurfdata));

        strcpy(stsurfdata.obtid,i.obtid);           // 站点编号
        strcpy(stsurfdata.ddatetime,strddatetime);  // 数据时间：格式yyyymmddhh24miss,精确到分钟，秒固定为00
        stsurfdata.t=rand()%350;            // 气温：0.1摄氏度
        stsurfdata.p=rand()%265 + 10000;    // 气压：0.1百帕
        stsurfdata.u=rand()%101;            // 相对湿度 0-100
        stsurfdata.wd=rand()%360;           // 风向 0-360
        stsurfdata.wf=rand()%150;           // 风速 单位0.1m/s
        stsurfdata.r=rand()%16;             // 降雨量 0.1mm
        stsurfdata.vis=rand()%5001 + 100000;// 能见度 0.1m

        // 将生成的数据放入容器中
        datalist.push_back(stsurfdata);
    }
    /* for(auto i:datalist) // 调试代码
    {
        logfile.write("%s %s %d %d %d %d %d %d %d\n",i.obtid,
        i.ddatetime,i.t,i.p,i.u,i.wd,i.wf,i.r,i.vis);
    } */
    return ;
}
// 将datalist中的数据写入文件，outpath-文件存放目录；datafmt-文件存储格式，支持csv、xml、json
bool crtsurffile(const string& outpath,const string& datafmt)
{   
    // 拼接文件名
    string filename = outpath + "/" + "SURF_ZH_" + strddatetime + "_" + to_string(getpid()) + "." +datafmt;
    cofile ofile;
    if(ofile.open(filename)==false)
    {   
        logfile.write("打开文件%s失败",filename.c_str()); // 切记不要手动root用户建目录！！！
        return false;
    }
    if(datafmt=="csv") ofile.writeline("站点编号 数据时间 气温 气压 相对湿度 风向 风速 降雨量 能见度\n");
    if(datafmt=="xml") ofile.writeline("<data>\n");
    if(datafmt=="json") ofile.writeline("{\"<data>\":[\n");

    for(auto i:datalist)
    {   
        if(datafmt=="csv") ofile.writeline("%s,%s,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f\n",i.obtid,i.ddatetime,
                                            i.t/10.0,i.p/10.0,i.u,i.wd,i.wf/10.0,i.r/10.0,i.vis/10.0);
        
        if(datafmt=="xml") ofile.writeline("<obtid>%s</obtid><ddatetime>%s</ddatetime><t>%.1f</t><p>%.1f</p>"
                                            "<u>%d</u><wd>%d</wd><wf>%.1f</wf><r>%.1f</r><vis>%.1f</vis><endl/>\n",
                                            i.obtid,i.ddatetime,i.t/10.0,i.p/10.0,i.u,i.wd,i.wf/10.0,i.r/10.0,i.vis/10.0);
        
        if(datafmt=="json") 
        {
            ofile.writeline("{\"obtid\":\"%s\"," // ！！！前后都加 " 分行
                            "\"ddatetime\":\"%s\","
                            "\"t\":\"%.1f\","
                            "\"p\":\"%.1f\","
                            "\"u\":\"%d\","
                            "\"wd\":\"%d\","
                            "\"wf\":\"%.1f\","
                            "\"r\":\"%.1f\","
                            "\"vis\":\"%.1f\"}",
                            i.obtid,i.ddatetime,i.t/10.0,i.p/10.0,i.u,i.wd,i.wf/10.0,i.r/10.0,i.vis/10.0);
            // json文件最后一条记录，不需要逗号
            static int i=0; // 写入行数计数器
            if(i<datalist.size()-1)
            {   // 不是最后一行时
                ofile.writeline(",\n"); i++;
            }
            else ofile.writeline("\n");
        }
        
    }
    if(datafmt=="xml") ofile.writeline("</data>\n");
    if(datafmt=="json") ofile.writeline("]}\n");

    ofile.closeandrename();

    logfile.write("生成文件%s成功,数据时间%s,数据大小%d行\n",filename.c_str(),strddatetime,datalist.size());
    return true;
}