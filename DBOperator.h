#ifndef _DB_OPERATOR_H_
#define _DB_OPERATOR_H_

#include <string>
#include <map>
#include <vector>

#include <util/tc_config.h>
#include <util/tc_mysql.h>
#include <util/tc_singleton.h>
#include <util/tc_autoptr.h>

///
#include <wbl/pthread_util.h>

#include "scratch.pb.h"
#include "ActivityServer.h"
//
using namespace std;
using namespace tars;
using namespace wbl;


typedef struct LeagueUserInfo
{
    long uid;
    int rank;
    int league_level; //等级
    int last_league_level;//上赛季等级
    int partition_id;//分区
    long win_num;   //赢取的筹码
    long pump_num; //抽水
    LeagueUserInfo(): uid(0), rank(0), league_level(0), last_league_level(0), partition_id(0), win_num(0), pump_num(0)
    {
    }
    void reset(int in_rank, int in_league_level, int in_last_league_level, int in_partition_id, long in_win_num = 0, long in_pump_num = 0)
    {
        rank = in_rank;
        league_level = in_league_level;
        last_league_level = in_last_league_level;
        partition_id = in_partition_id;
        win_num = in_win_num;
        pump_num = in_pump_num;
    }
} sLeagueUserInfo; 

/**
*
* DB操作类
*/
class CDBOperator : public TC_HandleBase
{
public:
    /**
     *
    */
    CDBOperator();

    /**
     *
    */
    ~CDBOperator();

public:
    /**
    * 初始化
    */
    int init();
    int init(const TC_DBConf &dbConf);
    int init(const string &dbhost, const string &dbuser, const string &dbpass,
             const string &dbname, const string &charset, const string &dbport);

public:
    //加载配置数据
    int loadConfig();

    int ExcuteSql(long uid, int opType, const string& table_name, const string& sql, const std::vector<string>& col_name, dbagent::TExcuteSqlRsp &rsp);

    int insert_newhand(long lUid, int league_id, long win_num = 0, long pump_num = 0);

    int update_user_league_info(long lUin, long lWinNum, long lPumpNum);

    int select_achievement_board(std::vector<std::vector<long>>& vResult, int iBetId);

    int select_league_board(int lUid, ScratchProto::QueryLeagueBoardResp& resp);

    int select_league_reward_info(long lUid, ScratchProto::LeagueRewardInfoResp& resp);

    int reset_new_league_info();

    int reset_partition_info(map<string, std::vector<sLeagueUserInfo>>& map_league_user_info_group, int league_id);

    int update_newhand_league_info(long lUid);

    int check_add_newhand();

    int change_robot_league_score();

    long cal_league_reward_num(int iRank, int iLeagueLevel);

    int select_league_level(long lUid);

private:
    wbl::ReadWriteLocker m_rwlock; //读写锁，防止数据脏读

    wbl::ReadWriteLocker m_sub_rwlock; //读写锁，防止数据脏读

private:
    TC_Mysql m_mysqlObj; //mysql操作对象
    std::vector<long> vecRobotList;
    long startResetTime;
    long startAddNewHandTime;
    long startChangeScoreTime;
};

//singleton
typedef TC_Singleton<CDBOperator, CreateStatic, DefaultLifetime> DBOperatorSingleton;

//ptr
typedef TC_AutoPtr<CDBOperator> CDBOperatorPtr;

#endif


