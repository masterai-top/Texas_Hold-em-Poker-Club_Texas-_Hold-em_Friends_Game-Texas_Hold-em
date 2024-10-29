#include "PreciousBox.h"
#include "Processor.h"
#include "globe.h"
#include "ConfigProto.h"
#include "TimeUtil.h"
#include "ActivityServer.h"
#include "ServiceUtil.h"

static const int BOX_TODAY_NUMBER = 30;
static const int BOX_TOTAL_NUMBER = 100;

PreciousBox::PreciousBox()
{
}

PreciousBox::~PreciousBox()
{
}

int PreciousBox::QueryBox(ActivityProto::QueryBoxReq &req, ActivityProto::QueryBoxResp &resp)
{
    int iRet = 0;
    Box::QueryBoxResp boxResp;
    iRet = ProcessorSingleton::getInstance()->query_box(req.uid(), boxResp);
    if (iRet != 0)
    {
        LOG_ERROR << "query_box err! uid: " << req.uid() << endl;
        resp.set_resultcode(iRet);
        return iRet;
    }

    for (auto &it : boxResp.mapBox)
    {
        // if (it.second.number == 0)
        //     continue;

        auto &pbBoxInfo = *resp.mutable_mapbox();
        auto &info = pbBoxInfo[it.first];
        info.set_number(it.second.number);
        info.set_todaynumber(it.second.todayNumber);
        info.set_progressrate(it.second.progressRate);
    }
    resp.set_totalnumber(boxResp.totalNumber);
    resp.set_totaltodaynumber(boxResp.totalTodayNumber);
    resp.set_resultcode(iRet);
    LOG_DEBUG << "uid" << req.uid() << resp.DebugString() << endl;
    return iRet;
}

int PreciousBox::ReportBoxProgressRate(const Box::BoxProgressRateReq &req, Box::BoxProgressRateResp &resp)
{
    int iRet = 0;

    Box::QueryBoxResp queryboxResp;
    iRet = ProcessorSingleton::getInstance()->query_box(req.uid, queryboxResp);
    if (iRet != 0)
    {
        LOG_ERROR << "query_box err! uid: " << req.uid << endl;
        return iRet;
    }

    LOG_DEBUG << " iBetID:" << req.betId << " queryboxResp:" << logTars(queryboxResp) << endl;

    if (queryboxResp.totalTodayNumber >= BOX_TODAY_NUMBER)
        return -1;

    if (queryboxResp.totalNumber >= BOX_TOTAL_NUMBER)
        return -1;

    int num = 0;
    int todayNum = 0;
    int progressRate = req.progressRate;
    auto it = queryboxResp.mapBox.find(req.betId);
    if (it != queryboxResp.mapBox.end())
    {
        num = it->second.number;
        todayNum = it->second.todayNumber;
        progressRate += it->second.progressRate;
    }

    if (progressRate >= 10000)
    {
        int curCount = progressRate / 10000;
        LOG_DEBUG << "uid:" << req.uid << " progressRate:" << progressRate << " num:" << num << " curCount:" << curCount << endl;
        progressRate %= 10000;
        
        if ((queryboxResp.totalTodayNumber + curCount) > BOX_TODAY_NUMBER)
        {
            curCount = BOX_TODAY_NUMBER > queryboxResp.totalTodayNumber ? BOX_TODAY_NUMBER - queryboxResp.totalTodayNumber : 0;
            progressRate = 0;
        }
        
        if ((queryboxResp.totalNumber + curCount) > BOX_TOTAL_NUMBER)
        {
            curCount = BOX_TOTAL_NUMBER > queryboxResp.totalNumber ? BOX_TOTAL_NUMBER - queryboxResp.totalNumber : 0;
            progressRate = 0;
        }

        num += curCount;
        todayNum += curCount;
        PushGetBox(req.uid, req.betId, num);
    }

    LOG_DEBUG << "uid:" << req.uid << " progressRate:" << progressRate << " num:" << num << " todayNum:" << todayNum << endl;

    iRet = ProcessorSingleton::getInstance()->update_box_progress_rate(req.uid, req.betId, progressRate, num, todayNum);
    if (iRet != 0)
    {
        LOG_ERROR << "update_box_progress_rate err! uid: " << req.uid << endl;
        return iRet;
    }
    return iRet;
}

