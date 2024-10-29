#include "ActivityServer.h"
#include "ActivityServantImp.h"

using namespace std;

ActivityServer g_app;

/////////////////////////////////////////////////////////////////
void
ActivityServer::initialize()
{
    //initialize application here:
    //...

    addServant<ActivityServantImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".ActivityServantObj");

    initOuterFactory();

    initDBOperator();

    // 注册动态加载命令
    TARS_ADD_ADMIN_CMD_NORMAL("reload", ActivityServer::reloadSvrConfig);
}
/////////////////////////////////////////////////////////////////
void
ActivityServer::destroyApp()
{
    //destroy application here:
    //...
}

/*
* 配置变更，重新加载配置
*/
bool ActivityServer::reloadSvrConfig(const string &command, const string &params, string &result)
{
    try
    {
        //加载配置
        getOuterFactoryPtr()->load();
        result = "reload server config success.";
        LOG_DEBUG << "reloadSvrConfig: " << result << endl;
        return true;
    }
    catch (TC_Exception const &e)
    {
        result = string("catch tc exception: ") + e.what();
    }
    catch (std::exception const &e)
    {
        result = string("catch std exception: ") + e.what();
    }
    catch (...)
    {
        result = "catch unknown exception.";
    }

    result += "\n fail, please check it.";

    LOG_DEBUG << "reloadSvrConfig: " << result << endl;

    return true;
}

/**
 * 初始化DB操作对象
*/
void ActivityServer::initDBOperator()
{
    const DBConf &dbConf = getOuterFactoryPtr()->getDBConfig();
    int iRet = DBOperatorSingleton::getInstance()->init(dbConf.Host, dbConf.user, dbConf.password, dbConf.dbname, dbConf.charset, dbConf.port);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "Init DBOperator failed, exit server." << endl;
        //terminate();
        return;
    }
}

int ActivityServer::initOuterFactory()
{
    _pOuter = new OuterFactoryImp();
    return 0;
}

/////////////////////////////////////////////////////////////////
int
main(int argc, char *argv[])
{
    try
    {
        g_app.main(argc, argv);
        g_app.waitForShutdown();
    }
    catch (std::exception &e)
    {
        cerr << "std::exception:" << e.what() << std::endl;
    }
    catch (...)
    {
        cerr << "unknown exception." << std::endl;
    }
    return -1;
}
/////////////////////////////////////////////////////////////////
