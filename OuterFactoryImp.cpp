#include <sstream>
#include "OuterFactoryImp.h"
#include "LogComm.h"
#include "ActivityServer.h"
#include "util/tc_hash_fun.h"

//
using namespace wbl;

static vector<std::string> split(const string &str, const string &pattern)
{
    return TC_Common::sepstr<string>(str, pattern);
}

OuterFactoryImp::OuterFactoryImp(): _pFileConf(NULL)
{
    createAllObject();
}

OuterFactoryImp::~OuterFactoryImp()
{
    deleteAllObject();
}

void OuterFactoryImp::deleteAllObject()
{
    if (_pFileConf)
    {
        delete _pFileConf;
        _pFileConf = NULL;
    }
}

void OuterFactoryImp::createAllObject()
{
    try
    {
        //
        deleteAllObject();

        //本地配置文件
        _pFileConf = new tars::TC_Config();
        if (NULL == _pFileConf)
        {
            ROLLLOG_ERROR << "create config parser fail, ptr null." << endl;
            terminate();
        }

        //tars代理Factory,访问其他tars接口时使用
        _pProxyFactory = new OuterProxyFactory();
        if ((long int)NULL == _pProxyFactory)
        {
            ROLLLOG_ERROR << "create outer proxy factory fail, ptr null." << endl;
            terminate();
        }

        LOG_DEBUG << "init proxy factory succ." << endl;

        //读取所有配置
        load();
    }
    catch (TC_Exception &ex)
    {
        LOG->error() << ex.what() << endl;
    }
    catch (exception &e)
    {
        LOG->error() << e.what() << endl;
    }
    catch (...)
    {
        LOG->error() << "unknown exception." << endl;
    }

    return;
}

static void printGoldPigConfig(const std::vector<config::GoldPigItem> &vec)
{
    FUNC_ENTRY("");
    int iRet = 0;

    ostringstream os;
    os << "gold pig config: [";

    size_t len = vec.size();
    for (size_t i = 0; i < len; ++i)
    {
        auto &elem = vec[i];
        os << "{";
        os << "\"" << "level" << "\":" << elem.level << ",";
        os << "\"" << "gold" << "\":" << elem.gold << ",";
        os << "\"" << "siphon" << "\":" << elem.siphon << ",";
        os << "\"" << "storage" << "\":" << elem.storage << ",";
        os << "\"" << "purchaseAmount" << "\":" << elem.purchaseAmount;
        os << "}";
        if (i != len - 1)
        {
            os << ",";
        }
    }

    os << "]";
    FDLOG_CONFIG_INFO << os.str() << endl;

    FUNC_EXIT("", iRet);
    return;
}

//读取所有配置
void OuterFactoryImp::load()
{
    __TRY__

    //拉取远程配置
    g_app.addConfig(ServerConfig::ServerName + ".conf");

    wbl::WriteLocker lock(m_rwlock);

    _pFileConf->parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
    LOG_DEBUG << "init config file succ:" << ServerConfig::BasePath + ServerConfig::ServerName + ".conf" << endl;

    //代理配置
    readPrxConfig();
    printPrxConfig();

    readDBConfig();

    ////
    config::ListGoldPigRsp resp;
    int iRet = getConfigServantPrx()->ListGoldPigConfig(resp);
    if (iRet != 0 )
    {
        ROLLLOG_ERROR << "get Gold config error ! iRet : " << iRet << endl;
        return;
    }

    //保存
    _goldPigConfig.assign(resp.data.begin(), resp.data.end());
    printGoldPigConfig(_goldPigConfig);

    //roomsvr, from remote server
    readRoomServerListFromRemote();
    printRoomServerListFromRemote();

    //加载通用配置
    readGeneralConfigResp();
    printGeneralConfigResp();

    __CATCH__
}

//
const config::ListGeneralConfigResp &OuterFactoryImp::getGeneralConfigResp()
{
    return listGeneralConfigResp;
}