int PreciousBox::GetBoxReward(const ActivityProto::BoxRewardReq &req, ActivityProto::BoxRewardResp &resp)
{
    int iRet = 0;

    Box::QueryBoxResp queryboxResp;
    iRet = ProcessorSingleton::getInstance()->query_box(req.uid(), queryboxResp);
    if (iRet != 0)
    {
        LOG_ERROR << "query_box err! uid: " << req.uid() << endl;
        resp.set_resultcode(iRet);
        return iRet;
    }

    auto it = queryboxResp.mapBox.find(req.betid());
    if (it == queryboxResp.mapBox.end())
    {
        LOG_ERROR << "queryboxResp.mapBox.find err! uid: " << req.uid() << " betId:" << req.betid() << endl;
        resp.set_resultcode(-1);
        return -1;
    }

    if (it->second.number < 1)
    {
        LOG_ERROR << "err! uid: " << req.uid() << " it->second.number:" << it->second.number << " betId:" << req.betid() << endl;
        resp.set_resultcode(-1);
        return -1;        
    }

    int num = it->second.number - 1;
    iRet = ProcessorSingleton::getInstance()->update_box_number(req.uid(), req.betid(), num);
    if (iRet != 0)
    {
        LOG_ERROR << "update_box_progress_rate err! uid: " << req.uid() << endl;
        resp.set_resultcode(iRet);
        return iRet;
    }

    //奖励
    config::PreciousBoxResp cfgResp;
    iRet = g_app.getOuterFactoryPtr()->getConfigServantPrx()->getPreciousBoxCfg(cfgResp);
    if (iRet != 0)
    {
        LOG_ERROR << "getPreciousBoxCfg err! uid: " << req.uid() << " iRet:" << iRet << endl;
        resp.set_resultcode(iRet);
        return iRet;
    }

    auto itCfg = cfgResp.data.find(req.betid());
    if (itCfg == cfgResp.data.end())
    {
        LOG_ERROR << "cfgResp.data.find err! uid: " << req.uid() << " req.betid:" << req.betid() << endl;
        resp.set_resultcode(-1);
        return iRet;
    }

    //根据奖励ID获取奖励配置
    config::SeasonRewardsCfgResp seasonRewardsCfgResp;
    g_app.getOuterFactoryPtr()->getConfigServantPrx()->ListSeasonRewardsCfg(seasonRewardsCfgResp);
    auto rewardId = ServiceUtil::calculate_weight(itCfg->second.rewardsId);
    auto iter = seasonRewardsCfgResp.data.find(rewardId);
    if (iter != seasonRewardsCfgResp.data.end())
    {
        ROLLLOG_DEBUG << "rewards id:" << iter->second.id
                      << ",points:" << iter->second.points << ",chips:" << iter->second.chips << ",tickets:" << iter->second.tickets
                      << ",icon:" << iter->second.icon << ",props_size:" << iter->second.props.size() << endl;
        if ((iter->second.points > 0)  )
        {
            auto rewards = resp.add_rewards();
            rewards->set_propsid(10000);
            rewards->set_number(iter->second.points);
        }
        if (iter->second.chips > 0)
        {
            auto rewards = resp.add_rewards();
            rewards->set_propsid(20000);
            rewards->set_number(iter->second.chips);
        }
        if (iter->second.tickets > 0)
        {
            auto rewards = resp.add_rewards();
            rewards->set_propsid(30000);
            rewards->set_number(iter->second.tickets);
        }
        for (const auto &itt : iter->second.props)
        {
            auto rewards = resp.add_rewards();
            rewards->set_propsid(itt.id);
            rewards->set_number(itt.number);
        }
    }

    for (auto &itprops : resp.rewards())
    {
        //发送金币
        GoodsManager::GiveGoodsReq giveGoodsReq;
        GoodsManager::GiveGoodsRsp giveGoodsRsp;
        giveGoodsReq.uid = req.uid();
        giveGoodsReq.goods.goodsID = itprops.propsid();
        giveGoodsReq.goods.count = itprops.number();

        if (itprops.propsid() == 20000)
            giveGoodsReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_BOX_REWARD;

        iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(req.uid())->giveGoods(giveGoodsReq, giveGoodsRsp);
        if (iRet != 0)
        {
            LOG_ERROR << "reward gold err. iRet: " << iRet  << " propID:" << itprops.propsid() << endl;
        }
    }

    return iRet;
}

void PreciousBox::PushGetBox(tars::Int64 uid, int betId, int num)
{
    FUNC_ENTRY("");
    __TRY__

    push::PushMsgReq pushMsgReq;
    push::PushMsg pushMsg;
    pushMsg.uid = uid;
    pushMsg.msgType = push::E_PUSH_MSG_TYPE_GET_BOX_NOTIFY;

    push::PreciousBoxNotify tarsNotify;
    tarsNotify.iBetId = betId;
    tarsNotify.number = num;
    tobuffer(tarsNotify, pushMsg.vecData);

    pushMsgReq.msg.push_back(pushMsg);
    g_app.getOuterFactoryPtr()->getPushServantPrx(uid)->async_pushMsg(NULL, pushMsgReq);

    ROLLLOG_DEBUG << "uid: " << uid << endl;

    __CATCH__
    FUNC_EXIT("", 0);
}