#include <sstream>
#include <util/tc_common.h>
#include "DBOperator.h"
#include "globe.h"
#include "LogComm.h"

//
using namespace wbl;

/**
 *
*/
CDBOperator::CDBOperator()
{

}

/**
 *
*/
CDBOperator::~CDBOperator()
{

}

//初始化
int CDBOperator::init()
{
    FUNC_ENTRY("");

    int iRet = 0;

    try
    {
        //for test
        map<string, string> mpParam;
        mpParam["dbhost"]  = "localhost";
        mpParam["dbuser"]  = "tars";
        mpParam["dbpass"]  = "tars2015";
        mpParam["dbname"]  = "config";
        mpParam["charset"] = "utf8";
        mpParam["dbport"]  = "3306";

        TC_DBConf dbConf;
        dbConf.loadFromMap(mpParam);

        //初始化数据库连接
        m_mysqlObj.init(dbConf);
    }
    catch (exception &e)
    {
        iRet = -1;
        ROLLLOG_ERROR << "Catch exception: " << e.what() << endl;
    }
    catch (...)
    {
        iRet = -2;
        ROLLLOG_ERROR << "Catch unknown exception." << endl;
    }

    FUNC_EXIT("", iRet);
    return iRet;
}

//初始化
int CDBOperator::init(const string &dbhost, const string &dbuser, const string &dbpass, const string &dbname, const string &charset, const string &dbport)
{
    FUNC_ENTRY("");

    int iRet = 0;

    try
    {
        map<string, string> mpParam;
        mpParam["dbhost"]  = dbhost;
        mpParam["dbuser"]  = dbuser;
        mpParam["dbpass"]  = dbpass;
        mpParam["dbname"]  = dbname;
        mpParam["charset"] = charset;
        mpParam["dbport"]  = dbport;

        TC_DBConf dbConf;
        dbConf.loadFromMap(mpParam);

        m_mysqlObj.init(dbConf);
    }
    catch (exception &e)
    {
        iRet = -1;
        ROLLLOG_ERROR << "Catch exception: " << e.what() << endl;
    }
    catch (...)
    {
        iRet = -2;
        ROLLLOG_ERROR << "Catch unknown exception." << endl;
    }

    FUNC_EXIT("", iRet);

    return iRet;
}

//初始化
int CDBOperator::init(const TC_DBConf &dbConf)
{
    FUNC_ENTRY("");

    int iRet = 0;

    try
    {
        //初始化数据库连接
        m_mysqlObj.init(dbConf);
    }
    catch (exception &e)
    {
        iRet = -1;
        ROLLLOG_ERROR << "Catch exception: " << e.what() << endl;
    }
    catch (...)
    {
        iRet = -2;
        ROLLLOG_ERROR << "Catch unknown exception." << endl;
    }

    FUNC_EXIT("", iRet);

    return iRet;
}

int CDBOperator::ExcuteSql(long uid, int opType, const string& table_name, const string& sql, const std::vector<string>& col_name, dbagent::TExcuteSqlRsp &rsp)
{
    dbagent::TExcuteSqlReq req;
    req.keyIndex = 0;
    req.tableName = table_name;
    req.sql = sql;
    req.opType = opType;

    vector<dbagent::TField> fields;
    dbagent::TField tfield;
    tfield.colArithType = E_NONE;
    for(auto item : col_name)
    {
        tfield.colName = item;
        fields.push_back(tfield);
    }
    req.fields = fields;

    int iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(uid)->Excute(req, rsp);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << "exec sql err. sql: "<< sql<< endl;
        return iRet;
    }
    return 0;
}