//代理配置
void OuterFactoryImp::readPrxConfig()
{
    //配置服务
    _ConfigServantObj = (*_pFileConf).get("/Main/Interface/ConfigServer<ProxyObj>", "");
    _DBAgentServantObj = (*_pFileConf).get("/Main/Interface/DBAgentServer<ProxyObj>", "");
    _HallServantObj         =   (*_pFileConf).get("/Main/Interface/HallServer<ProxyObj>", "");
    _Log2DBServantObj = (*_pFileConf).get("/Main/Interface/Log2DBServer<ProxyObj>", "");

    ROLLLOG_DEBUG << "------------------" << endl;

    {
        //累计控制表
        _mapSynopsis.clear();
        vector<string> vecDomainKey = (*_pFileConf).getDomainVector("/Main/leijikongzhi");
        for (auto &domain : vecDomainKey)
        {
            string subDomain = "/Main/leijikongzhi/" + domain;
            Synopsis item;
            item.type       = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<type>"));
            item.frequency  = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<frequency>"), "-1");
            item.rotateId   = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<rotateId>"));

            auto it = _mapSynopsis.find((lottery::E_LOTTERY_TYPE)(item.type));
            if (it == _mapSynopsis.end())
            {
                vector<Synopsis> vec;
                vec.push_back(item);
                _mapSynopsis[(lottery::E_LOTTERY_TYPE)item.type] = vec;
            }
            else
            {
                it->second.push_back(item);
            }
        }
    }

    for (auto item : _mapSynopsis)
    {
        for (auto it : item.second)
        {
            ROLLLOG_DEBUG << "type:" << it.type << ", frequency:" << it.frequency << ", rotateId:" << it.rotateId << endl;
        }
    }

    //免费旋转表--里面是奖励信息
    _LotteryAwards.clear();
    vector<string> vecDomainKey = (*_pFileConf).getDomainVector("/Main/mianfeixuanzhuan");
    for (auto &domain : vecDomainKey)
    {
        string subDomain = "/Main/mianfeixuanzhuan/" + domain;
        Award item;
        item.dropId      = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<dropId>"));
        item.Id          = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<Id>"));
        item.Type        = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<Type>"));
        item.PropId      = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<PropId>"));
        item.Number      = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<Number>"));
        item.probability = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<probability>"));

        auto it = _LotteryAwards.find(item.dropId);
        if (it == _LotteryAwards.end())
        {
            vector<Award> vec;
            vec.push_back(item);
            _LotteryAwards.insert(std::make_pair(item.dropId, vec));
        }
        else
        {
            it->second.push_back(item);
        }
    }

    //for test
    for (auto item : _LotteryAwards)
    {
        for (auto it : item.second)
        {
            ROLLLOG_DEBUG << "dropId:" << it.dropId << ", Number:" << it.Number << ", probability:" << it.probability << endl;
        }
    }

    //旋转编辑表
    _RotationInfoList.clear();
    vecDomainKey = (*_pFileConf).getDomainVector("/Main/xuanzhuanbianji");
    for (auto &domain : vecDomainKey)
    {
        string subDomain = "/Main/xuanzhuanbianji/" + domain;
        RotationInfo item;
        item.rotateId = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<rotateId>"));
        //item.order
        item.icon = (*_pFileConf).get(subDomain + "<icon>");
        item.cartoon = (*_pFileConf).get(subDomain + "<cartoon>");
        item.name = (*_pFileConf).get(subDomain + "<name>");
        item.time = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<time>"));
        item.accumulate = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<accumulate>"));
        item.dropId = TC_Common::strto<int>((*_pFileConf).get(subDomain + "<dropId>"));
        auto it = _RotationInfoList.find(item.rotateId);
        if (it == _RotationInfoList.end()) //插入数组
        {
            std::vector<RotationInfo> vecInfo;
            vecInfo.push_back(item);
            _RotationInfoList.insert(make_pair(item.rotateId, vecInfo));
        }
        else
        {
            it->second.push_back(item);
        }
    }
    _vScratchList = split((*_pFileConf).get("/Main/scratch<propsId>"), "|");
    _vScratchMultList = split((*_pFileConf).get("/Main/scratch<mult>"), "|");
    vector<string> rotateTimesVec = split((*_pFileConf).get("/Main<historyRotateTimes>"), "|");

    _league_conf.league_max_level = TC_Common::strto<int>((*_pFileConf).get("/Main/league<level>"));
    _league_conf.league_start_time = TC_Common::strto<int>((*_pFileConf).get("/Main/league<start_time>"));
    _league_conf.league_interval_time = TC_Common::strto<int>((*_pFileConf).get("/Main/league<interval_time>"));
    _league_conf.newhand_interval_time = TC_Common::strto<int>((*_pFileConf).get("/Main/league<newhand_interval_time>"));
     _league_conf.change_score_interval_time = TC_Common::strto<int>((*_pFileConf).get("/Main/league<change_score_interval_time>"));

    for (auto num : split((*_pFileConf).get("/Main/league<group>"), "|"))
    {
        _league_conf.vec_league_group_num.push_back(TC_Common::strto<int>(num));
    }
    for (auto ratio : split((*_pFileConf).get("/Main/league<extra_reward_ratio>"), "|"))
    {
        _league_conf.vec_extra_reward_ratio.push_back(TC_Common::strto<int>(ratio));
    }
    vecDomainKey = (*_pFileConf).getDomainVector("/Main/league");
    for (auto &domain : vecDomainKey)
    {
        string subDomain = "/Main/league/" + domain;
        auto vec_reward = split((*_pFileConf).get(subDomain + "<reward>"), ":");
        auto vec_score = split((*_pFileConf).get(subDomain + "<score>"), ":");
        
        std::vector<int> vt;
        for(auto item : vec_reward)
        {
            vt.push_back(TC_Common::strto<int>(item));
        }
        _league_conf.map_reward_conf.insert(std::make_pair(TC_Common::strto<int>(domain), vt));

        std::vector<int> vtt;
        for(auto item : vec_score)
        {
            vtt.push_back(TC_Common::strto<int>(item));
        }
        _league_conf.map_score_conf.insert(std::make_pair(TC_Common::strto<int>(domain), vtt));
    }
}

