#ifndef _OUTER_FACTORY_IMP_H_
#define _OUTER_FACTORY_IMP_H_

#include <string>
#include <map>
#include <vector>

//
#include "servant/Application.h"
#include "globe.h"
#include "OuterFactory.h"
#include "LotteryDefine.h"
#include "LotteryProto.h"
#include "LogComm.h"

//wbl
#include <wbl/regex_util.h>

//配置服务
#include "ConfigServant.h"
#include "DBAgentServant.h"
#include "HallServant.h"
#include "Log2DBServant.h"
#include "PushServant.h"

///
using namespace config;
using namespace dataproxy;
using namespace dbagent;
using namespace hall;

//时区
#define ONE_DAY_TIME (24*60*60)
#define ZONE_TIME_OFFSET (8*60*60)

//
class OuterFactoryImp;
typedef TC_AutoPtr<OuterFactoryImp> OuterFactoryImpPtr;

typedef struct LeagueConf
{
    int league_max_level; //最大等级
    std::vector<int> vec_league_group_num;//升级：保级：降级 人数分布
    long league_start_time;//初始开始时间
    int league_interval_time; //赛季间隔时间
    std::map<int, std::vector<int>> map_reward_conf;//奖励配置
    std::vector<int> vec_extra_reward_ratio;//抽水奖励比例配置
    std::map<int, std::vector<int>> map_score_conf;// 机器人加分区间
    int newhand_interval_time;//新手添加间隔时间
    int change_score_interval_time; //改变机器人分数间隔时间
    LeagueConf(): league_max_level(0), league_start_time(0), league_interval_time(0), newhand_interval_time(0), change_score_interval_time(0)
    {
       vec_league_group_num.clear();
       map_reward_conf.clear();
       vec_extra_reward_ratio.clear();
       map_score_conf.clear();
    }
} sLeagueConf; 

/**
 * 外部工具接口对象工厂
 */
class OuterFactoryImp : public OuterFactory
{
private:
    /**
     *
    */
    OuterFactoryImp();

    /***
     *
    */
    ~OuterFactoryImp();

    ///
    friend class ActivityServantImp;
    //
    friend class ActivityServer;

public:
    //框架中用到的outer接口(不能修改):
    const OuterProxyFactoryPtr &getProxyFactory() const
    {
        return _pProxyFactory;
    }

    tars::TC_Config &getConfig() const
    {
        return *_pFileConf;
    }

    const config::DBConf &getDBConfig()
    {
        return dbConf;
    }

public:
    //读取所有配置
    void load();
    //代理配置
    void readPrxConfig();
    //
    void readDBConfig();
    //
    string getIp(const string &domain);
    //打印代理配置
    void printPrxConfig();
    //roomsvr, from remote server
    void readRoomServerListFromRemote();
    //
    void printRoomServerListFromRemote();
    //
    int getRoomServerPrxByRemote(const string &id, string &prx);
    //
    const map<string, string> getRoomServerFromRemote();
    //
    const std::vector<Award> &getAwardByDropId(int dropId);
    //
    int getSuperNeedFreeTimes();
    //
    int getMaxLoopTimes();
    //
    bool needReset_accumulated_rotate_times(int times);
    //
    bool achieveLoopRotation(int times);
    //
    int getAccumulate(int rotateId, int order);
    //返回掉落组
    int getDropId(int rotateId, int order);
    //获取首次操作
    lottery::E_LOTTERY_TYPE getFirstOpt();
    //获取旋转组的ID
    int getRotateId(lottery::E_LOTTERY_TYPE type, int frequency);
    //获取全部的旋转组的ID
    std::vector<int> getAllRotateId();
    // 判断是否满足首次旋转的条件
    bool achieveHistoryRotateCondition(int rotateTimes);
    //TODO order记得要归零,否则容易越界crash
    const RotationInfo &getRotationInfo(int rotateId, int order);
    //
    const std::map<int, std::vector<RotationInfo>> &getRotationInfo();
    //
    bool order_out_of_range(int order, int rotateId);

private:
    //
    void createAllObject();
    //
    void deleteAllObject();

public:
    //游戏配置服务代理
    const ConfigServantPrx getConfigServantPrx();
    //数据库代理服务代理
    const DBAgentServantPrx getDBAgentServantPrx(const long uid);
    //数据库代理服务代理
    const DBAgentServantPrx getDBAgentServantPrx(const string key);
    //广场服务代理
    const hall::HallServantPrx getHallServantPrx(const long uid);
    //广场服务代理
    const hall::HallServantPrx getHallServantPrx(const string key);
    //日志入库服务代理
    const DaqiGame::Log2DBServantPrx getLog2DBServantPrx(const long uid);
    //推送代理服务
    const push::PushServantPrx getPushServantPrx(const long uid);

public:
    ///金猪配置
    const config::GoldPigItem &getGoldPigConfigByLevel(int level);
    //
    int getFirstLevel();
    //
    int maxLevel();
    //获取免费旋转冷却时间(秒)
    int getRotateTime();
    //加载通用配置
    void readGeneralConfigResp();
    //
    void printGeneralConfigResp();
    //
    std::vector<string>& getScratchList()
    {
        return _vScratchList;
    }
    //
    std::vector<string>& getScratchMultList()
    {
        return _vScratchMultList;
    }
    //
    const config::ListGeneralConfigResp &getGeneralConfigResp();

