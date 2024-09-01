
#include"idcapp.h"

// 把文件读取的一行拆分到m_stzhobtmind结构体中
bool CZHOBTMIND::splitbuffer(const string &strline,const bool bisxml)
{
    if(bisxml==true)
    {
        memset(&m_stzhobtmind,0,sizeof(m_stzhobtmind));

        // 解析行的内容(xml),存放到结构体中
        getxmlbuffer(strline,"obtid",m_stzhobtmind.obtid,5);
        getxmlbuffer(strline,"ddatetime",m_stzhobtmind.ddatetime,14);
        getxmlbuffer(strline,"t",m_stzhobtmind.t,10);
        getxmlbuffer(strline,"p",m_stzhobtmind.p,10);
        getxmlbuffer(strline,"u",m_stzhobtmind.u,10);
        getxmlbuffer(strline,"wd",m_stzhobtmind.wd,10);
        getxmlbuffer(strline,"wf",m_stzhobtmind.wf,10);
        getxmlbuffer(strline,"r",m_stzhobtmind.r,10);
        getxmlbuffer(strline,"vis",m_stzhobtmind.vis,10);

    }
    else
    {   // 解析行的内容(csv),存放到结构体中
        ccmdstr cmdstr;
        cmdstr.splittocmd(strline,",");
        cmdstr.getvalue(0,m_stzhobtmind.obtid,5);
        cmdstr.getvalue(1,m_stzhobtmind.ddatetime,14);
        cmdstr.getvalue(2,m_stzhobtmind.t,10);
        cmdstr.getvalue(3,m_stzhobtmind.p,10);
        cmdstr.getvalue(4,m_stzhobtmind.u,10);
        cmdstr.getvalue(5,m_stzhobtmind.wd,10);
        cmdstr.getvalue(6,m_stzhobtmind.wf,10);
        cmdstr.getvalue(7,m_stzhobtmind.r,10);
        cmdstr.getvalue(8,m_stzhobtmind.vis,10);
    }
    m_buf=strline;
    return true;
}

// 把读取的数据插入到表中
bool CZHOBTMIND::inserttable()
{
    // 只绑定一次
    if(m_stmtins.isopen()==false)
    {
        m_stmtins.connect(&m_conn);
        m_stmtins.prepare("\
            insert into T_ZHOBTMIND(obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid)\
                        values(:1,to_date(:2,'yyyymmddhh24miss'),:3*10,:4*10,:5,:6,:7*10,:8*10,:9*10,SEQ_ZHOBTCODE.nextval)");
            // 绑定变量
            m_stmtins.bindin(1,m_stzhobtmind.obtid,5);
            m_stmtins.bindin(2,m_stzhobtmind.ddatetime,14);
            m_stmtins.bindin(3,m_stzhobtmind.t,10);
            m_stmtins.bindin(4,m_stzhobtmind.p,10);
            m_stmtins.bindin(5,m_stzhobtmind.u,10);
            m_stmtins.bindin(6,m_stzhobtmind.wd,10);
            m_stmtins.bindin(7,m_stzhobtmind.wf,10);
            m_stmtins.bindin(8,m_stzhobtmind.r,10);
            m_stmtins.bindin(9,m_stzhobtmind.vis,10);
    }

    // 执行SQL语句，向数据库中插入数据
    if(m_stmtins.execute()!=0)
    {
        // 发生非重复记录的错误
        if(m_stmtins.rc()!=1) 
        {
            m_logfile.write("buf=%s\n",m_buf.c_str());
            m_logfile.write("stmtins.execute() failed.\n%s\n",m_stmtins.message());
        }
        return false;
    }
    
    return true;
}