//加载奖励配置
int CDBOperator::select_achievement_board(std::vector<std::vector<long>>& vResult, int iBetId)
{
    FUNC_ENTRY("");

    int iRet = 0;

    try
    {
        std::vector<string> col_name = {"uid", "champion_num", "reward_num", "win_num"};
        string strSQL = "SELECT uid,  floor( `champion_num` / `ach_condition` ) AS champion_num,  reward_num , " \
                        "IF( champion_num  <= 0, 0, floor(  game_num / champion_num ) ) AS win_num " \
                        "FROM tb_achievement WHERE `bet_id` = '" + I2S(iBetId) + "' " \
                        "ORDER BY champion_num DESC, reward_num DESC, win_num DESC LIMIT 100";

        dbagent::TExcuteSqlRsp rsp;
        iRet =  ExcuteSql(0, 0, "tb_achievement", strSQL, col_name, rsp);

        std::vector<long> vecItem = {0, 0, 0, 0};
        for(auto record : rsp.records)
        {
            for(auto item : record)
            {
                if(item.colName == "uid")
                {
                   vecItem[0] =  TC_Common::strto<long>(item.colValue);
                }
                else if(item.colName == "champion_num")
                {
                   vecItem[1] =  TC_Common::strto<long>(item.colValue);
                }
                else if(item.colName == "reward_num")
                {
                   vecItem[2] =  TC_Common::strto<long>(item.colValue);
                }
                else if(item.colName == "win_num")
                {
                   vecItem[3] =  TC_Common::strto<long>(item.colValue);
                }
            }
            vResult.push_back(vecItem);
        }
    }
    catch (TC_Mysql_Exception &e)
    {
        ROLLLOG_DEBUG << "select operator catch mysql exception: " << e.what() << endl;
        iRet = -1;
    }
    catch (...)
    {
        ROLLLOG_DEBUG << "select operator catch unknown exception." << endl;
        iRet = -2;
    }

    FUNC_EXIT("", iRet);
    return iRet;
}

int CDBOperator::insert_newhand(long lUid, int league_id, long win_num, long pump_num)
{
    std::vector<string> col_name = {"member_count"};
    string strSQL = "select count(1) as member_count from tb_league where league_level = 1 and league_id = " + I2S(league_id) + ";";

    dbagent::TExcuteSqlRsp rsp;
    int iRet =  ExcuteSql(lUid, 0, "tb_league", strSQL, col_name, rsp);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
        return iRet;
    }

    int partition_id = 1;
    for(auto record : rsp.records)
    {
        for(auto item : record)
        {
            if(item.colName == "member_count")
            {
                int member_count =  TC_Common::strto<int>(item.colValue);
                int add_partition = member_count % g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) == 0 ? 1: 0;
                partition_id = (member_count / g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) + 1) + add_partition;
            }
        }
    }

    strSQL = "insert into tb_league(uid, league_level, league_id, partition_id, win_num, pump_num) values(" +  L2S(lUid) + ", 1, " + I2S(league_id) + \
            "," + I2S(partition_id) + ", " + L2S(win_num) + ", " + L2S(pump_num) + ");";
    iRet =  ExcuteSql(lUid, 1, "tb_league", strSQL, {}, rsp);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
        return iRet;
    }
    return 0;
}

int CDBOperator::update_user_league_info(long lUin, long lWinNum, long lPumpNum)
{
    FUNC_ENTRY("");

    int iRet = 0;

    std::vector<string> col_name = {"member_count"};
    string strSQL = "select count(1) as member_count from tb_league where uid = " + I2S(lUin) + ";";

    dbagent::TExcuteSqlRsp rsp;
    iRet =  ExcuteSql(lUin, 0, "tb_league", strSQL, col_name, rsp);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
        return iRet;
    }

    int member_count = 0;
    for(auto record : rsp.records)
    {
        for(auto item : record)
        {
            if(item.colName == "member_count")
            {
                member_count =  TC_Common::strto<int>(item.colValue);
            }
        }
    }

    if(member_count == 0)
    {
        int league_id = ((TNOW - g_app.getOuterFactoryPtr()->getLeagueConfStartTime()) / g_app.getOuterFactoryPtr()->getLeagueConfIntervalTime()) + 1;
        insert_newhand(lUin, league_id, lWinNum, lPumpNum);
    }
    else
    {
        strSQL = "update tb_league set win_num = win_num + " + I2S(lWinNum) + ", pump_num = pump_num + " + I2S(lPumpNum) + " where uid = " + I2S(lUin) + ";";
        iRet =  ExcuteSql(lUin, 1, "tb_league", strSQL, {}, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }
    }
    FUNC_EXIT("", iRet);
    return iRet;
}