//打印代理配置
void OuterFactoryImp::printPrxConfig()
{
    FDLOG_CONFIG_INFO << "_ConfigServantObj ProxyObj : " << _ConfigServantObj << endl;
    FDLOG_CONFIG_INFO << "_DBAgentServantObj ProxyObj : " << _DBAgentServantObj << endl;
    FDLOG_CONFIG_INFO << "_HallServantObj ProxyObj : " << _HallServantObj << endl;
    FDLOG_CONFIG_INFO << "_Log2DBServantObj ProxyObj : " << _Log2DBServantObj << endl;
    FDLOG_CONFIG_INFO << "league_conf_info.  league_max_level: "<< _league_conf.league_max_level << ", league_start_time:"  
    << _league_conf.league_start_time << ", league_interval_time:" << _league_conf.league_interval_time 
    <<", vec_league_group_num size: "<< _league_conf.vec_league_group_num.size() << endl;
}

//域名解析
string OuterFactoryImp::getIp(const string &domain)
{
    if(domain.length() == 0)
    {
        return "";
    }

    struct hostent host = *gethostbyname(domain.c_str());
    for (int i = 0; host.h_addr_list[i]; i++)
    {
        string ip = inet_ntoa(*(struct in_addr *)host.h_addr_list[i]);
        return ip;
    }

    return "";
}


// 读取db配置
void OuterFactoryImp::readDBConfig()
{
    dbConf.Domain = (*_pFileConf).get("/Main/db<domain>", "");
    dbConf.Host = (*_pFileConf).get("/Main/db<host>", "");
    dbConf.port = (*_pFileConf).get("/Main/db<port>", "3306");
    dbConf.user = (*_pFileConf).get("/Main/db<user>", "tars");
    dbConf.password = (*_pFileConf).get("/Main/db<password>", "tars2015");
    dbConf.charset = (*_pFileConf).get("/Main/db<charset>", "utf8");
    dbConf.dbname = (*_pFileConf).get("/Main/db<dbname>", "");

    //域名
    if (dbConf.Domain.length() > 0)
    {
        string szHost = getIp(dbConf.Domain);
        if (szHost.length() > 0)
        {
            dbConf.Host = szHost;
            ROLLLOG_DEBUG << "get host by domain, Domain: " << dbConf.Domain << ", szHost: " << szHost << endl;
        }
    }
}

