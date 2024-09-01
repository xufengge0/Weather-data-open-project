# 用于结束数据共享平台的全部服务程序

# 结束调度程序
killall -9 procctl

# 尝试正常终止其他服务程序
killall crtsurfdata deletefiles gzipfiles ftpgetfiles ftpputfiles
killall fileserver tcpputfiles tcpgetfiles obtmindtodb dminingoracle

# 等待其他服务程序退出
sleep 5

# 不论是否退出，强制杀死
killall -9 crtsurfdata deletefiles gzipfiles ftpgetfiles ftpputfiles
killall -9 fileserver tcpputfiles tcpgetfiles obtmindtodb dminingoracle