long CDBOperator::cal_league_reward_num(int iRank, int iLeagueLevel)
{
    ROLLLOG_DEBUG<<"iRank: "<< iRank << ", iLeagueLevel: "<< iLeagueLevel << endl;
/*    if(iRank <= 0 || iRank > g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1))
    {
        ROLLLOG_ERROR << "iRank err. iRank :"<< iRank << ", total_group_num: "<< g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) << endl;
        return 0;
    }

    WriteLocker lock(m_sub_rwlock);
    long reward_num = 0;
    //固定奖励
    auto itreward = g_app.getOuterFactoryPtr()->getLeagueRewardConf().find(iLeagueLevel);
    if(itreward != g_app.getOuterFactoryPtr()->getLeagueRewardConf().end())
    {
        if(itreward->second.size() != 3)
        {
            ROLLLOG_ERROR<<"reward err. " << endl;
            return 0;
        }
        if(iRank <= g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(0) &&iLeagueLevel < g_app.getOuterFactoryPtr()->getLeagueConfMaxLevel())//升级区域
        {
            reward_num += itreward->second[0];
        }
        else if(iRank > g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(1) + g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(0) && iLeagueLevel > 1)//降级区域
        {
            reward_num += itreward->second[2];
        }
        else
        {
            reward_num += itreward->second[1];
        }
        
        long total_pump_num = 0;
        string strSQL = "select sum(pump_num) as pump_num from tb_league where league_level = " + I2S(iLeagueLevel) + ";";
        TC_Mysql::MysqlData res = m_mysqlObj.queryRecord(strSQL);
        for (size_t i = 0; i < res.size(); ++i)
        {
            total_pump_num += TC_Common::strto<long>(res[i]["pump_num"]);
        }

        ROLLLOG_DEBUG<<"reward_num: "<< reward_num <<", total_pump_num: "<< total_pump_num << ", size: "<< g_app.getOuterFactoryPtr()->getLeagueExtraRewardRatio().size() << endl;
        //额外抽水奖励
        if((unsigned) iRank <= g_app.getOuterFactoryPtr()->getLeagueExtraRewardRatio().size() && iRank > 0)
        {
            reward_num += (total_pump_num * 0.1) * g_app.getOuterFactoryPtr()->getLeagueExtraRewardRatio()[iRank - 1] / 100;
        } 
    }
    else
    {
        ROLLLOG_ERROR << "reward conf err. iLeagueLevel: " << iLeagueLevel <<", conf size: "<< g_app.getOuterFactoryPtr()->getLeagueRewardConf().size() << endl;
    }
    return reward_num;*/
    return 0;
}

