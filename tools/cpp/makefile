# 开发框架头文件路径
PUBINCL = -I/project/public

# 开发框架cpp文件名
PUBCPP = /project/public/_public.cpp

##################################
#oracle头文件路径
ORAINCL = -I$(ORACLE_HOME)/rdbms/public -I//project/public/db/oracle

#oracle库文件路径  -L是一个编译器选项，用于指定链接器查找库文件的路径, .表示当前目录。
ORALIB = -L$(ORACLE_HOME)/lib -L.

#oracle的oci库
ORALIBS = -lclntsh

#开发框架Oracle的cpp文件名，这里直接包含进来，方便调试
ORACPP = /project/public/db/oracle/_ooci.cpp
##################################

all:procctl server checkproc deletefiles gzipfiles ftpgetfiles ftpputfiles tcpputfiles tcpgetfiles fileserver\
	dminingoracle xmltodb deletetable migratetable syncref syncinc inetd webserver webserver1

procctl:procctl.cpp
	g++ -g -o procctl procctl.cpp $(PUBINCL) $(PUBCPP)

server: server.cpp
	g++ -g -o server server.cpp $(PUBINCL) $(PUBCPP)

checkproc: checkproc.cpp
	g++ -g -o checkproc checkproc.cpp $(PUBINCL) $(PUBCPP)

deletefiles: deletefiles.cpp
	g++ -g -o deletefiles deletefiles.cpp $(PUBINCL) $(PUBCPP)

gzipfiles:gzipfiles.cpp
	g++ -g -o gzipfiles gzipfiles.cpp $(PUBINCL) $(PUBCPP)

ftpgetfiles:ftpgetfiles.cpp
	g++ -g -o ftpgetfiles ftpgetfiles.cpp $(PUBINCL) $(PUBCPP) /project/public/libftp.a /project/public/_ftp.cpp

ftpputfiles:ftpputfiles.cpp
	g++ -g -o ftpputfiles ftpputfiles.cpp $(PUBINCL) $(PUBCPP) /project/public/libftp.a /project/public/_ftp.cpp

tcpputfiles:tcpputfiles.cpp
	g++ -g -o tcpputfiles tcpputfiles.cpp $(PUBINCL) $(PUBCPP)

tcpgetfiles:tcpgetfiles.cpp
	g++ -g -o tcpgetfiles tcpgetfiles.cpp $(PUBINCL) $(PUBCPP)

fileserver:fileserver.cpp
	g++ -g -o fileserver fileserver.cpp $(PUBINCL) $(PUBCPP)

dminingoracle:dminingoracle.cpp
	g++ -g -o dminingoracle dminingoracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP)

xmltodb:xmltodb.cpp _tools.cpp
	g++ -g -o xmltodb xmltodb.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) _tools.cpp

deletetable:deletetable.cpp
	g++ -g -o deletetable deletetable.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP)

migratetable:migratetable.cpp _tools.cpp
	g++ -g -o migratetable migratetable.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) _tools.cpp

syncref:syncref.cpp _tools.cpp
	g++ -g -o syncref syncref.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) _tools.cpp

syncinc:syncinc.cpp _tools.cpp
	g++ -g -o syncinc syncinc.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) _tools.cpp

inetd:inetd.cpp
	g++ -g -o inetd inetd.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP)

webserver:webserver.cpp
	g++ -g -o webserver webserver.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) -lpthread

webserver1:webserver1.cpp
	g++ -g -o webserver1 webserver1.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP) -lpthread
clean:
	rm -f procctl server checkproc deletefiles gzipfiles ftpgetfiles dminingoracle xmltodb 