//游戏配置服务代理
const ConfigServantPrx OuterFactoryImp::getConfigServantPrx()
{
    if (!_ConfigServerPrx)
    {
        _ConfigServerPrx = Application::getCommunicator()->stringToProxy<ConfigServantPrx>(_ConfigServantObj);
        ROLLLOG_DEBUG << "Init _ConfigServantObj succ, _ConfigServantObj:" << _ConfigServantObj << endl;
    }

    return _ConfigServerPrx;
}

//数据库代理服务代理
const DBAgentServantPrx OuterFactoryImp::getDBAgentServantPrx(const long uid)
{
    if (!_DBAgentServerPrx)
    {
        _DBAgentServerPrx = Application::getCommunicator()->stringToProxy<dbagent::DBAgentServantPrx>(_DBAgentServantObj);
        ROLLLOG_DEBUG << "Init _DBAgentServantObj succ, _DBAgentServantObj:" << _DBAgentServantObj << endl;
    }

    if (_DBAgentServerPrx)
    {
        return _DBAgentServerPrx->tars_hash(uid);
    }

    return NULL;
}

//数据库代理服务代理
const DBAgentServantPrx OuterFactoryImp::getDBAgentServantPrx(const string key)
{
    if (!_DBAgentServerPrx)
    {
        _DBAgentServerPrx = Application::getCommunicator()->stringToProxy<dbagent::DBAgentServantPrx>(_DBAgentServantObj);
        ROLLLOG_DEBUG << "Init _DBAgentServantObj succ, _DBAgentServantObj:" << _DBAgentServantObj << endl;
    }

    if (_DBAgentServerPrx)
    {
        return _DBAgentServerPrx->tars_hash(tars::hash<string>()(key));
    }

    return NULL;
}


//广场服务代理
const hall::HallServantPrx OuterFactoryImp::getHallServantPrx(const long uid)
{
    if (!_HallServerPrx)
    {
        _HallServerPrx = Application::getCommunicator()->stringToProxy<hall::HallServantPrx>(_HallServantObj);
        LOG_DEBUG << "Init _HallServantObj succ, _HallServantObj: " << _HallServantObj << endl;
    }

    if (_HallServerPrx)
    {
        return _HallServerPrx->tars_hash(uid);
    }

    return NULL;
}

//广场服务代理
const hall::HallServantPrx OuterFactoryImp::getHallServantPrx(const string key)
{
    if (!_HallServerPrx)
    {
        _HallServerPrx = Application::getCommunicator()->stringToProxy<hall::HallServantPrx>(_HallServantObj);
        LOG_DEBUG << "Init _HallServantObj succ, _HallServantObj: " << _HallServantObj << endl;
    }

    if (_HallServerPrx)
    {
        return _HallServerPrx->tars_hash(tars::hash<string>()(key));
    }

    return NULL;
}

//日志服务代理
const DaqiGame::Log2DBServantPrx OuterFactoryImp::getLog2DBServantPrx(const long uid)
{
    if (!_Log2DBServerPrx)
    {
        _Log2DBServerPrx = Application::getCommunicator()->stringToProxy<DaqiGame::Log2DBServantPrx>(_Log2DBServantObj);
        ROLLLOG_DEBUG << "Init _Log2DBServantObj succ, _Log2DBServantObj : " << _Log2DBServantObj << endl;
    }

    if (_Log2DBServerPrx)
    {
        return _Log2DBServerPrx->tars_hash(uid);
    }

    return NULL;
}

//推送代理服务
const push::PushServantPrx OuterFactoryImp::getPushServantPrx(const long uid)
{
    if (!_PushServerPrx)
    {
        _PushServerPrx = Application::getCommunicator()->stringToProxy<push::PushServantPrx>(_PushServantObj);
        ROLLLOG_DEBUG << "Init _PushServantObj succ, _PushServantObj : " << _PushServerPrx << endl;
    }

    if (_PushServerPrx)
    {
        return _PushServerPrx->tars_hash(uid);
    }

    return NULL;
}