int CDBOperator::select_league_board(int lUid, ScratchProto::QueryLeagueBoardResp& resp)
{
    FUNC_ENTRY("");

    WriteLocker lock(m_rwlock);

    int iRet = 0;

    try
    {
        int league_id = ((TNOW - g_app.getOuterFactoryPtr()->getLeagueConfStartTime()) / g_app.getOuterFactoryPtr()->getLeagueConfIntervalTime()) + 1;

        std::vector<string> col_name = {"league_id", "partition_id"};
        string strSQL = "SELECT league_id, partition_id FROM tb_league WHERE uid = '" + L2S(lUid) +  "';";

        dbagent::TExcuteSqlRsp rsp;
        iRet =  ExcuteSql(lUid, 0, "tb_league", strSQL, col_name, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }

        if(rsp.records.size() == 0)//检测是否存在
        {
            insert_newhand(lUid, league_id);
        }

        rsp.records.clear();
        col_name.clear();
        col_name = {"level"};
        strSQL = "SELECT level FROM tb_userinfo WHERE uid = '" + L2S(lUid) +  "';";
        iRet =  ExcuteSql(lUid, 0, "tb_userinfo", strSQL, col_name, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }

        for(auto record : rsp.records)
        {
            for(auto item : record)
            {
                if(item.colName == "level")
                {
                    if(TC_Common::strto<long>(item.colValue) < 3)
                    {
                        return -1;
                    }
                }
            }
        }

        rsp.records.clear();
        col_name.clear();
        col_name = {"uid", "league_level", "win_num"};
        strSQL = "SELECT tb_league.*,if (tb_league.uid in (select gambler_id from  `config`.`sys_config_robot_gambler`), 1, 0 )AS is_robot FROM tb_league, " \
                "( SELECT league_id, partition_id, league_level FROM tb_league WHERE uid = ' " + L2S(lUid) + "' ) AS t WHERE tb_league.league_id = t.league_id " \
                "AND tb_league.partition_id = t.partition_id AND tb_league.league_level = t.league_level ORDER BY tb_league.win_num DESC, is_robot DESC, id ASC;";
        iRet =  ExcuteSql(lUid, 0, "tb_league", strSQL, col_name, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }

        int remain_num = rsp.records.size() - g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1);
        int iRank = 0;
        int count = 0;
        for(auto record : rsp.records)
        {
            vector<long> vResult = {0, 0, 0};
            for(auto item : record)
            {
                if(item.colName == "uid")
                {
                   vResult[0] = TC_Common::strto<long>(item.colValue);
                }
                else if (item.colName == "league_level")
                {
                    vResult[1] = TC_Common::strto<long>(item.colValue);
                }
                else if (item.colName == "win_num")
                {
                    vResult[2] = TC_Common::strto<long>(item.colValue);
                }
            }
            if(count < remain_num && vResult[0] != lUid)
            {
                count++;
                continue;
            }
            iRank++;
            auto ptr = resp.add_league_board_info();
            ptr->set_uid(vResult[0]);
            ptr->set_irank(iRank);
            ptr->set_league_reward_num(cal_league_reward_num(iRank, vResult[1]));
            ptr->set_league_win_num(vResult[2]);
            resp.set_league_level(vResult[1]);
        }
    }
    catch (TC_Mysql_Exception &e)
    {
        ROLLLOG_DEBUG << "select operator catch mysql exception: " << e.what() << endl;
        iRet = -1;
    }
    catch (...)
    {
        ROLLLOG_DEBUG << "select operator catch unknown exception." << endl;
        iRet = -2;
    }

    FUNC_EXIT("", iRet);
    return iRet;
}

int CDBOperator::select_league_level(long lUid)
{
    FUNC_ENTRY("");

    WriteLocker lock(m_rwlock);

    int iRet = 0;
    int league_level = 0;
    try
    {
        std::vector<string> col_name = {"league_level"};
        string strSQL = "select league_level from tb_league where uid = " + L2S(lUid);

        dbagent::TExcuteSqlRsp rsp;
        iRet =  ExcuteSql(lUid, 0, "tb_league", strSQL, col_name, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }

        for(auto record : rsp.records)
        {
            for(auto item : record)
            {
                if(item.colName == "league_level")
                {
                    league_level = TC_Common::strto<long>(item.colValue);
                }
            }
        }
    }
    catch (TC_Mysql_Exception &e)
    {
        ROLLLOG_DEBUG << "select operator catch mysql exception: " << e.what() << endl;
        iRet = -1;
    }
    catch (...)
    {
        ROLLLOG_DEBUG << "select operator catch unknown exception." << endl;
        iRet = -2;
    }

    FUNC_EXIT("", iRet);
    return league_level;
}

int CDBOperator::select_league_reward_info(long lUid, ScratchProto::LeagueRewardInfoResp& resp)
{
    FUNC_ENTRY("");

    WriteLocker lock(m_rwlock);

    int iRet = 0;

    try
    {
        std::vector<string> col_name = {"reward_level", "reward_rank", "reward_num"};
        string strSQL = "select reward_level, reward_rank, reward_num from tb_league where uid = " + L2S(lUid);

        dbagent::TExcuteSqlRsp rsp;
        iRet =  ExcuteSql(lUid, 0, "tb_league", strSQL, col_name, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }

        if (rsp.records.size() <= 0)
        {
            int league_id = ((TNOW - g_app.getOuterFactoryPtr()->getLeagueConfStartTime()) / g_app.getOuterFactoryPtr()->getLeagueConfIntervalTime()) + 1;
            insert_newhand(lUid, league_id);
            iRet =  ExcuteSql(lUid, 0, "tb_league", strSQL, col_name, rsp);
            if(iRet != 0)
            {
                ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
                return iRet;
            }
        }

        for (size_t i = 0; i < rsp.records.size(); ++i)
        {
            for(auto record : rsp.records)
            {
                for(auto item : record)
                {
                    if(item.colName == "reward_level")
                    {
                        resp.set_league_level(TC_Common::strto<long>(item.colValue));
                    }
                    else if(item.colName == "reward_rank")
                    {
                        resp.set_league_rank(TC_Common::strto<long>(item.colValue));
                    }
                    else if (item.colName == "reward_num")
                    {
                        resp.set_league_reward_num(TC_Common::strto<long>(item.colValue));
                    }
                }
            }
        }

        int league_status = 2;
        if(resp.league_rank() <= g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(0) )//升级区域
        {
            league_status = 1;
        }
        else if(resp.league_rank() > g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(0) + g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(1))
        {
            league_status = 3;
        }
        resp.set_league_status(league_status);

        col_name.clear();
        strSQL = "update tb_league set reward_num = -1, status = 2 where uid = " + L2S(lUid);
        iRet =  ExcuteSql(lUid, 1, "tb_league", strSQL, col_name, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }
    }
    catch (TC_Mysql_Exception &e)
    {
        ROLLLOG_DEBUG << "select operator catch mysql exception: " << e.what() << endl;
        iRet = -1;
    }
    catch (...)
    {
        ROLLLOG_DEBUG << "select operator catch unknown exception." << endl;
        iRet = -2;
    }

    FUNC_EXIT("", iRet);
    return iRet;
}

