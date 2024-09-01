/* 
    共享平台项目公用函数和类的声明文件
 */

#include"_public.h" // 开发框架
#include"_ooci.h"   // 数据库操作
using namespace idc;

// 气象观测数据文件入库的类
class CZHOBTMIND
{
private:

    struct st_zhobtmind // 站点观测数据的结构体
    {
        char obtid[6]; // 站点编号
        char ddatetime[21]; // 数据时间：格式yyyymmddhh24miss,精确到分钟
        char t[11]; // 气温：0.1摄氏度
        char p[11]; // 气压：0.1百帕
        char u[11]; // 相对湿度 0-100
        char wd[11]; // 风向 0-360
        char wf[11]; // 风速 单位0.1m/s
        char r[11]; // 降雨量 0.1mm
        char vis[11]; // 能见度 0.1m

    } m_stzhobtmind;

    clogfile &m_logfile;    // 创建日志对象的引用，此时引用不占任何内存空间，避免创建新的clogfile对象
    connection &m_conn;     // 创建数据库连接类对象的引用

    sqlstatement m_stmtins;
    string m_buf;

public:
    CZHOBTMIND(connection &conn,clogfile &logfile):m_conn(conn),m_logfile(logfile){}
    ~CZHOBTMIND(){}

    // 把文件读取的一行拆分到m_stzhobtmind结构体中
    bool splitbuffer(const string &strline,const bool bisxml);

    // 把读取的数据插入到表中
    bool inserttable();
};