//roomsvr, from remote server
void OuterFactoryImp::readRoomServerListFromRemote()
{
    int iRet = 0;

    const config::ConfigServantPrx prx = getConfigServantPrx();
    iRet = prx->listAllRoomAddress(mapRoomServerFromRemote);
    //路由信息，关键因素，拉不到程序直接退出
    if (iRet != 0)
    {
        //退出
        //terminate();
    }
}

void OuterFactoryImp::printRoomServerListFromRemote()
{
    ostringstream os;
    os << "_sRoomServerObj from remote, ProxyObj: ";
    for (auto it = mapRoomServerFromRemote.begin(); it != mapRoomServerFromRemote.end(); it++)
    {
        os << ", key: " << it->first << ", value: " << it->second << endl;
    }
    FDLOG_CONFIG_INFO << os.str() << endl;
}

int OuterFactoryImp::getRoomServerPrxByRemote(const string &id, string &prx)
{
    wbl::ReadLocker lock(m_rwlock);

    //empty err
    if (id.length() == 0)
    {
        return -1;
    }

    FDLOG_CONFIG_INFO << "roomid:" << id << endl;

    auto it = mapRoomServerFromRemote.find(id);
    if (it != mapRoomServerFromRemote.end())
    {
        prx = it->second;
        return 0;
    }

    //not find
    return -2;
}

const map<string, string> OuterFactoryImp::getRoomServerFromRemote()
{
    return mapRoomServerFromRemote;
}

int OuterFactoryImp::getSuperNeedFreeTimes()
{
    try
    {
        auto elem = _mapSynopsis.at(lottery::E_LOTTERY_FREE);
        auto rotateId = elem.front().rotateId;
        std::vector<RotationInfo> &rotateGroupInfo = _RotationInfoList.at(rotateId);
        return rotateGroupInfo.size();
    }
    catch (...)
    {
        ROLLLOG_ERROR << "leijikongzhi configure error!" << endl;
        TERMINATE_APPLICATION();
    }
}

int OuterFactoryImp::getMaxLoopTimes()
{
    try
    {
        auto &elem = _mapSynopsis.at(lottery::E_LOTTERY_LOOP);
        int times = elem.at(elem.size() - 1).frequency;
        return times;
    }
    catch (const std::out_of_range &e)
    {
        ROLLLOG_ERROR << "leijikongzhi configure error!" << endl;
        TERMINATE_APPLICATION();
    }
}

bool OuterFactoryImp::needReset_accumulated_rotate_times(int times)
{
    try
    {
        auto &elem = _mapSynopsis.at(lottery::E_LOTTERY_LOOP);
        if (!elem.empty())
        {
            int maxTimes = elem.back().frequency;
            return times >= maxTimes;
        }

        return false;
    }
    catch (std::out_of_range &e)
    {
        ROLLLOG_ERROR << e.what() << endl;
        return false;
    }
}

bool OuterFactoryImp::achieveLoopRotation(int times)
{
    try
    {
        auto &elem = _mapSynopsis.at(lottery::E_LOTTERY_LOOP);
        auto it = std::find_if(elem.begin(), elem.end(), [times](const Synopsis & param)->bool
        {
            return param.frequency == times;
        });
        return it != elem.end();
    }
    catch (std::out_of_range &e)
    {
        ROLLLOG_ERROR << e.what() << endl;
        return false;
    }
}

int OuterFactoryImp::getAccumulate(int rotateId, int order)
{
    try
    {
        std::vector<RotationInfo> &infoVec = _RotationInfoList.at(rotateId);
        RotationInfo &infoItem = infoVec.at(order);
        int accumulate = infoItem.accumulate;
        return accumulate;
    }
    catch (...)
    {
        ROLLLOG_ERROR << "out_of_range! rotateId : " << rotateId << ", order : " << order << endl;
        TERMINATE_APPLICATION();
    }
}

