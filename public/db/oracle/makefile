#oracle头文件路径
ORAINCL = -I$(ORACLE_HOME)/rdbms/public

#oracle库文件路径  -L是一个编译器选项，用于指定链接器查找库文件的路径, .表示当前目录。
ORALIB = -L$(ORACLE_HOME)/lib -L.

#oracle的oci库
ORALIBS = -lclntsh

CFLAGS = -O2 -Wall  #-Wall是一个警告选项，表示开启所有的警告信息。
CFLAGS = -g -Wall -Wno-write-strings #-Wno-write-strings表示关闭关于字符串常量赋值给char变量的警告。


all:createtable inserttable updatetable deletetable selecttable filetoclob clobtofile filetoblob blobtofile

createtable:createtable.cpp _ooci.h _ooci.cpp
	g++ $(CFLAGS) -o createtable createtable.cpp $(ORAINCL) $(ORALIB) $(ORALIBS) _ooci.cpp

inserttable:inserttable.cpp _ooci.h _ooci.cpp
	g++ $(CFLAGS) -o inserttable inserttable.cpp $(ORAINCL) $(ORALIB) $(ORALIBS) _ooci.cpp

updatetable:updatetable.cpp _ooci.h _ooci.cpp
	g++ $(CFLAGS) -o updatetable updatetable.cpp $(ORAINCL) $(ORALIB) $(ORALIBS) _ooci.cpp

deletetable:deletetable.cpp _ooci.h _ooci.cpp
	g++ $(CFLAGS) -o deletetable deletetable.cpp $(ORAINCL) $(ORALIB) $(ORALIBS) _ooci.cpp

selecttable:selecttable.cpp _ooci.h _ooci.cpp
	g++ $(CFLAGS) -o selecttable selecttable.cpp $(ORAINCL) $(ORALIB) $(ORALIBS) _ooci.cpp

filetoclob:filetoclob.cpp _ooci.h _ooci.cpp
	g++ $(CFLAGS) -o filetoclob filetoclob.cpp $(ORAINCL) $(ORALIB) $(ORALIBS) _ooci.cpp

clobtofile:clobtofile.cpp _ooci.h _ooci.cpp
	g++ $(CFLAGS) -o clobtofile clobtofile.cpp $(ORAINCL) $(ORALIB) $(ORALIBS) _ooci.cpp

filetoblob:filetoblob.cpp _ooci.h _ooci.cpp
	g++ $(CFLAGS) -o filetoblob filetoblob.cpp $(ORAINCL) $(ORALIB) $(ORALIBS) _ooci.cpp

blobtofile:blobtofile.cpp _ooci.h _ooci.cpp
	g++ $(CFLAGS) -o blobtofile blobtofile.cpp $(ORAINCL) $(ORALIB) $(ORALIBS) _ooci.cpp
clean:
	rm -rf createtable inserttable updatetable deletetable selecttable filetoclob clobtofile filetoblob blobtofile