int CDBOperator::reset_partition_info(map<string, std::vector<sLeagueUserInfo>>& map_league_user_info_group, int league_id)
{
    //step 1: sort and up down
    map<int, std::vector<sLeagueUserInfo>> map_league_user_info_group_new;
    map<int, long> map_pump_stat; //level:num
    for(auto& item : map_league_user_info_group)
    {
       /* std::sort(item.second.begin(), item.second.end(), [](sLeagueUserInfo l, sLeagueUserInfo r)->bool{
            return l.win_num > r.win_num;
        });*/

        for(unsigned int i = 0; i < item.second.size(); i++)
        {
            //过滤机器人
            
            long cur_uid = item.second[i].uid;
            auto it_robot = std::find_if(vecRobotList.begin(), vecRobotList.end(), [cur_uid](const long robot_id)->bool{
                return cur_uid == robot_id;
            });
            if(it_robot != vecRobotList.end())
            {
                continue;
            }
            
            int new_league_level = item.second[i].league_level;
            if(item.second[i].rank <= g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(0) && item.second[i].league_level < g_app.getOuterFactoryPtr()->getLeagueConfMaxLevel())//升级区域
            {
                new_league_level += 1;
            }
            else if(item.second[i].rank > g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(1) + g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(0) && item.second[i].league_level > 1)//降级区域
            {
                new_league_level -= 1;
            }
            item.second[i].reset(item.second[i].rank, new_league_level, item.second[i].league_level, 0, 0);

            auto it = map_league_user_info_group_new.find(item.second[i].league_level);
            if(it == map_league_user_info_group_new.end())
            {
                std::vector<sLeagueUserInfo> sub;
                sub.push_back(item.second[i]);
                map_league_user_info_group_new.insert(std::make_pair(item.second[i].league_level, sub));
            }
            else
            {
                it->second.push_back(item.second[i]);
            }
            ROLLLOG_DEBUG << "change league_level: "<< item.second[i].league_level << ", uid： "<< item.second[i].uid<<", rank: "<< item.second[i].rank << ", new_league_level:" << new_league_level << endl;
        }
    }
   
    //step 3: new group
    for(auto& item : map_league_user_info_group_new)
    {
        if(item.second.size() == 0)
        {
            continue;
        }
       
        ROLLLOG_DEBUG << "===> before item size : " << item.second.size() << ", level:" << item.first<< ", group size: "<< g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) << endl;

        //step 3.1: 分区不够的需要补充机器人 非新手区  随机12-24个机器人数量补位
        if(item.first != 1)
        {
            int min_partition_num = ceil(item.second.size() / 28);
            int max_partition_num = floor(item.second.size() / 16);

            srand(TNOW);
            int partition_num = min_partition_num == max_partition_num ? max_partition_num : (rand() % (max_partition_num - min_partition_num) + min_partition_num);
            partition_num  = partition_num <= 0 ? 1: partition_num;

            int add_num = partition_num * g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) - item.second.size();

            for(int i = 0; i < add_num; i++)
            {
                if(vecRobotList.empty())
                {
                    break;
                }
                sLeagueUserInfo info;
                info.uid = vecRobotList.back();
                vecRobotList.pop_back();
                int iRank = item.second.size()  + i + 1;
                info.reset(iRank, item.second[0].league_level, item.second[0].last_league_level, 0, 0);
                item.second.push_back(info);
            }

           /* if(item.second.size() % g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) != 0 && !vecRobotList.empty())
            {
                int add_num = g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) - (item.second.size() % g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1));
                ROLLLOG_DEBUG << " add_num: " << add_num << endl;
                for(int i = 0; i < add_num; i++)
                {
                    sLeagueUserInfo info;
                    info.uid = vecRobotList.back();
                    vecRobotList.pop_back();
                    int iRank = (item.second.size() % g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1)) + i + 1;
                    info.reset(iRank, item.second[0].league_level, item.second[0].last_league_level, 0, 0);
                    item.second.push_back(info);
                }
            }*/
            ROLLLOG_DEBUG << "<=== after item size : " << item.second.size() << endl;

            if(item.second.size() % g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) != 0)
            {
                ROLLLOG_ERROR<<"group err. level: "<< item.first <<", size: "<< item.second.size()<<endl;
            }
        }

        std::random_shuffle(item.second.begin(), item.second.end());
        std::random_shuffle(item.second.begin(), item.second.end());

        //step 3.2
        for(unsigned int i = 0; i < item.second.size(); i++)
        {
            int reward_rank = item.second[i].rank > g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) ? g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) : item.second[i].rank;
            long last_league_reward_num = cal_league_reward_num(reward_rank, item.second[i].last_league_level);
            try{
                int new_partition_id = i / g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) + 1;
                string strSQL = "insert into tb_league(`uid`, `league_level`, `league_id`, `partition_id`) value(" + L2S(item.second[i].uid) + "," + \
                        I2S(item.second[i].league_level) + ", " + I2S(league_id) + ", " + I2S(new_partition_id) + ")on duplicate key update " + \
                        "league_level = " + I2S(item.second[i].league_level) + ", status = 1, reward_num = " + L2S(last_league_reward_num) + \
                        ", partition_id = " + I2S(new_partition_id) + ", reward_level = " + I2S(item.second[i].last_league_level) + ", reward_rank = " + I2S(item.second[i].rank) + \
                        ", win_num = 0, pump_num =0; ";

                dbagent::TExcuteSqlRsp rsp;
                int iRet =  ExcuteSql(item.second[i].uid, 1, "tb_league", strSQL, {}, rsp);
                if(iRet != 0)
                {
                    ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
                    return iRet;
                }
            } catch (TC_Mysql_Exception &e)
            {
                ROLLLOG_DEBUG << "select operator catch mysql exception: " << e.what() << endl;
                return -1;
            }
        }
    }
    return 0;
}