//返回掉落组
int OuterFactoryImp::getDropId(int rotateId, int order)
{
    try
    {
        auto &item = _RotationInfoList.at(rotateId);
        RotationInfo &info = item.at(order);
        int dropId = info.dropId;
        return dropId;
    }
    catch (std::out_of_range &e)
    {
        ROLLLOG_ERROR << e.what() << endl;
        TERMINATE_APPLICATION();
    }
}

//获取首次操作
lottery::E_LOTTERY_TYPE OuterFactoryImp::getFirstOpt()
{
    lottery::E_LOTTERY_TYPE type = lottery::E_LOTTERY_NONE;
    for (auto it = _mapSynopsis.begin(); it != _mapSynopsis.end(); ++it)
    {
        if (it->first == lottery::E_LOTTERY_FREE)
            continue;

        std::vector<Synopsis>  &vec = it->second;
        for (auto &elem : vec)
        {
            if (elem.frequency == 0)
            {
                type = (lottery::E_LOTTERY_TYPE)elem.type;
                return type;
            }
        }
    }

    return lottery::E_LOTTERY_FREE;
}

//获取旋转组的ID
int OuterFactoryImp::getRotateId(lottery::E_LOTTERY_TYPE type, int frequency)
{
    try
    {
        std::vector<Synopsis> &synopsisVec = _mapSynopsis.at(type);
        for (auto &item : synopsisVec)
        {
            if (item.frequency == frequency)
            {
                return item.rotateId;
            }
        }

        //普通旋转没有累计次数,直接返回第一项
        return synopsisVec.front().rotateId;
    }
    catch (...)
    {
        ROLLLOG_ERROR << "out_of_range! type : " << type << ", frequency : " << frequency << endl;
        TERMINATE_APPLICATION();
    }
}

//获取全部的旋转组的ID
std::vector<int> OuterFactoryImp::getAllRotateId()
{
    std::vector<int> vec;
    for (auto &item : _RotationInfoList)
    {
        vec.push_back(item.first);
    }

    return vec;
}

// 判断是否满足首次旋转的条件
bool OuterFactoryImp::achieveHistoryRotateCondition(int rotateTimes)
{
    try
    {
        lottery::E_LOTTERY_TYPE type = lottery::E_LOTTERY_FIRST_TIME;
        auto &vec = _mapSynopsis.at(type);
        auto it = std::find_if(vec.begin(), vec.end(), [rotateTimes](const Synopsis & param)->bool
        {
            return param.frequency == rotateTimes;
        });
        return it != vec.end();
    }
    catch (const std::out_of_range &e)
    {
        ROLLLOG_ERROR << "configure error! can not find type E_LOTTERY_FIRST_TIME entity!" << endl;
        TERMINATE_APPLICATION();
    }
}

//TODO order记得要归零,否则容易越界crash
const RotationInfo &OuterFactoryImp::getRotationInfo(int rotateId, int order)
{
    try
    {
        std::vector<RotationInfo> &rotationInfoVec = _RotationInfoList.at(rotateId);
        const RotationInfo &result = rotationInfoVec.at(order);
        return result;
    }
    catch (std::out_of_range &e)
    {
        ROLLLOG_ERROR << e.what() << endl;
        TERMINATE_APPLICATION();
    }
}

const std::map<int, std::vector<RotationInfo>> &OuterFactoryImp::getRotationInfo()
{
    return _RotationInfoList;
}

bool OuterFactoryImp::order_out_of_range(int order, int rotateId)
{
    try
    {
        const std::vector<RotationInfo> &vec = _RotationInfoList.at(rotateId);
        return order >= (int)vec.size();
    }
    catch (const std::out_of_range &e)
    {
        ostringstream os;
        os << "param : { \"order\":" << order << ",\"rotateId\":" << rotateId << "}, errorDetail : " << e.what() << endl;
        string errorMsg = os.str();
        ROLLLOG_ERROR <<  errorMsg << endl;
        TERMINATE_APPLICATION();
    }
}

