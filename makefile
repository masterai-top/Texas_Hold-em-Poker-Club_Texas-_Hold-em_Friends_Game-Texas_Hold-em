
#-----------------------------------------------------------------------

APP           := XGame
TARGET        := ActivityServer
CONFIG        := 
STRIP_FLAG    := N
TARS2CPP_FLAG := 
CFLAGS        += -lm
CXXFLAGS      += -lm

INCLUDE   += 
LIB       += 

INCLUDE   += -I/usr/local/cpp_modules/wbl/include
LIB       += -L/usr/local/cpp_modules/wbl/lib -lwbl

INCLUDE   += -I/usr/local/mysql/include
LIB       += -L/usr/local/mysql/lib/mysql -lmysqlclient

INCLUDE   += -I/usr/local/cpp_modules/protobuf/include
LIB       += -L/usr/local/cpp_modules/protobuf/lib -lprotobuf

INCLUDE	  += -L/usr/local/cpp_modules/maxminddb/include
LIB		  += -L/usr/local/cpp_modules/maxminddb/lib -lmaxminddb

#-----------------------------------------------------------------------
include /home/tarsproto/XGame/Comm/Comm.mk
include /home/tarsproto/XGame/ConfigServer/ConfigServer.mk
include /home/tarsproto/XGame/DBAgentServer/DBAgentServer.mk
include /home/tarsproto/XGame/util/util.mk
include /home/tarsproto/XGame/protocols/protocols.mk
include /home/tarsproto/XGame/RouterServer/RouterServer.mk
include /home/tarsproto/XGame/HallServer/HallServer.mk
include /home/tarsproto/XGame/RoomServer/RoomServer.mk
include /home/tarsproto/XGame/Log2DBServer/Log2DBServer.mk
include /home/tarsproto/XGame/AIServer/AIServer.mk
include /home/tarsproto/XGame/PushServer/PushServer.mk
include /usr/local/tars/cpp/makefile/makefile.tars

#-----------------------------------------------------------------------

xgame:
	cp -f $(TARGET) /usr/local/app/tars/tarsnode/data/XCommon.ActivityServer/bin/

100:
	sshpass -p 'awzs2022' scp ./ActivityServer  root@10.10.10.100:/home/yuj/server/activityserver