int CDBOperator::check_add_newhand()
{
    //防止重复执行
    if(std::abs(TNOW - startAddNewHandTime) < 3)
    {
        return 0;
    }

    startAddNewHandTime = TNOW;

    int league_id = ((TNOW - g_app.getOuterFactoryPtr()->getLeagueConfStartTime()) / g_app.getOuterFactoryPtr()->getLeagueConfIntervalTime()) + 1;

    //空闲的机器人
    std::vector<long> vecFreeRobotList;
    string strSQL = "select gambler_id from `config`.`sys_config_robot_gambler` where gambler_id not in (select uid from tb_league);";

    dbagent::TExcuteSqlRsp rsp;
    int iRet =  ExcuteSql(0, 0, "tb_league", strSQL, {"gambler_id"}, rsp);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
        return iRet;
    }

    for(auto record : rsp.records)
    {
        for(auto item : record)
        {
            if(item.colName == "gambler_id")
            {
                vecFreeRobotList.push_back(TC_Common::strto<long>(item.colValue));
            }
        }
    }

    if(vecFreeRobotList.empty())
    {
        return 0;
    }

    std::random_shuffle(vecFreeRobotList.begin(), vecFreeRobotList.end());

    rsp.records.clear();
    strSQL = "select partition_id, count(1) as member_count from tb_league where league_level = 1 and league_id = " + I2S(league_id) + " GROUP BY partition_id;";
    iRet =  ExcuteSql(0, 0, "tb_league", strSQL, {"partition_id", "member_count"}, rsp);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
        return iRet;
    }

    for(auto record : rsp.records)
    {
        vector<int> vResult = {0, 0};
        for(auto item : record)
        {
            if(item.colName == "member_count")
            {
                vResult[0] = TC_Common::strto<long>(item.colValue);
            }
            else if (item.colName == "partition_id")
            {
                vResult[1] = TC_Common::strto<long>(item.colValue);
            }
        }
        if(vResult[0] < g_app.getOuterFactoryPtr()->getLeagueConfGroupNumByIndex(-1) && vResult[0] > 0)
        {
            long robot_id = vecFreeRobotList.back();
            vecFreeRobotList.pop_back();
            strSQL = "insert into tb_league(uid, league_level, league_id, partition_id) values(" +  L2S(robot_id) + ", 1, " + I2S(league_id) + \
            "," + I2S(vResult[1]) + ");";

            iRet =  ExcuteSql(robot_id, 1, "tb_league", strSQL, {}, rsp);
            if(iRet != 0)
            {
                ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
                return iRet;
            }
        }
    }
    return 0;
}