const config::GoldPigItem &OuterFactoryImp::getGoldPigConfigByLevel(int level)
{
    try
    {
        return _goldPigConfig.at(level - 1);
    }
    catch (const std::out_of_range &e)
    {
        ostringstream os;
        os << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << " level : " << level << endl;
        string msg = os.str();
        throw std::out_of_range(msg);
    }
}

int OuterFactoryImp::getFirstLevel()
{
    try
    {
        return _goldPigConfig.at(0).level;
    }
    catch (const std::out_of_range &e)
    {
        ostringstream os;
        os << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__  << endl;
        string msg = os.str();
        throw std::out_of_range(msg);
    }
}

int OuterFactoryImp::maxLevel()
{
    try
    {
        size_t len = _goldPigConfig.size();
        return _goldPigConfig.at(len - 1).level;
    }
    catch (const std::out_of_range &e)
    {
        ostringstream os;
        os << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << endl;
        string msg = os.str();
        throw std::out_of_range(msg);
    }
}

//免费旋转冷却时间(秒)
int OuterFactoryImp::getRotateTime()
{
    //通用配置-免费旋转冷却时间(秒)
    int type = config::E_GENERAL_TYPE_ROTATE_TIME;
    auto iter = listGeneralConfigResp.data.find(type);
    if ((iter == listGeneralConfigResp.data.end()) || ((int)iter->second.size() != 1))
    {
        ROLLLOG_ERROR << "getRotateTime failed, type: " << type << ", size: " << iter->second.size() << endl;
        return 14400;
    }
    auto itCfg = iter->second.begin();
    if (itCfg->second.value < 0)
    {
        ROLLLOG_ERROR << "listGeneralConfigResp value error, type: " << type << ", value: " << itCfg->second.value << endl;
        return 14400;
    }

    ROLLLOG_DEBUG << "listGeneralConfigResp RotateTime:" << itCfg->second.value << ", type:" << type << endl;
    return itCfg->second.value;
}

//加载通用配置
void OuterFactoryImp::readGeneralConfigResp()
{
    getConfigServantPrx()->ListGeneralConfig(listGeneralConfigResp);
}

void OuterFactoryImp::printGeneralConfigResp()
{
    FDLOG_CONFIG_INFO << "listGeneralConfigResp: " << printTars(listGeneralConfigResp) << endl;
}


//格式化时间
string OuterFactoryImp::GetTimeFormat()
{
    string sFormat("%Y-%m-%d %H:%M:%S");
    time_t t = time(NULL);
    auto pTm = localtime(&t);
    if (pTm == NULL)
    {
        return "";
    }

    char sTimeString[255] = "\0";
    strftime(sTimeString, sizeof(sTimeString), sFormat.c_str(), pTm);
    return string(sTimeString);
}

//拆分字符串成整形
int OuterFactoryImp::splitInt(string szSrc, vector<int> &vecInt)
{
    split_int(szSrc, "[ \t]*\\|[ \t]*", vecInt);
    return 0;
}

int OuterFactoryImp::splitStringFromat(string szSrc, std::vector<std::vector<int>> &vecResult)
{
    for(auto item : split(szSrc, "|"))
    {
        vector<int> vecInt;
        for(auto sub : split(item, ":"))
        {
            vecInt.push_back(S2I(sub));
        }
        vecResult.push_back(vecInt);
    }
    return 0;
}

string OuterFactoryImp::joinStringFromat(std::vector<string> &vecSrc)
{
    string result;
    std::random_shuffle(vecSrc.begin(), vecSrc.end());
    for(unsigned int i = 0; i < vecSrc.size(); i++)
    {
        result += (vecSrc[i] + (i == vecSrc.size() -1 ? "" : "|"));
    }
    return result;
}

//TODO
const std::vector<Award> &OuterFactoryImp::getAwardByDropId(int dropId)
{
    int iRet = 0;
    FUNC_ENTRY("");

    static vector<Award> award;// 该变量永远是空
    auto it = _LotteryAwards.find(dropId);
    if (it == _LotteryAwards.end())
    {
        return award;
    }

    FUNC_EXIT("", iRet);
    return it->second;
}

////////////////////////////////////////////////////////////////////////////////