    int getLeagueConfMaxLevel()
    {
        return _league_conf.league_max_level;
    }

    long getLeagueConfStartTime()
    {
        return _league_conf.league_start_time;
    }

    int getLeagueConfIntervalTime()
    {
        return _league_conf.league_interval_time;
    }

    int getLeagueConfNewHandIntervalTime()
    {
        return _league_conf.newhand_interval_time;
    }

    int getLeagueConfChangeScoreIntervalTime()
    {
        return _league_conf.change_score_interval_time;
    }

    std::vector<int>& getLeagueConfGroupNum()
    {
        return _league_conf.vec_league_group_num;
    }

    std::vector<int>& getLeagueExtraRewardRatio()
    {
        return _league_conf.vec_extra_reward_ratio;
    }

    map<int, std::vector<int>>& getLeagueRewardConf()
    {
        return _league_conf.map_reward_conf;
    }

    int getLeagueChangeScoreByLevel(int level)
    {
        auto it = _league_conf.map_score_conf.find(level);
        if(it == _league_conf.map_score_conf.end() || it->second.size() != 2)
        {
            return 0;
        }
        return rand() % it->second[1] + it->second[0];
    }

    int getLeagueConfGroupNumByIndex(int index)
    {
        if(index < 0)
        {
            int totol_num = 0;
            for(auto num : _league_conf.vec_league_group_num)
            {
                totol_num += num;
            }
            return totol_num;
        }
        return _league_conf.vec_league_group_num[index];
    }

public:
    //格式化时间
    string GetTimeFormat();
    //拆分字符串成整形
    int splitInt(string szSrc, vector<int> &vecInt);

    int splitStringFromat(string szSrc, std::vector<std::vector<int>> &vecResult);

    string joinStringFromat(std::vector<string> &vecSrc);

private:
    //读写锁
    wbl::ReadWriteLocker m_rwlock;
    //框架用到的共享对象(不能修改):
    tars::TC_Config *_pFileConf;
    //
    OuterProxyFactoryPtr _pProxyFactory;
    //游戏配置服务
    std::string _ConfigServantObj;
    ConfigServantPrx _ConfigServerPrx;
    //数据库代理服务
    std::string _DBAgentServantObj;
    DBAgentServantPrx _DBAgentServerPrx;
    //广场代理服务
    std::string _HallServantObj;
    HallServantPrx _HallServerPrx;
    //日志入库服务
    std::string _Log2DBServantObj;
    DaqiGame::Log2DBServantPrx _Log2DBServerPrx;
    //推送代理服务
    std::string _PushServantObj;
    push::PushServantPrx _PushServerPrx;


private:
    config::DBConf dbConf; //数据源配置

private:
    //房间配置
    map<string, string> mapRoomServerFromRemote;
    //金猪配置
    std::vector<config::GoldPigItem> _goldPigConfig;

private:
    // dropId --> Award
    std::map<int, std::vector<Award>> _LotteryAwards;
    std::map<int, std::vector<RotationInfo>> _RotationInfoList; //key:旋转组ID
    std::map<lottery::E_LOTTERY_TYPE, std::vector<Synopsis>> _mapSynopsis;
    std::vector<string> _vScratchList;
    std::vector<string> _vScratchMultList;
    config::ListGeneralConfigResp listGeneralConfigResp;//通用配置

    //联赛配置
    sLeagueConf _league_conf; //联赛配置
};

////////////////////////////////////////////////////////////////////////////////
#endif