int CDBOperator::change_robot_league_score()
{
    //防止重复执行
    if(std::abs(TNOW - startChangeScoreTime) < 3)
    {
        return 0;
    }

    startChangeScoreTime = TNOW;
    WriteLocker lock(m_rwlock);

    string strSQL = "select uid, league_id, league_level from tb_league where uid in (select gambler_id from `config`.`sys_config_robot_gambler`) ORDER BY league_level; ";

    dbagent::TExcuteSqlRsp rsp;
    int iRet =  ExcuteSql(0, 0, "tb_league", strSQL, {"uid", "league_id"}, rsp);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
        return iRet;
    }

    srand(TNOW);
    map<long, int> map_change_robot_league_score;
    for(auto record : rsp.records)
    {
        vector<long> vResult = {0, 0};
        for(auto item : record)
        {
            if(item.colName == "uid")
            {
                vResult[0] = TC_Common::strto<long>(item.colValue);
            }
            if(item.colName == "league_level")
            {
                vResult[1] = TC_Common::strto<long>(item.colValue);
            }
        }

        long uid = vResult[0];
        int level = vResult[1];

        int change_score =  g_app.getOuterFactoryPtr()->getLeagueChangeScoreByLevel(level);
        int random_num = rand() % 100;

        //ROLLLOG_DEBUG << "change socre robot_id: " << uid << ", change: "<< change_score << ", random_num: "<< random_num << endl;
        if(random_num > 60 || change_score <= 0)
        {
            continue;
        }

        auto it = map_change_robot_league_score.find(uid);
        if(it == map_change_robot_league_score.end())
        {
            map_change_robot_league_score.insert(std::make_pair(uid, change_score));
        }
    }

    for(auto item : map_change_robot_league_score)
    {
        strSQL = "update tb_league set win_num = win_num + " + I2S(item.second) + " where uid = " + I2S(item.first) + ";";

        iRet =  ExcuteSql(0, 1, "tb_league", strSQL, {}, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }
    }

    return 0;
}

