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

# 编译参数
CFLAGS = -g

all:crtsurfdata obtcodetodb obtmindtodb

crtsurfdata:crtsurfdata.cpp
# -lm -lc 表示链接math库、标准库
	g++ $(CFLAGS) -o crtsurfdata crtsurfdata.cpp $(PUBINCL) $(PUBCPP) -lm -lc
# 将可执行文件复制到上一级目录的bin文件夹中。-f表示如果目标文件已存在，就覆盖它。
	cp -f crtsurfdata ../bin/.
	
obtcodetodb:obtcodetodb.cpp 
	g++ $(CFLAGS) -o obtcodetodb obtcodetodb.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP)
	cp obtcodetodb ../bin/.

obtmindtodb:obtmindtodb.cpp idcapp.cpp
	g++ $(CFLAGS) -o obtmindtodb obtmindtodb.cpp idcapp.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORALIB) $(ORALIBS) $(ORACPP)
	cp obtmindtodb ../bin/.

clean:
	rm -f crtsurfdata