#ifndef _Processor_H_
#define _Processor_H_

//
#include "globe.h"
#include "LogComm.h"
#include "DataProxyProto.h"
#include "ServiceDefine.h"
#include "util/tc_hash_fun.h"
#include "uuid.h"
#include "util/tc_base64.h"
#include <util/tc_singleton.h>
#include "ActivityServant.h"
#include "ActivityServer.h"
#include "XGameComm.pb.h"
#include "CommonStruct.pb.h"
#include "scratch.pb.h"
#include "ActivityProto.h"
#include "DBOperator.h"
#include "activity.pb.h"

//
using namespace tars;
using namespace std;
using namespace dataproxy;
using namespace dbagent;
/**
 *请求处理类
 *
 */
class Processor
{
public:
	/**
	 * 
	*/
	Processor();

	/**
	 * 
	*/
	~Processor();

public:
	int readDataFromDB(long uid, const string& table_name, const std::vector<string>& col_name, const map<string, string>& whlist, const string& order_col, int limit_num,  dbagent::TDBReadRsp &dataRsp);

	int writeDataFromDB(dbagent::Eum_Query_Type dBOPType, long uid, const string& table_name, const std::map<string, string>& col_info, const map<string, string>& whlist);

public:
	int query_scratch_detail(long uid, ScratchProto::QueryScratchDetailResp &resp);
    string create_scratch_info(long uid, ScratchProto::ScratchDetail &detail, long total_reward_num);
    void prase_scratch_info(std::vector<std::vector<int>> &scratch_info, ScratchProto::ScratchDetail &detail);
    int reward_scratch(long uid, int props_id, ScratchProto::ScratchRewardResp &resp);
    int scratch_board(ScratchProto::QueryScratchBoardResp &resp);

    int getUserAddress(tars::Int64 uid, std::string &address);
    int getUserInfo(long uid, std::map<string, string>& mapUserInfo);

    int query_achievement_info(long lUin, std::map<int, std::vector<long>>& mapResult);
    int update_achievement_status(long lUin, const ScratchProto::AchievementStatusReq &req);
    int update_achievement_info(long lUin, int iRank, long lRewardNum, int BetID, int iCondition, std::vector<long>& vResult);
	int update_achievement_unlock(long lUin, const ScratchProto::AchievementUnlockReq &req);

	int query_box(long uid, Box::QueryBoxResp &resp);
	int query_box_by_betid(long uid, int betid, Box::QueryBoxResp &resp);
	int update_box_progress_rate(long uid, int betid, int progressRate, int num, int todayNum);
	int update_box_number(long uid, int betid, int num);
	int reset_box_today_number(long uid);
	
    int update_user_league_info(long lUin, long lWinNum, long lPumpNum);
    int league_reward(long uid);

private:
	wbl::ReadWriteLocker m_rwlock; //读写锁，防止数据脏读
};

//singleton
typedef TC_Singleton<Processor, CreateStatic, DefaultLifetime> ProcessorSingleton;

#endif

