# 用于启动数据共享平台的全部服务程序


# 生成气象站点观测的分钟数据，每分钟运行一次
/project/tools/cpp/procctl 60 /project/idc/bin/crtsurfdata /project/idc/ini/stcode.ini /tmp/idc/durfdata /log/idc/crtsurfdata.log csv,xml,json
# 清理原始气象观测数据目录（/tmp/idc/surfdata）中的历史数据
/project/tools/cpp/procctl 300 project/tools/cpp/deletefiles /tmp/idc/durfdata "*" 0.02

# 压缩后台服务程序的备份日志
/project/tools/cpp/procctl 300 project/tools/cpp/gzipfiles /log/idc "*.log.20*" 0.02

# 从/tmp/idc/surfdata目录采集数据文件，存放在/idcdata/surfdata目录
/project/tools/cpp/procctl 30 project/tools/cpp/ftpgetfiles /log/idc/ftpgetfiles_surfdata.log "<host>192.168.247.128:21</host><mode>1</mode><username>mysql</username><password>12345</password><remotepath>/tmp/idc/surfdata</remotepath><localpath>/idcdata/surfdata</localpath><matchname>*.csv,*.xml</matchname><ptype>1</ptype><remotepathbak>/tmp/idc/surfdatabak</remotepathbak><okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename><checktime>true</checktime><timeout>80</timeout><pname>ftpfileget_surfdata</pname>"
# 清理/idcdata/surfdata目录中0.04天（约1h）之前的文件
/project/tools/cpp/procctl 300 project/tools/cpp/deletefiles /idcdata/surfdata "*" 0.04

# 把/tmp/idc/surfdata目录的数据文件上传到/tmp/ftpputest目录
# 注意先创建服务端目录：mkdir /tmp/ftpputest
/project/tools/cpp/procctl 30 project/tools/cpp/ftpputfiles /log/idc/ftpgetfiles_surfdata.log "<host>192.168.247.128:21</host><mode>1</mode><username>mysql</username><password>12345</password><remotepath>/tmp/ftpputtest</remotepath><localpath>/tmp/idc/surfdata</localpath><matchname>*.json</matchname><ptype>1</ptype><localpathbak>/tmp/idc/surfdatabak</localpathbak><okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename><timeout>80</timeout><pname>ftpfileget_surfdata</pname>"
# 清理/tmp/ftpputest目录中0.04天（约1h）之前的文件
/project/tools/cpp/procctl 300 project/tools/cpp/deletefiles /tmp/ftpputest "*" 0.04

# 文件传输的服务端程序
/project/tools/cpp/procctl 10 project/tools/cpp/fileserver /log/idc/fileserver.log 5000
# 将目录/tmp/ftpputtest中的文件上传到/tmp/tcpputtest中
/project/tools/cpp/procctl 20 project/tools/cpp/tcpputfiles /log/idc/tcpputfiles.log "<ip>192.168.247.128</ip><port>5000</port><clientpath>/tmp/ftpputtest</clientpath><ptype>1</ptype><srvpath>/tmp/tcpputtest</srvpath><andchild>true</andchild><matchname>*.xml,*.csv,*.json</matchname><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>"
# 将目录/tmp/tcpputtest中文件下载到/tmp/tcpgetest中
/project/tools/cpp/procctl 20 project/tools/cpp/tcpgetfiles /log/idc/tcpgetfiles.log "<ip>192.168.247.128</ip><port>5000</port><clientpath>/tmp/tcpgetest</clientpath><ptype>1</ptype><srvpath>/tmp/tcpputtest</srvpath><andchild>true</andchild><matchname>*.xml,*.csv,*.json</matchname><timetvl>10</timetvl><timeout>50</timeout><pname>tcpgetfiles_surfdata</pname>"
# 清理/tmp/tcpgetest目录中0.04天（约1h）之前的文件
/project/tools/cpp/procctl 300 project/tools/cpp/deletefiles /tmp/tcpgetest "*" 0.04

# 把/idcdata/surfdata目录下的气象观测数据文件入库到T_ZHOBTMIND表中
/project/tools/cpp/procctl 10 /project/idc/cpp/obtmindtodb /idcdata/surfdata \"idc/idcpwd\" \"Simplified Chinese_China.AL32UTF8\" /log/idc/obtmindtodb.log
# 执行/project/idc/sql/deletetable.sql,删除表T_ZHOBTMIND两小时之前的数据
/project/tools/cpp/procctl 120 /oracle/home/bin/sqlplus idc/idcpwd @/project/idc/sql/deletetable.sql

# 每隔1小时把表T_ZHOBTCODE（站点数据）中数据全量抽取
/project/tools/cpp/procctl 3600 /project/tools/cpp/dminingoracle /log/idc/dminingoracle_ZHOBTCODE.log "<connstr>idc/idcpwd</connstr><charset>Simplified Chinese_China.AL32UTF8</charset><selectsql>select obtid,cityname,provname,lat,lon,height from T_ZHOBTCODE where obtid like '5%%%%'</selectsql><fieldstr>obtid,cityname,provname,lat,lon,height</fieldstr><fieldlen>5,30,30,10,10,10</fieldlen><bfilename>ZHOBTCODE</bfilename><efilename>togxpt</efilename><outpath>/idcdata/dmindata</outpath><timeout>30</timeout><pname>dminingoracle_ZHOBTCODE</pname>"
# 每隔30秒把表T_ZHOBTMIND（观测数据）中数据增量抽取
/project/tools/cpp/procctl 30 /project/tools/cpp/dminingoracle /log/idc/dminingoracle_ZHOBTMIND.log "<connstr>idc/idcpwd</connstr><charset>Simplified Chinese_China.AL32UTF8</charset><selectsql>select obtid,to_char(ddatetime,'yyyymmddhh24miss'),t,p,u,wd,wf,r,vis,keyid from T_ZHOBTMIND where keyid>:1 and obtid like '5%%%%'</selectsql><fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>5,19,8,8,8,8,8,8,8,15</fieldlen><bfilename>ZHOBTMIND</bfilename><efilename>togxpt</efilename><outpath>/idcdata/dmindata</outpath><starttime></starttime><incfield>keyid</incfield><incfilename>/idcdata/dmining/dminingoracle_ZHOBTMIND_togxpt.keyid</incfilename><timeout>30</timeout><pname>dminingoracle_ZHOBTMIND_togxpt</pname><maxcount>1000</maxcount><connstr1>scott/tiger</connstr1>"
# 每隔半小时清理/idcdata/dmindata目录中的文件
/project/tools/cpp/procctl 300 project/tools/cpp/deletefiles /idcdata/dmindata "*" 0.02