int CDBOperator::reset_new_league_info()
{
    FUNC_ENTRY("");

    //防止重复执行
    if(std::abs(TNOW - startResetTime) < 3)
    {
        return 0;
    }

    startResetTime = TNOW;
    int iRet = 0;
    int league_id = ((TNOW - g_app.getOuterFactoryPtr()->getLeagueConfStartTime()) / g_app.getOuterFactoryPtr()->getLeagueConfIntervalTime()) + 1;
    try
    {
        dbagent::TExcuteSqlRsp rsp;

        string strSQL = "update tb_league set league_id = " + I2S(league_id) + ";";
        iRet =  ExcuteSql(0, 1, "tb_league", strSQL, {}, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }

        //加载所有机器人
        vecRobotList.clear();
        strSQL = "select gambler_id from  `config`.`sys_config_robot_gambler` where room_id not like '3%';";

        iRet =  ExcuteSql(0, 0, "sys_config_robot_gambler", strSQL, {"gambler_id"}, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }

        for(auto record : rsp.records)
        {
            for(auto item : record)
            {
                if(item.colName == "gambler_id")
                {
                    vecRobotList.push_back(TC_Common::strto<long>(item.colValue));
                }
            }
        }
        std::random_shuffle(vecRobotList.begin(), vecRobotList.end());

        //检查加载 经典场机器人
        rsp.records.clear();
        strSQL = "select gambler_id from  `config`.`sys_config_robot_gambler` where gambler_id not in (select uid from tb_league ) and room_id like '3%' ;";

        iRet =  ExcuteSql(0, 0, "sys_config_robot_gambler", strSQL, {"gambler_id"}, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }
        for(auto record : rsp.records)
        {
            for(auto item : record)
            {
                if(item.colName == "gambler_id")
                {
                    insert_newhand(TC_Common::strto<long>(item.colValue), league_id);
                }
            }
        }

        rsp.records.clear();
        strSQL = "select uid, league_id, league_level, partition_id, win_num, pump_num, if (tb_league.uid in (select gambler_id from `config`.`sys_config_robot_gambler`), 1, 0 )AS is_robot " \
                "from tb_league ORDER BY league_level, partition_id, win_num DESC, is_robot DESC, id ASC; ";

        iRet =  ExcuteSql(0, 0, "tb_league", strSQL, {"uid", "league_id", "league_level", "partition_id", "win_num", "pump_num"}, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }
        
        //step 1:  group by league_level partition_id
        map<string, std::vector<sLeagueUserInfo>> map_league_user_info_group;

        for(auto record : rsp.records)
        {
            sLeagueUserInfo item;
            for(auto sub : record)
            {
                if(sub.colName == "uid")
                {
                    item.uid = TC_Common::strto<long>(sub.colValue);
                }
                else if(sub.colName == "league_level")
                {
                    item.league_level = TC_Common::strto<int>(sub.colValue);
                }
                else if(sub.colName == "partition_id")
                {
                    item.partition_id = TC_Common::strto<int>(sub.colValue);
                }
                else if(sub.colName == "win_num")
                {
                    item.win_num = TC_Common::strto<long>(sub.colValue);
                }
                else if(sub.colName == "pump_num")
                {
                    item.pump_num = TC_Common::strto<long>(sub.colValue);
                }
            }

            string keyname = I2S(item.league_level) + ":" +I2S(item.partition_id);
            auto it = map_league_user_info_group.find(keyname);
            if(it == map_league_user_info_group.end())
            {
                item.rank = 1;
                std::vector<sLeagueUserInfo> sub;
                sub.push_back(item);
                map_league_user_info_group.insert(std::make_pair(keyname, sub));
                ROLLLOG_DEBUG<<"group keyname: "<< keyname <<", uid: "<< item.uid  <<", rank: "<< item.rank << endl;
            }
            else
            {
                item.rank = it->second.size() + 1;
                it->second.push_back(item);
                ROLLLOG_DEBUG<<"group keyname: "<< keyname <<", uid: "<< item.uid << ", rank: "<< item.rank << endl;
            }
        }

        strSQL = "delete from tb_league where uid in (select gambler_id from  `config`.`sys_config_robot_gambler` where room_id not like '3%');";
        iRet =  ExcuteSql(0, 1, "tb_league", strSQL, {}, rsp);
        if(iRet != 0)
        {
            ROLLLOG_ERROR << "exec sql err. sql: "<< strSQL << ", iRet: "<< iRet << endl;
            return iRet;
        }

        reset_partition_info(map_league_user_info_group, league_id);
    }
    catch (TC_Mysql_Exception &e)
    {
        ROLLLOG_DEBUG << "select operator catch mysql exception: " << e.what() << endl;
        iRet = -1;
    }
    catch (...)
    {
        ROLLLOG_DEBUG << "select operator catch unknown exception." << endl;
        iRet = -2;
    }

    FUNC_EXIT("", iRet);
    return iRet;
}

int CDBOperator::update_newhand_league_info(long lUid)
{
   return 0;
}

//加载配置数据
int CDBOperator::loadConfig()
{
    FUNC_ENTRY("");

    int iRet = 0;

    FUNC_EXIT("", iRet);

    return iRet;
}

