

# 启动守护模块
/project/tools/cpp/procctl 10 /project/tools/cpp/checkproc /tmp/log/checkproc.log

# 启动气象站点观测数据程序
/project/tools/cpp/procctl 60 /project/idc/cpp/crtsurfdata /project/idc/ini/stcode.ini /tmp/idc/durfdata /log/idc/crtsurfdata.log csv,xml,json