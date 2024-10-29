#ifndef _ActivityServantImp_H_
#define _ActivityServantImp_H_

#include "servant/Application.h"
#include "ActivityServant.h"
#include "Lottery.pb.h"
#include "scratch.pb.h"
#include "Processor.h"
#include "DBOperator.h"
#include "TimerThread.h"

namespace XGameComm
{
    class TPackage;
    enum MSGTYPE : int;
}

namespace XGameProto
{
    enum ActionName : int;
}
namespace Box
{
    class BoxProgressRateReq;
    class BoxProgressRateResp;
    class QueryBoxReq;
    class QueryBoxResp;
}
/**
 *
 *
 */
class ActivityServantImp : public activity::ActivityServant
{
public:
    /**
     *
     */
    virtual ~ActivityServantImp() {}

    /**
     *
     */
    virtual void initialize();

    /**
     *
     */
    
    void initializeTimer();
    virtual void destroy();
    virtual tars::Int32 onRequest(tars::Int64 lUin, const std::string &sMsgPack, const std::string &sCurServrantAddr, const JFGame::TClientParam &stClientParam, const JFGame::UserBaseInfoExt &stUserBaseInfo, tars::TarsCurrentPtr current);

    /**************************转盘****************************/
    virtual tars::Int32 lotteryGo(const lottery::LotteryReq &req, lottery::LotteryResp &resp, tars::TarsCurrentPtr current);
    virtual tars::Int32 queryLotteryDetail(const lottery::QueryLotteryDetailReq &req, lottery::QueryLotteryDetailResp &resp, tars::TarsCurrentPtr current);

    //上报成就信息
    virtual tars::Int32 reportAchievementInfo(tars::Int64 lUin, tars::Int32 iRank, tars::Int64 lRewardNum, tars::Int32 BetID, tars::Int32 iCondition, tars::TarsCurrentPtr current);
    //获取成就信息
    virtual tars::Int32 getAchievementInfo(tars::Int64 lUin, map<tars::Int32, tars::Int64>& mapAchievementInfo, tars::TarsCurrentPtr current);
    //上报赛季输赢
    virtual tars::Int32 reportLeagueStat(tars::Int64 lUin, tars::Int64 lWinNum, tars::Int64 lPumpNum, tars::TarsCurrentPtr current);
    //获取赛季信息
    virtual tars::Int32 getLeagueInfo(tars::Int64 lUin, tars::TarsCurrentPtr current);
    //上报宝箱进度
    virtual tars::Int32 reportBoxProgressRate(const Box::BoxProgressRateReq &req, Box::BoxProgressRateResp &resp, tars::TarsCurrentPtr current);
	//查询宝箱信息
	virtual tars::Int32 queryBox(const Box::QueryBoxReq &req, Box::QueryBoxResp &resp, tars::TarsCurrentPtr current);

    //转盘查询
    virtual tars::Int32 queryLuckeyWheel(tars::Int64 lUin, LuckeyWheel::QueryLuckeyWheelResp &resp, tars::TarsCurrentPtr current);

private:
    /**************************转盘****************************/
    tars::Int32 onLotteryGo(const XGameComm::TPackage &pkg, const LotteryProto::LotteryReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    tars::Int32 onQueryLotteryDetail(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);

    /**************************刮刮乐**************************/
    int onQueryScratchDetail(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    int onGetScratchReward(const XGameComm::TPackage &pkg, const ScratchProto::ScratchRewardReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    int onObtainScratch(const XGameComm::TPackage &pkg, const ScratchProto::ObtainScratchReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    int onQueryScratchBoard(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);

    //成就
    int onGetAchievementInfo(const XGameComm::TPackage &pkg, const ScratchProto::QueryAchievementInfoReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    int onGetAchievementInfo2(const XGameComm::TPackage &pkg, const ScratchProto::QueryAchievementInfoReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    int onAchievementStatus(const XGameComm::TPackage &pkg, const ScratchProto::AchievementStatusReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    int onAchievementBoard(const XGameComm::TPackage &pkg, const ScratchProto::QueryAchievementBoardReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    int onAchievementUnlock(const XGameComm::TPackage &pkg, const ScratchProto::AchievementUnlockReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);

    //联赛
    int onLeagueBoard(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    int onGetLeagueRewardInfo(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);
    int onGetLeagueReward(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current);

    //转盘
    int onQueryLuckyWheel(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, const std::string &msg);
    int onLuckyWheelLottery(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, const std::string &msg);
    void LuckyWhellLog(long uid, int propsId, long propsNum, int type, int lotteryId);

    //jackpot
    int onJackpotGo(const XGameComm::TPackage &pkg, const ActivityProto::JackpotGoReq &req, ActivityProto::JackpotGoResp &resp);

    template<typename T>
    int toClientPb(const XGameComm::TPackage &tPackage, const std::string &sCurServrantAddr, XGameProto::ActionName actionName, const T &t);
    template<typename T>
    int toClientPb(const XGameComm::TPackage &tPackage, const std::string &sCurServrantAddr, XGameProto::ActionName actionName, XGameComm::MSGTYPE type, const T &t);

public:
    //时钟周期回调
    virtual tars::Int32 doCustomMessage(bool bExpectIdle = false);

private:
    //定时器管理
    TimerThread m_threadTimer;
};
/////////////////////////////////////////////////////
#endif
