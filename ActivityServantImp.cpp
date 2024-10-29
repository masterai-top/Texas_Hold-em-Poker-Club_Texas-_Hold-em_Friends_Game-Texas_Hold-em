#include "ActivityServantImp.h"
#include "servant/Application.h"
#include "LogComm.h"
#include "globe.h"
#include "ServiceUtil.h"
#include "DBAgentServant.h"
#include "ActivityServer.h"
#include "XGameComm.pb.h"
#include "CommonStruct.pb.h"
#include "Push.h"
#include "HallServant.h"
#include "RoomServant.h"
#include "LogDefine.h"
#include "util/tc_hash_fun.h"
#include "JFGameHttpProto.h"
#include "DataProxyProto.h"
#include "ServiceDefine.h"
#include "CommonCode.h"
#include "JFGame.h"
#include "PreciousBox.h"
#include "activity.pb.h"
#include "TimeUtil.h"
#include "ConfigProto.h"
#include "CommonCode.pb.h"

using namespace std;
using namespace JFGame;
using namespace dataproxy;
using namespace dbagent;
using namespace JFGameHttpProto;

//////////////////////////////////////////////////////
void ActivityServantImp::initialize()
{
    //initialize servant here:
    //...
    initializeTimer();
}

//////////////////////////////////////////////////////
void ActivityServantImp::destroy()
{
    //destroy servant here:
    //...
}

void ActivityServantImp::initializeTimer()
{
    m_threadTimer.initialize(this);
    m_threadTimer.start();
}


static std::map<std::string, std::string> parseFieldRow(vector<dbagent::TField> &fields)
{
    map<string, string> result;
    for (auto &field : fields)
    {
        result[field.colName] = field.colValue;
    }
    return result;
}

//拆分字符串
static vector<std::string> split(const string &str, const string &pattern)
{
    return TC_Common::sepstr<string>(str, pattern);
}

static int constructTFields(const std::vector<std::map<std::string, std::string>> &param, std::vector<dbagent::TField> &fields)
{
    fields.clear();
    for (auto &m : param)
    {
        dbagent::TField field;
        field.colArithType = E_NONE;
        auto it = m.find("colName");
        if (it != m.end())
            field.colName = it->second;

        it = m.find("colType");
        if (it != m.end())
            field.colType = (dbagent::Eum_Col_Type)(S2I(it->second));

        it = m.find("colValue");
        if (it != m.end())
            field.colValue = it->second;

        it = m.find("colAs");
        if (it != m.end())
            field.colAs = it->second;

        it = m.find("colArithType");
        if (it != m.end())
            field.colArithType = (dbagent::Eum_Col_Arith_Type)(S2I(it->second));

        fields.push_back(field);
    }

    return 0;
}

static tars::Int32 readProxyDataLottery(long uid, dataproxy::TReadDataRsp &dataRsp)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__
    ROLLLOG_DEBUG << "uid : " << uid << endl;
    dataproxy::TReadDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(LOTTERY) + ":" + L2S(uid);
    dataReq.operateType = dataproxy::E_REDIS_READ;
    dataReq.clusterInfo.resetDefautlt();
    dataReq.clusterInfo.busiType = dataproxy::E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = dataproxy::E_FRAGE_FACTOR_USER_ID;
    dataReq.clusterInfo.frageFactor = uid;

    vector<map<string, string>> dataVec =
    {
        { {"colName", "uid"}, { "colType", I2S(dbagent::BIGINT) } },
        { { "colName", "free_rotate_times" }, { "colType", I2S(dbagent::INT) } },
        { { "colName", "accumulated_rotate_times" }, { "colType", I2S(dbagent::INT) } },
        { { "colName", "historical_rotate_times" }, { "colType", I2S(dbagent::INT) } },
        { { "colName", "cooldown" }, { "colType", I2S(dbagent::STRING) } },
        { { "colName", "rotate_queue" }, { "colType", I2S(dbagent::STRING) }},
        {{ "colName", "order_queue" }, { "colType", I2S(dbagent::STRING) }}
    };

    constructTFields(dataVec, dataReq.fields);
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(uid)->redisRead(dataReq, dataRsp);
    ROLLLOG_DEBUG << "get lottery data, iRet: " << iRet << ", dataRsp: " << printTars(dataRsp) << endl;
    if (iRet != 0 || dataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "get lottery map err, iRet: " << iRet << ", iResult: " << dataRsp.iResult << endl;
        return -2;
    }
    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

//TODO 写入数据库的地方增加order_types 字段
static int writeProxyDataLottery(long uid, dataproxy::Eum_Redis_Operate_Type operrateType, const vector<map<string, string>> &dataVec, dataproxy::TWriteDataRsp &dataRsp)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__
    ostringstream os;
    os << "uid : " << uid << ", operrateType : " << operrateType <<  " dataVec : " << endl; //TODO 有时间将其打印为JSON
    for (size_t i = 0; i < dataVec.size(); ++i)
    {
        for (auto &item : dataVec[i])
        {
            os << item.first << " : " << item.second << endl;
        }
    }

    ROLLLOG_DEBUG << os.str();
    dataproxy::TWriteDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(LOTTERY) + ":" + L2S(uid);
    dataReq.operateType = operrateType;
    dataReq.clusterInfo.resetDefautlt();
    dataReq.clusterInfo.busiType = dataproxy::E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = dataproxy::E_FRAGE_FACTOR_USER_ID;
    dataReq.clusterInfo.frageFactor = uid;

    constructTFields(dataVec, dataReq.fields);
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(uid)->redisWrite(dataReq, dataRsp);
    ROLLLOG_DEBUG << "write lottery data, iRet: " << iRet << ", dataRsp: " << printTars(dataRsp) << endl;
    if (iRet != 0 || dataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "write lottery map err, iRet: " << iRet << ", iResult: " << dataRsp.iResult << endl;
        return -1;
    }
    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

tars::Int32 ActivityServantImp::onRequest(tars::Int64 lUin, const std::string &sMsgPack, const std::string &sCurServrantAddr, const JFGame::TClientParam &stClientParam, const JFGame::UserBaseInfoExt &stUserBaseInfo, tars::TarsCurrentPtr current)
{
    int iRet = 0;

    __TRY__

    ROLLLOG_DEBUG << "recv msg, uid : " << lUin << ", addr : " << stClientParam.sAddr << endl;

    ActivityServant::async_response_onRequest(current, 0);

    XGameComm::TPackage pkg;
    pbToObj(sMsgPack, pkg);
    if (pkg.vecmsghead_size() == 0)
    {
        ROLLLOG_DEBUG << "package empty." << endl;
        return -1;
    }

    ROLLLOG_DEBUG << "recv msg, uid : " << lUin << ", msg : " << logPb(pkg) << endl;

    for (int i = 0; i < pkg.vecmsghead_size(); ++i)
    {
        int64_t ms1 = TNOWMS;

        auto &head = pkg.vecmsghead(i);
        switch (head.nmsgid())
        {
        case XGameProto::ActionName::LOTTERY_GO:
        {
            LotteryProto::LotteryReq lotteryReq;
            pbToObj(pkg.vecmsgdata(i), lotteryReq);
            iRet = onLotteryGo(pkg, lotteryReq, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::LOTTERY_QUERY:
        {
            iRet = onQueryLotteryDetail(pkg, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::SCRATCH_DETAIL_QUERY:
        {
            iRet = onQueryScratchDetail(pkg, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::SCRATCH_REWARD:
        {
            ScratchProto::ScratchRewardReq scratchReq;
            pbToObj(pkg.vecmsgdata(i), scratchReq);
            iRet = onGetScratchReward(pkg, scratchReq, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::SCRATCH_BOARD:
        {
            iRet = onQueryScratchBoard(pkg, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::ACHIEVEMENT_DETAIL_QUERY:
        {
            ScratchProto::QueryAchievementInfoReq achievmentReq;
            pbToObj(pkg.vecmsgdata(i), achievmentReq);
            iRet = onGetAchievementInfo(pkg, achievmentReq, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::ACHIEVEMENT_DETAIL_QUERY2:
        {
            ScratchProto::QueryAchievementInfoReq achievmentReq;
            pbToObj(pkg.vecmsgdata(i), achievmentReq);
            iRet = onGetAchievementInfo2(pkg, achievmentReq, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::ACHIEVEMENT_UNLOCK:
        {
            ScratchProto::AchievementUnlockReq achievmentReq;
            pbToObj(pkg.vecmsgdata(i), achievmentReq);
            iRet = onAchievementUnlock(pkg, achievmentReq, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::ACHIEVEMENT_STATUS:
        {
            ScratchProto::AchievementStatusReq achievmentReq;
            pbToObj(pkg.vecmsgdata(i), achievmentReq);
            iRet = onAchievementStatus(pkg, achievmentReq, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::ACHIEVEMENT_BOARD:
        {
            ScratchProto::QueryAchievementBoardReq achievmentReq;
            pbToObj(pkg.vecmsgdata(i), achievmentReq);
            iRet = onAchievementBoard(pkg, achievmentReq, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::LEAGUE_BOARD:
        {
            iRet = onLeagueBoard(pkg, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::LEAGUE_REWARD:
        {
            iRet = onGetLeagueRewardInfo(pkg, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::LEAGUE_REWARD_REVICE:
        {
            iRet = onGetLeagueReward(pkg, sCurServrantAddr, current);
            break;
        }
        case XGameProto::ActionName::ACTIVITY_BOX_QUERY:
        {
            ActivityProto::QueryBoxReq pbBoxReq;
            pbToObj(pkg.vecmsgdata(i), pbBoxReq);

            ActivityProto::QueryBoxResp pbBoxResp;
            iRet = PreciousBoxSingleton::getInstance()->QueryBox(pbBoxReq, pbBoxResp);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ACTIVITY_BOX_QUERY, XGameComm::MSGTYPE_RESPONSE, pbBoxResp);
            break;
        }
        case XGameProto::ActionName::ACTIVITY_BOX_GET_REWARDS:
        {
            ActivityProto::BoxRewardReq pbBoxReq;
            pbToObj(pkg.vecmsgdata(i), pbBoxReq);
            
            ActivityProto::BoxRewardResp pbBoxResp;
            iRet = PreciousBoxSingleton::getInstance()->GetBoxReward(pbBoxReq, pbBoxResp);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ACTIVITY_BOX_GET_REWARDS, XGameComm::MSGTYPE_RESPONSE, pbBoxResp);
            break;
        }
        case XGameProto::ActionName::ACTIVITY_LUCKY_WHEEL_QUERY:
        {
            iRet = onQueryLuckyWheel(pkg, sCurServrantAddr, pkg.vecmsgdata(i));
            break;
        }
        case XGameProto::ActionName::ACTIVITY_LUCKY_WHEEL_LOTTERY:
        {
            iRet = onLuckyWheelLottery(pkg, sCurServrantAddr, pkg.vecmsgdata(i));
            break;
        }
        case XGameProto::ActionName::ACTIVITY_JACKPOT:
        {
            ActivityProto::JackpotGoReq req;
            ActivityProto::JackpotGoResp resp;
            pbToObj(pkg.vecmsgdata(i), req);
            iRet = onJackpotGo(pkg, req, resp);
            resp.set_resultcode(iRet);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ACTIVITY_JACKPOT, XGameComm::MSGTYPE_RESPONSE, resp);
            break;
        }
        default:
        {
            ROLLLOG_ERROR << "invalid msg id, uid: " << lUin << ", msg id: " << head.nmsgid() << endl;
            break;
        }
        }

        if (iRet != 0)
        {
            ROLLLOG_ERROR << "process msg fail, uid: " << lUin << ", msg id: " << head.nmsgid() << ", iRet: " << iRet << endl;
        }
        else
        {
            ROLLLOG_DEBUG << "process msg succ, uid: " << lUin << ", msg id: " << head.nmsgid() << ", iRet: " << iRet << endl;
        }

        int64_t ms2 = TNOWMS;
        if ((ms2 - ms1) > COST_MS)
        {
            ROLLLOG_WARN << "@Performance, msgid:" << head.nmsgid() << ", costTime:" << (ms2 - ms1) << endl;
        }
    }

    __CATCH__

    return iRet;
}

static const int FREQUENCY_NULL = -1;

// 写入操作时,把操作和DropId同时写入DB
struct RotateOperator
{
    int rotateType;//旋转类型
    int dropId;  //掉落组ID
    int rotateId;//旋转组的ID
};

//TODO 或许所有的operatoer[] 都应该换成at函数,越界的时候可以检查
static map<int, int> decodeOrderQueue(const string &str)
{
    vector<string> orderVec = split(str, "|");
    map<int, int> result;
    for (size_t i = 0; i < orderVec.size(); ++i)
    {
        string item = orderVec[i];
        vector<string> tempResult = split(item, ":");
        int key = S2I(tempResult[0]);
        int value = S2I(tempResult[1]);
        result[key] = value;
    }

    return result;
}

static string encodeOrderQueue(const map<int, int> &input)
{
    if (input.empty())
    {
        return string();
    }

    string str;
    for (auto it = input.begin(); it != input.end(); it++)
    {
        ostringstream os;
        os << it->first << ":" << it->second << "|";
        str += os.str();
    }
    str.resize(str.size() - 1);//丢弃最后一个"|"
    return str;
}

//序列化数据,然后保存起来
static string encode(const vector<string> &strs, const string &pattern)
{
    string ret;
    size_t len = strs.size();
    for (size_t i = 0; i < len; ++i)
    {
        ret += strs[i];
        if (i != len - 1)
        {
            ret += "|";
        }
    }

    return ret;
}

static int getCurrentOrderByRotateId(int rotateId, const map<int, int> &order_queue)
{
    int order = -1;

    try
    {
        order = order_queue.at(rotateId);
    }
    catch (...)
    {
        ROLLLOG_ERROR << "order_queue out of range, type : " << rotateId << endl;
        TERMINATE_APPLICATION();
    }

    return order;
}


//发奖的代码抽离出来
static int giveAward(const lottery::LotteryReq &req, lottery::LotteryResp &resp, vector<string> &rotate_queue_vec,
                     vector<string> &cooldownDeadlineVec, int dropId)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__
    //TODO 通过rotateId 读取奖励信息,rotateId直接从数据库里面读取,在写入操作的时候记录进去数据库
    auto &awards = g_app.getOuterFactoryPtr()->getAwardByDropId(dropId);//TODO 这里或许客户端不需要传入旋转类型
    // 产生一个随机数,RPC发奖励 --TODO 这部分的代码可以抽离出来
    int total = 0;
    for_each(awards.begin(), awards.end(), [&total](const Award & param)->void
    {
        total += param.probability;
    });

    time_t now = TC_TimeProvider::getInstance()->getNow();
    srand(now);
    int luckyNum = rand() % total;
    int right_edge = 0;
    for (auto &award : awards)
    {
        right_edge += award.probability;
        if (right_edge > luckyNum)
        {
            resp.rewardId = award.Id;
            //发放奖励
            GoodsManager::GiveGoodsReq  giveGoodsReq;
            giveGoodsReq.uid = req.uid;
            giveGoodsReq.goods.goodsID = award.PropId;
            giveGoodsReq.goods.count = award.Number;
            if(lottery::E_LOTTERY_FREE == req.rotateType)
            {
                 giveGoodsReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_LUCKEY_ROTATE;
            }
            else
            {
                giveGoodsReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SUPER_ROTATE;
            }
            GoodsManager::GiveGoodsRsp giveGoodsRsp;
            iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(req.uid)->giveGoods(giveGoodsReq, giveGoodsRsp);
            if (iRet != 0 || giveGoodsRsp.resultCode != 0)
            {
                ROLLLOG_ERROR << "giveGoods error!" << "iRet : " << iRet << ", resultCode : " << giveGoodsRsp.resultCode << endl;
                return -1;
            }

            rotate_queue_vec.erase(rotate_queue_vec.begin());//去掉一个操作
            cooldownDeadlineVec.erase(cooldownDeadlineVec.begin());
            break;
        }
    }

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

static RotateOperator parseOperator(const string &input)
{
    RotateOperator result;
    vector<string> vec = split(input, ":");
    result.rotateType = S2I(vec[0]);
    result.dropId = S2I(vec[1]);
    result.rotateId = S2I(vec[2]);
    return result;
}

//[item0, item1]
// item0:RotateQueueItem
// item1:Cooldown
//TODO 这里要进行相应的处理
static void getRotateQueueItemAndCooldown(lottery::E_LOTTERY_TYPE rotateType, int rotaimeTimes,
        const map<int, int> &order_queue, vector<string> &rotate_queue_vec, vector<string> &cooldownDeadlineVec)
{
    // const int MEASURE = 60;
    time_t now = TC_TimeProvider::getInstance()->getNow();
    int rotateId = g_app.getOuterFactoryPtr()->getRotateId(rotateType, rotaimeTimes);
    int order = getCurrentOrderByRotateId(rotateId, order_queue);//order 用encode order_queue 对象存入数据库
    int dropId = g_app.getOuterFactoryPtr()->getDropId(rotateId, order);
    stringstream os;
    os << rotateType << ":" << dropId << ":" << rotateId;
    string str = os.str();
    rotate_queue_vec.push_back(str);
    int interval = g_app.getOuterFactoryPtr()->getRotationInfo(rotateId, order).time;
    int rotateTime = g_app.getOuterFactoryPtr()->getRotateTime();
    cooldownDeadlineVec.push_back(I2S(now + interval * rotateTime));
}

//TODO 中间新增了旋转组怎么办
tars::Int32 ActivityServantImp::lotteryGo(const lottery::LotteryReq &req, lottery::LotteryResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    ROLLLOG_DEBUG << "req : " << printTars(req) << endl;

    //step1: 读取数据库
    dataproxy::TReadDataRsp dataRsq;
    iRet = readProxyDataLottery(req.uid, dataRsq);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "Read Lottery data error!" << endl;
        resp.resultCode = -1;
        return -1;
    }

    if (dataRsq.fields.empty())
    {
        ROLLLOG_ERROR << "invalid operator ! try lottery without entity! uid : " << req.uid << ", type : " << req.rotateType << endl;
        return -2;
    }

    //step2:解析数据
    map<std::string, std::string> data = parseFieldRow(dataRsq.fields[0]);
    int free_rotate_times = S2I(data["free_rotate_times"]);
    int accumulated_rotate_times = S2I(data["accumulated_rotate_times"]);
    int historical_rotate_times = S2I(data["historical_rotate_times"]);
    string cooldownDeadlineraw = data["cooldown"];
    vector<string> cooldownDeadlineVec = split(cooldownDeadlineraw, "|");
    int cooldownDeadline = S2I(cooldownDeadlineVec.front());
    string rotate_queue = data["rotate_queue"];
    vector<string> rotate_queue_vec = split(rotate_queue, "|");
    string str_order_queue = data["order_queue"];
    map<int, int> order_queue = decodeOrderQueue(str_order_queue);//通过次序order和rotateId获取掉落组

    //step3: 条件判断
    if (TNOW < cooldownDeadline)
    {
        ROLLLOG_ERROR << "invalid operator ! try lottery in cooldowm period ! uid : " << req.uid << ", type : " << req.rotateType << endl;
        return -3;
    }

    RotateOperator nextOpt = parseOperator(rotate_queue_vec.front());
    lottery::E_LOTTERY_TYPE nextOptType = (lottery::E_LOTTERY_TYPE)(nextOpt.rotateType);
    if (rotate_queue_vec.empty() == false && req.rotateType != nextOptType)
    {
        ROLLLOG_ERROR << "invalid operator! Request rotateType not correspond to nextOptType! uid : " << req.uid << endl;
        return -4;
    }

    //step4: 抽奖
    int curOrder = getCurrentOrderByRotateId(nextOpt.rotateId, order_queue);
    iRet = giveAward(req, resp, rotate_queue_vec, cooldownDeadlineVec, nextOpt.dropId);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "give award error!" << endl;
        return -1;
    }

    // 更新order_queue
    {
        auto &rotationList = g_app.getOuterFactoryPtr()->getRotationInfo();
        for (auto it = rotationList.begin(); it != rotationList.end(); ++it)
        {
            int rotationId = it->first;
            auto ptr = order_queue.find(rotationId);
            if (ptr == order_queue.end()) //以后更新配置可能会加入新的旋转组
            {
                order_queue[rotationId] = 0;
            }
        }

        order_queue[nextOpt.rotateId] += 1;

        //可能会溢出
        if (g_app.getOuterFactoryPtr()->order_out_of_range(order_queue[nextOpt.rotateId], nextOpt.rotateId))
        {
            order_queue[nextOpt.rotateId] = 0;
        }
    }

    //TODO 这部分注释代码等测试通过后删除
    //auto& awards = g_app.getOuterFactoryPtr()->getAwardByLotteryType(req.rotateType);//TODO 这里或许客户端不需要传入旋转类型
    //// 产生一个随机数,RPC发奖励 --TODO 这部分的代码可以抽离出来
    //int total = 0;
    //for_each(awards.begin(), awards.end(), [&total](const Award& param)->void {
    //  total += param.probability;
    //});
    //srand(now);
    //int luckyNum = rand() % total;
    //int right_edge = 0;
    //for (auto& award : awards)
    //{
    //  right_edge += award.probability;
    //  if (right_edge > luckyNum)
    //  {
    //      resp.rewardId = award.Id;
    //      //发放奖励
    //      GoodsManager::GiveGoodsReq  giveGoodsReq;
    //      giveGoodsReq.uid = req.uid;
    //      giveGoodsReq.goods.goodsID = award.PropId;
    //      giveGoodsReq.goods.count = award.Number;
    //      GoodsManager::GiveGoodsRsp giveGoodsRsp;
    //      iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(req.uid)->giveGoods(giveGoodsReq, giveGoodsRsp);
    //      if (iRet != 0 || giveGoodsRsp.resultCode != 0)
    //      {
    //          ROLLLOG_ERROR << "giveGoods error!" << "iRet : " << iRet << ", resultCode : " << giveGoodsRsp.resultCode << endl;
    //          return -1;
    //      }
    //      rotate_queue_vec.erase(rotate_queue_vec.begin());//去掉一个操作
    //      cooldownDeadlineVec.erase(cooldownDeadlineVec.begin());
    //      break;
    //  }
    //}

    // step5 : 设置状态
    //if (req.rotateType == lottery::E_LOTTERY_FREE)
    //{
    //free_rotate_times += 1;   //免费转5次(可配置)给一次超级旋转,免费旋转次数置零 ==> 不管是哪种转法,次数都会加1 TODO 此处应该读取配置 ==必须解决
    //}

    int accumulate = g_app.getOuterFactoryPtr()->getAccumulate(nextOpt.rotateId, curOrder);
    if (req.rotateType == lottery::E_LOTTERY_LOOP)
    {
        free_rotate_times -= g_app.getOuterFactoryPtr()->getMaxLoopTimes();//如果循环旋转配置多个会有问题 TODO
        if (accumulate == 0)
        {
            //循环旋转(即超级旋转)次数记录到历史旋转次数中
            historical_rotate_times += 1;
        }
    }

    accumulated_rotate_times  += accumulate;  //累计旋转次数到一定值(可配置)激活循环旋转
    historical_rotate_times   += accumulate;  //不清零,到一定次数给首次旋转
    free_rotate_times         += accumulate;  //免费旋转次数

    // 插入下次的操作  写入OPT 对应的rotateID  TODO --这部分代码调试通过即可
    {
        ROLLLOG_DEBUG << "historical_rotate_times: " << historical_rotate_times << ", accumulated_rotate_times: " << accumulated_rotate_times << endl;
        if (g_app.getOuterFactoryPtr()->achieveHistoryRotateCondition(historical_rotate_times))
        {
            //需要利用测试用例单步进来验证代码是否正确--TODO
            getRotateQueueItemAndCooldown(lottery::E_LOTTERY_FIRST_TIME, historical_rotate_times, order_queue, rotate_queue_vec, cooldownDeadlineVec);

            ROLLLOG_DEBUG << "rotate_queue_vec: " << rotate_queue_vec.size() << ", cooldownDeadlineVec: " << cooldownDeadlineVec.size() << endl;
        }

        if (g_app.getOuterFactoryPtr()->achieveLoopRotation(accumulated_rotate_times))
        {
            getRotateQueueItemAndCooldown(lottery::E_LOTTERY_LOOP, accumulated_rotate_times, order_queue, rotate_queue_vec, cooldownDeadlineVec);

            if (g_app.getOuterFactoryPtr()->needReset_accumulated_rotate_times(accumulated_rotate_times))
            {
                accumulated_rotate_times = 0;
            }
            ROLLLOG_DEBUG << "rotate_queue_vec: " << rotate_queue_vec.size() << ", cooldownDeadlineVec: " << cooldownDeadlineVec.size() << endl;
        }

       /* if (free_rotate_times == g_app.getOuterFactoryPtr()->getSuperNeedFreeTimes() && req.rotateType == lottery::E_LOTTERY_FREE)
        {
            //TODO 这里的rotaimeTimes 怎么传入呢?  -- 免费旋转和超级旋转的旋转次数不知道应该怎么传入
            getRotateQueueItemAndCooldown(lottery::E_LOTTERY_SUPER, -1, order_queue, rotate_queue_vec, cooldownDeadlineVec);
        }*/

        if (rotate_queue_vec.empty())// 操作队列无操作的时候才插入普通旋转操作
        {
            getRotateQueueItemAndCooldown(lottery::E_LOTTERY_FREE, -1, order_queue, rotate_queue_vec, cooldownDeadlineVec);
        }
    }

    //step6 : 写入DB resp的设置
    string newCooldownDeadline = encode(cooldownDeadlineVec, "|");
    string new_rotate_queue_vec = encode(rotate_queue_vec, "|");

    ROLLLOG_DEBUG << "newCooldownDeadline: " << newCooldownDeadline << ", new_rotate_queue_vec: " << new_rotate_queue_vec << endl;
    string new_order_queue_str = encodeOrderQueue(order_queue);
    vector<map<string, string>> dataVec =
    {
        { { "colName", "uid" }, { "colType", I2S(dbagent::BIGINT) }, { "colValue", L2S(req.uid) } },
        { { "colName", "free_rotate_times" }, { "colType", I2S(dbagent::INT) }, { "colValue", L2S(free_rotate_times) } },
        { { "colName", "accumulated_rotate_times" }, { "colType", I2S(dbagent::INT) }, { "colValue", L2S(accumulated_rotate_times) } },
        { { "colName", "historical_rotate_times" }, { "colType", I2S(dbagent::INT) }, { "colValue", L2S(historical_rotate_times) } },
        { { "colName", "cooldown" }, { "colType", I2S(dbagent::STRING) }, { "colValue", newCooldownDeadline } },
        { { "colName", "rotate_queue" }, { "colType", I2S(dbagent::STRING) }, { "colValue", new_rotate_queue_vec } },
        { { "colName", "order_queue" }, { "colType", I2S(dbagent::STRING) }, { "colValue", new_order_queue_str } }
    };

    dataproxy::TWriteDataRsp dataRsp;
    iRet = writeProxyDataLottery(req.uid, dataproxy::E_REDIS_WRITE, dataVec, dataRsp);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "Insert Lottery Entity error!" << "iRet : " << iRet << endl;
        resp.resultCode = -1;
        return -1;
    }

    ROLLLOG_DEBUG << "resp : " << printTars(resp) << endl;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

tars::Int32 ActivityServantImp::queryLotteryDetail(const lottery::QueryLotteryDetailReq &req, lottery::QueryLotteryDetailResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;

    __TRY__

    ROLLLOG_DEBUG << "req : " << printTars(req) << endl;
    dataproxy::TReadDataRsp dataRsq;
    iRet = readProxyDataLottery(req.uid, dataRsq);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "Read Lottery data error!" << endl;
        resp.resultCode = -1;
        return -1;
    }

    bool withoutEntity = false;
    ostringstream os;
    {
        vector<int> allRotateId = g_app.getOuterFactoryPtr()->getAllRotateId();
        size_t len = allRotateId.size();
        for (size_t i = 0; i < len; ++i)
        {
            os << allRotateId[i] << ":" << 0;
            if (i != len - 1)
            {
                os << "|";
            }
        }
    }

    string order_queue = os.str();

    lottery::E_LOTTERY_TYPE firstOpt = g_app.getOuterFactoryPtr()->getFirstOpt();
    int frequency = firstOpt == lottery::E_LOTTERY_FREE ? FREQUENCY_NULL : 0;//TODO 这里是不是可以统一为0
    int rotateId = g_app.getOuterFactoryPtr()->getRotateId(firstOpt, frequency);
    int dropId = g_app.getOuterFactoryPtr()->getDropId(rotateId, 0);
    os.str("");
    os << firstOpt << ":" << dropId << ":" << rotateId;
    string str_rotate_queue = os.str();
    if (dataRsq.fields.empty())
    {
        vector<map<string, string>> dataVec =
        {
            { { "colName", "uid" }, { "colType", I2S(dbagent::BIGINT) }, {"colValue", L2S(req.uid)} },
            { { "colName", "free_rotate_times" }, { "colType", I2S(dbagent::INT)}, { "colValue", L2S(0) } },
            { { "colName", "accumulated_rotate_times" }, { "colType", I2S(dbagent::INT)}, { "colValue", L2S(0) } },
            { { "colName", "historical_rotate_times" }, { "colType", I2S(dbagent::INT)}, { "colValue", L2S(0) } },
            { { "colName", "cooldown" }, { "colType", I2S(dbagent::INT)}, { "colValue", L2S(0) } },
            { { "colName", "rotate_queue" }, { "colType", I2S(dbagent::STRING)}, { "colValue", str_rotate_queue }},
            { { "colName", "order_queue" }, { "colType", I2S(dbagent::STRING) }, { "colValue", order_queue } }
        };

        dataproxy::TWriteDataRsp dataRsp;
        iRet = writeProxyDataLottery(req.uid, dataproxy::E_REDIS_INSERT, dataVec, dataRsp);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "Insert Lottery Entity error!" << "iRet : " << iRet << endl;
            resp.resultCode = -1;
            return -1;
        }

        withoutEntity = true;
    }

    //读记录
    dataproxy::TReadDataRsp readDataRsp;
    if (withoutEntity)
    {
        iRet = readProxyDataLottery(req.uid, readDataRsp);
        if (iRet != 0)
        {
            ROLLLOG_ERROR << "readProxyData error ! uid : " << req.uid << endl;
            resp.resultCode = -1;
            return -2;
        }
    }
    else
    {
        readDataRsp.fields = dataRsq.fields;
    }

    //基本不会出现这种情况,因为uid是unique key
    if (readDataRsp.fields.size() != 1)
    {
        ROLLLOG_ERROR << "unexpected error ! uid : " << req.uid << endl;
        resp.resultCode = -1;
        return -3;
    }

    map<std::string, std::string> data = parseFieldRow(readDataRsp.fields[0]);
    int free_rotate_times = S2I(data["free_rotate_times"]);
    //int accumulated_rotate_times = S2I(data["accumulated_rotate_times"]);
    //int historical_rotate_times = S2I(data["historical_rotate_times"]);
    string cooldownDeadlineraw = data["cooldown"];
    vector<string> cooldownDeadlineVec = split(cooldownDeadlineraw, "|");
    int cooldownDeadline = S2I(cooldownDeadlineVec.front());
    string rotate_queue = data["rotate_queue"];
    vector<string> rotate_queue_vec = split(rotate_queue, "|");
    string str_order_queue = data["order_queue"];

    //通过次序order和rotateId获取掉落组
    map<int, int> map_order_queue = decodeOrderQueue(str_order_queue);
    time_t now = TC_TimeProvider::getInstance()->getNow();

    ROLLLOG_DEBUG << "cooldownDeadlineraw : " << cooldownDeadlineraw <<", cooldownDeadline: "<< cooldownDeadline<<", now: " << now << endl;

    resp.resultCode = 0;
    resp.freeRotateTimes = free_rotate_times;
    resp.cooldown = cooldownDeadline > now ? cooldownDeadline - now : 0;
    resp.superNeedFreeTimes = g_app.getOuterFactoryPtr()->getSuperNeedFreeTimes();
    //rotateType:dropId:rotateId | ...
    RotateOperator nextOpt = parseOperator(rotate_queue_vec.front());
    int curOrder = getCurrentOrderByRotateId(nextOpt.rotateId, map_order_queue);

    auto &info = g_app.getOuterFactoryPtr()->getRotationInfo(nextOpt.rotateId, curOrder);
    resp.icon = info.icon;
    resp.cartoon = info.cartoon;
    resp.rotateType = lottery::E_LOTTERY_TYPE(nextOpt.rotateType);
    /*if (now < cooldownDeadline)
        {
            //处于CD状态
            ROLLLOG_DEBUG << "now : " << now<< ", cooldownDeadline:"<< cooldownDeadline << endl;
            return iRet;
        }*/

    auto &awards = g_app.getOuterFactoryPtr()->getAwardByDropId(nextOpt.dropId);
    for (size_t i = 0; i < awards.size(); ++i)
    {
        lottery::RewardItem item;
        item.goodsID = awards[i].PropId;
        item.count = awards[i].Number;
        item.rewardId = awards[i].Id;
        resp.rewardPool.push_back(item);
    }

    ROLLLOG_DEBUG << "resp : " << printTars(resp) << endl;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

tars::Int32 ActivityServantImp::onLotteryGo(const XGameComm::TPackage &pkg, const LotteryProto::LotteryReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;

    __TRY__

    ROLLLOG_DEBUG << "req : " << req.DebugString() << endl;

    lottery::LotteryReq lotteryReq;
    lotteryReq.uid = pkg.stuid().luid();
    lotteryReq.rotateType = (lottery::E_LOTTERY_TYPE)(req.rotatetype());
    lottery::LotteryResp lotteryResp;
    iRet = lotteryGo(lotteryReq, lotteryResp, current);
    LotteryProto::LotteryResp resp;
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "LotteryGo Request error ! uid : " << pkg.stuid().luid() << ", rotateType : " << req.rotatetype() << endl;
        resp.set_resultcode(-1);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::LOTTERY_GO, resp);
        ROLLLOG_DEBUG << "response to client : " << resp.DebugString() << endl;
        return -1;
    }

    resp.set_resultcode(0);
    resp.set_rewardid(lotteryResp.rewardId);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::LOTTERY_GO, resp);
    ROLLLOG_DEBUG << "response to client : " << resp.DebugString() << endl;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

tars::Int32 ActivityServantImp::onQueryLotteryDetail(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    lottery::QueryLotteryDetailReq queryLotteryDetailReq;
    queryLotteryDetailReq.uid = pkg.stuid().luid();
    lottery::QueryLotteryDetailResp queryLotteryDetailResp;
    iRet = queryLotteryDetail(queryLotteryDetailReq, queryLotteryDetailResp, current);
    LotteryProto::QueryLotteryDetailResp resp;
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "QueryLotteryDetail Request error ! uid : " << pkg.stuid().luid() << endl;
        resp.set_resultcode(-1);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::LOTTERY_QUERY, resp);
        ROLLLOG_DEBUG << "response to client : " << resp.DebugString() << endl;
        return -1;
    }

    resp.set_resultcode(0);
    resp.set_freerotatetimes(queryLotteryDetailResp.freeRotateTimes);
    resp.set_cooldown(queryLotteryDetailResp.cooldown);
    resp.set_superneedfreetimes(queryLotteryDetailResp.superNeedFreeTimes);
    resp.set_rotatetype((LotteryProto::E_LOTTERY_TYPE)queryLotteryDetailResp.rotateType);
    resp.set_icon(queryLotteryDetailResp.icon);
    resp.set_cartoon(queryLotteryDetailResp.cartoon);
    for (auto &item : queryLotteryDetailResp.rewardPool)
    {
        auto ptr = resp.add_rewardpool();
        ptr->set_goodsid(item.goodsID);
        ptr->set_count(item.count);
        ptr->set_rewardid(item.rewardId);
    }

    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::LOTTERY_QUERY, resp);
    ROLLLOG_DEBUG << "response to" << ": " << resp.Utf8DebugString() << endl;

    __CATCH__
    FUNC_EXIT("", iRet);
    return iRet;
}

int ActivityServantImp::onQueryScratchDetail(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    ScratchProto::QueryScratchDetailResp response;
    iRet = ProcessorSingleton::getInstance()->query_scratch_detail(pkg.stuid().luid(), response);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << " query scratch detail err ! uid : " << pkg.stuid().luid() << ", iRet: "<< iRet << endl;
        response.set_ret(iRet);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::SCRATCH_DETAIL_QUERY, XGameComm::MSGTYPE_RESPONSE, response);
        return -1;
    }

    response.set_ret(0);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::SCRATCH_DETAIL_QUERY, XGameComm::MSGTYPE_RESPONSE, response);

    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

int ActivityServantImp::onGetScratchReward(const XGameComm::TPackage &pkg, const ScratchProto::ScratchRewardReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    ScratchProto::ScratchRewardResp response;
    iRet = ProcessorSingleton::getInstance()->reward_scratch(pkg.stuid().luid(), req.props_id(), response);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << " reward scratch err ! uid : " << pkg.stuid().luid() << ", iRet: "<< iRet << endl;
        response.set_ret(iRet);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::SCRATCH_REWARD, XGameComm::MSGTYPE_RESPONSE, response);
        return -1;
    }

    response.set_ret(0);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::SCRATCH_REWARD, XGameComm::MSGTYPE_RESPONSE, response);

    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

int ActivityServantImp::onQueryScratchBoard(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    ScratchProto::QueryScratchBoardResp response;
    iRet = ProcessorSingleton::getInstance()->scratch_board( response);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << " board scratch err ! uid : " << pkg.stuid().luid() << ", iRet: "<< iRet << endl;
        response.set_ret(iRet);
        toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::SCRATCH_BOARD, XGameComm::MSGTYPE_RESPONSE, response);
        return -1;
    }
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::SCRATCH_BOARD, XGameComm::MSGTYPE_RESPONSE, response);
    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

int ActivityServantImp::onGetAchievementInfo(const XGameComm::TPackage &pkg, const ScratchProto::QueryAchievementInfoReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    std::map<int, std::vector<long>> mapResult;
    iRet = ProcessorSingleton::getInstance()->query_achievement_info(req.uid(), mapResult);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << " query achievement err. iRet:  " << iRet << endl;
        return iRet;
    }

    ScratchProto::QueryAchievementInfoResp resp;
    for(auto item : mapResult)
    {
        if(item.second.size() != 6)
        {
            continue;
        }
        auto ptr = resp.add_info();
        ptr->set_bet_id(item.second[0]);
        ptr->set_achievement_num(item.second[1]);
        ptr->set_reward_num(item.second[2]);
        ptr->set_condition(item.second[3]);
        ptr->set_status(item.second[4]);
        ptr->set_unlock(item.second[5] == 1);

        ListBaseServiceConfigResp baseServiceConfig;
        g_app.getOuterFactoryPtr()->getConfigServantPrx()->ListBaseServiceConfig(baseServiceConfig);
        auto it = baseServiceConfig.mapData.find(item.second[0]);
        if(it != baseServiceConfig.mapData.end())
        {
            ptr->set_reward_conf(it->second.number);
        }
    }
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ACHIEVEMENT_DETAIL_QUERY, XGameComm::MSGTYPE_RESPONSE, resp);

    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

int ActivityServantImp::onGetAchievementInfo2(const XGameComm::TPackage &pkg, const ScratchProto::QueryAchievementInfoReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    std::map<int, std::vector<long>> mapResult;
    iRet = ProcessorSingleton::getInstance()->query_achievement_info(req.uid(), mapResult);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << " query achievement err. iRet:  " << iRet << endl;
        return iRet;
    }

    ListBaseServiceConfigResp baseServiceConfig;
    g_app.getOuterFactoryPtr()->getConfigServantPrx()->ListBaseServiceConfig(baseServiceConfig);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << " query achievement err. iRet:  " << iRet << endl;
        return iRet;
    }

    ScratchProto::QueryAchievementInfoResp resp;
    int status = 1;
    for(auto &it : baseServiceConfig.mapData)
    {
        if (it.second.type != 5)
        {
            continue;
        }

        auto item = mapResult.find(it.first);
        if (item == mapResult.end())
        {
            auto ptr = resp.add_info();
            ptr->set_bet_id(it.first);
            continue;
        }

        if(item->second.size() != 6)
        {
            ROLLLOG_ERROR << " query achievement err. item.second.size:  " << item->second.size() << endl;
            continue;
        }
        auto ptr = resp.add_info();
        ptr->set_bet_id(item->second[0]);
        ptr->set_achievement_num(item->second[1]);
        ptr->set_reward_num(item->second[2]);
        ptr->set_condition(item->second[3]);
        ptr->set_status(item->second[4]);
        ptr->set_unlock(item->second[5] == 1);
        ptr->set_reward_conf(it.second.number);

        if (ptr->status() == 1)
            status = 0;
    }
    auto ptr = resp.add_info();
    ptr->set_bet_id(0);
    ptr->set_achievement_num(1);
    ptr->set_condition(1);
    ptr->set_status(status);
    ptr->set_unlock(true);

    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ACHIEVEMENT_DETAIL_QUERY2, XGameComm::MSGTYPE_RESPONSE, resp);

    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

int ActivityServantImp::onAchievementBoard(const XGameComm::TPackage &pkg, const ScratchProto::QueryAchievementBoardReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    std::vector<std::vector<long>> vResult;
    iRet = DBOperatorSingleton::getInstance()->select_achievement_board(vResult, req.bet_id());
    if(iRet != 0)
    {
        ROLLLOG_ERROR << " query achievement err. iRet:  " << iRet << endl;
    }

    ScratchProto::QueryAchievementBoardResp resp;
    resp.set_ret(iRet);
    for(auto item : vResult)
    {
        if(item.size() != 4)
        {
            continue;
        }
        auto ptr = resp.add_info();
        ptr->set_uid(item[0]);
        ptr->set_achievement_num(item[1]);
        ptr->set_reward_num(item[2]);
        ptr->set_win_num(item[3]);

        std::map<string, string> mapUserInfo;
        if(ProcessorSingleton::getInstance()->getUserInfo(item[0], mapUserInfo) == 0)
        {
            ptr->set_snickname(mapUserInfo["nickname"]);
            ptr->set_sheadstr(mapUserInfo["head_str"]);
            ptr->set_iplayergender(S2I(mapUserInfo["gender"]));
        }
    }
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ACHIEVEMENT_BOARD, XGameComm::MSGTYPE_RESPONSE, resp);

    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

int ActivityServantImp::onAchievementStatus(const XGameComm::TPackage &pkg, const ScratchProto::AchievementStatusReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    iRet = ProcessorSingleton::getInstance()->update_achievement_status(pkg.stuid().luid(), req);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << " query achievement err. iRet:  " << iRet << endl;
    }

    ScratchProto::AchievementStatusResp resp;
    resp.set_ret(iRet);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ACHIEVEMENT_STATUS, XGameComm::MSGTYPE_RESPONSE, resp);

    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

int ActivityServantImp::onAchievementUnlock(const XGameComm::TPackage &pkg, const ScratchProto::AchievementUnlockReq &req, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    iRet = ProcessorSingleton::getInstance()->update_achievement_unlock(pkg.stuid().luid(), req);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << " update achievement unlock err. iRet:  " << iRet << endl;
    }

    ScratchProto::AchievementUnlockResp resp;
    resp.set_ret(iRet);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ACHIEVEMENT_UNLOCK, XGameComm::MSGTYPE_RESPONSE, resp);

    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

//上报成就信息
tars::Int32 ActivityServantImp::reportAchievementInfo(tars::Int64 lUin, tars::Int32 iRank, tars::Int64 lRewardNum, tars::Int32 BetID, tars::Int32 iCondition, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    std::vector<long> vResult; // betid, reward_num, champion_num, status
    iRet = ProcessorSingleton::getInstance()->update_achievement_info(lUin, iRank, lRewardNum, BetID, iCondition, vResult);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << " update achievement err. param:  " << lUin << "|"<< iRank << "| " << lRewardNum << "|"<< BetID << "| " << iCondition << endl;
        return iRet;
    }

    ROLLLOG_DEBUG << " reportAchievementInfo vResult size: " << vResult.size() << ", uid: "<< lUin << endl;
    if(vResult.size() == 4 && iRank == 1)
    {
        ROLLLOG_DEBUG << " reportAchievementInfo vResult: " << vResult[0] << ","<< vResult[1] << ","<< vResult[2] << ", "<< vResult[3]<< endl;

        ScratchProto::AchievementInfoNotify notify;
        auto ptr = notify.add_info();
        ptr->set_bet_id(vResult[0]);
        ptr->set_reward_num(vResult[1]);
        ptr->set_achievement_num(vResult[2]);
        ptr->set_status(vResult[3]);
        ptr->set_condition(iCondition);

        ListBaseServiceConfigResp baseServiceConfig;
        g_app.getOuterFactoryPtr()->getConfigServantPrx()->ListBaseServiceConfig(baseServiceConfig);
        auto it = baseServiceConfig.mapData.find(vResult[0]);
        if(it != baseServiceConfig.mapData.end())
        {
            ROLLLOG_DEBUG << " reportAchievementInfo reward: " << it->second.number << endl;

            ptr->set_reward_conf(it->second.number);
            //发放奖励
            if(iCondition > 0 && vResult[2] > 0 && vResult[2] % iCondition == 0)
            {
                //发放奖励
                GoodsManager::GiveGoodsReq  giveGoodsReq;
                giveGoodsReq.uid = lUin;
                giveGoodsReq.goods.goodsID = it->second.propsID;
                giveGoodsReq.goods.count = it->second.number;
                giveGoodsReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SNG_RANK_REWARD_C;
                GoodsManager::GiveGoodsRsp giveGoodsRsp;
                iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(lUin)->giveGoods(giveGoodsReq, giveGoodsRsp);
                if (iRet != 0 || giveGoodsRsp.resultCode != 0)
                {
                    ROLLLOG_ERROR << "giveGoods error!" << "iRet : " << iRet << ", resultCode : " << giveGoodsRsp.resultCode << endl;
                    return 0;
                }
            }
        }
        XGameComm::TPackage pkg;
        pkg.mutable_stuid()->set_luid(lUin);
        std::string address = "";
        ProcessorSingleton::getInstance()->getUserAddress(lUin, address);
        toClientPb(pkg, address, XGameProto::ActionName::ACHIEVEMENT_TOUCH, XGameComm::MSGTYPE_NOTIFY, notify);
    }
    FUNC_EXIT("", iRet);
    return 0;
}

//获取成就信息
tars::Int32 ActivityServantImp::getAchievementInfo(tars::Int64 lUin, map<tars::Int32, tars::Int64>& mapAchievementInfo, tars::TarsCurrentPtr current)
{
    std::map<int, std::vector<long>> mapResult;
    int iRet = ProcessorSingleton::getInstance()->query_achievement_info(lUin, mapResult);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << " query achievement err. iRet:  " << iRet << endl;
        return iRet;
    }

    for(auto item : mapResult)
    {
        if(item.second.size() != 5 || item.second[4] != 1)
        {
            continue;
        }
        mapAchievementInfo.insert(std::make_pair(item.second[0], item.second[1]/item.second[3]));
    }
    return 0;
}

//上报宝箱进度
tars::Int32 ActivityServantImp::reportBoxProgressRate(const Box::BoxProgressRateReq &req, Box::BoxProgressRateResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    iRet = PreciousBoxSingleton::getInstance()->ReportBoxProgressRate(req, resp);

    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

//查询宝箱信息
tars::Int32 ActivityServantImp::queryBox(const Box::QueryBoxReq &req, Box::QueryBoxResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    iRet = ProcessorSingleton::getInstance()->query_box(req.uid, resp);

    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

int ActivityServantImp::onLeagueBoard(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    ScratchProto::QueryLeagueBoardResp resp;
    iRet = DBOperatorSingleton::getInstance()->select_league_board(pkg.stuid().luid(), resp);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << " query league board err. iRet:  " << iRet << endl;
    }

    //其他信息
    long total_league_reward = 0;
    for(auto& item : resp.league_board_info())
    {
        std::map<string, string> mapUserInfo;
        if(ProcessorSingleton::getInstance()->getUserInfo(item.uid(), mapUserInfo) == 0)
        {
            const_cast<ScratchProto::QueryLeagueBoardResp_Item &>(item).set_sheadstr(mapUserInfo["head_str"]);
            const_cast<ScratchProto::QueryLeagueBoardResp_Item &>(item).set_snickname(mapUserInfo["nickname"]);
            const_cast<ScratchProto::QueryLeagueBoardResp_Item &>(item).set_iplayergender(S2I(mapUserInfo["gender"]));
        }
        total_league_reward += item.league_reward_num();
    }

    auto prx = g_app.getOuterFactoryPtr();
    if(prx)
    {
        if(prx->getLeagueConfIntervalTime() > 0)
        {
            resp.set_league_remian_time(prx->getLeagueConfIntervalTime() - ((TNOW - prx->getLeagueConfStartTime()) % prx->getLeagueConfIntervalTime()));
        }

        auto vec_group_num = prx->getLeagueConfGroupNum();
        for(unsigned int i = 0; i < prx->getLeagueConfGroupNum().size(); i++)
        {
            (*resp.mutable_m_group_count())[i + 1] = prx->getLeagueConfGroupNum()[i];
        }
    }
    resp.set_ret(iRet);
    resp.set_total_league_reward(total_league_reward);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::LEAGUE_BOARD, XGameComm::MSGTYPE_RESPONSE, resp);
    
    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

int ActivityServantImp::onGetLeagueRewardInfo(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    ScratchProto::LeagueRewardInfoResp resp;
    iRet = DBOperatorSingleton::getInstance()->select_league_reward_info(pkg.stuid().luid(), resp);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << " get league reward info err. iRet:  " << iRet << endl;
    }

    resp.set_ret(iRet);
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::LEAGUE_REWARD, XGameComm::MSGTYPE_RESPONSE, resp);
    
    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

int ActivityServantImp::onGetLeagueReward(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    ScratchProto::GetLeagueRewardResp resp;
    iRet = ProcessorSingleton::getInstance()->league_reward(pkg.stuid().luid());
    if(iRet < 0)
    {
        ROLLLOG_ERROR << " get league reward info err. iRet:  " << iRet << endl;
    }

    resp.set_ret(iRet < 0 ? iRet : 0);
    resp.set_league_reward_num(iRet >  0 ? iRet : 0) ;
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::LEAGUE_REWARD_REVICE, XGameComm::MSGTYPE_RESPONSE, resp);

    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

tars::Int32 ActivityServantImp::reportLeagueStat(tars::Int64 lUin, tars::Int64 lWinNum, tars::Int64 lPumpNum, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    std::vector<long> vResult; // betid, reward_num, champion_num, status
    iRet = DBOperatorSingleton::getInstance()->update_user_league_info(lUin, lWinNum, lPumpNum);
    if(iRet != 0)
    {
        ROLLLOG_ERROR << " update league err. iRet:  " << iRet  << endl;
        return iRet;
    }

    ROLLLOG_DEBUG << " reportLeagueStat, uid: "<< lUin <<", lWinNum: "<< lWinNum << ", lPumpNum: "<< lPumpNum << endl;
    
    FUNC_EXIT("", iRet);
    return 0;
}

tars::Int32 ActivityServantImp::getLeagueInfo(tars::Int64 lUin, tars::TarsCurrentPtr current)
{
    return DBOperatorSingleton::getInstance()->select_league_level(lUin);
}

tars::Int32 ActivityServantImp::queryLuckeyWheel(tars::Int64 lUin, LuckeyWheel::QueryLuckeyWheelResp &resp, tars::TarsCurrentPtr current)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    GoodsManager::GetAGoodsCountReq goodsCountReq;
    GoodsManager::GetAGoodsCountRsp goodsCountRsp;
    goodsCountReq.uid = lUin;
    goodsCountReq.goodsID = 12001;
    iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(lUin)->getAGoodsCount(goodsCountReq, goodsCountRsp);
    if(iRet != 0)
    {
        LOG_ERROR<<"getAGoodsCount err! uid: "<< lUin << " goodsID:" << goodsCountReq.goodsID << endl;
    }

    resp.lootCount = goodsCountRsp.count;
    resp.resultCode = 0;

    ROLLLOG_DEBUG << "uid: "<< lUin << " resp:" << printTars(resp) << endl;

    __CATCH__
    FUNC_EXIT(iRet, "");
    return 0;
}

int ActivityServantImp::onQueryLuckyWheel(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, const std::string &msg)
{
    FUNC_ENTRY("");
    int iRet = 0;
    long uid = pkg.stuid().luid();
    ActivityProto::QueryLuckyWheelResp resp;

    GoodsManager::GetAGoodsCountReq goodsCountReq;
    GoodsManager::GetAGoodsCountRsp goodsCountRsp;
    goodsCountReq.uid = uid;
    goodsCountReq.goodsID = 12001;
    iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(uid)->getAGoodsCount(goodsCountReq, goodsCountRsp);
    if(iRet != 0)
    {
        ROLLLOG_DEBUG <<"getAGoodsCount err! uid: "<< uid << " goodsID:" << goodsCountReq.goodsID << endl;
    }

    resp.set_propsid(12001);
    resp.set_lootcount(goodsCountRsp.count);
    resp.set_resultcode(0);

    ROLLLOG_DEBUG << "uid: "<< uid << " resp:" << resp.DebugString() << endl;
    
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ACTIVITY_LUCKY_WHEEL_QUERY, XGameComm::MSGTYPE_RESPONSE, resp);
    
    FUNC_EXIT("", iRet);
    return 0;
}

int ActivityServantImp::onLuckyWheelLottery(const XGameComm::TPackage &pkg, const std::string &sCurServrantAddr, const std::string &msg)
{
    FUNC_ENTRY("");
    int iRet = 0;
    long uid = pkg.stuid().luid();
    ActivityProto::LuckyWheelLotteryResp resp;

    config::LuckyWheelCfgResp cfgResp;
    iRet = g_app.getOuterFactoryPtr()->getConfigServantPrx()->getLuckyWheelCfg(cfgResp);
    if (iRet != 0)
    {
        LOG_ERROR << "getLuckyWheelCfg err! uid: " << uid << " iRet:" << iRet << endl;
        resp.set_resultcode(iRet);
        return iRet;
    }

    auto itCfg = cfgResp.data.find(ActivityProto::TREASURE);
    if (itCfg == cfgResp.data.end())
    {
        LOG_ERROR << "lotteryType err! uid: " << uid << endl;
        resp.set_resultcode(-1);
        return iRet;    
    }

    int sumWeight = 0;
    for (auto &it : itCfg->second)
    {
        sumWeight += it.weights;
    }

    int rewardID = 0;
    int raio = timeutil::randomNumber(0, sumWeight);
    for (auto &it : itCfg->second)
    {
        raio -= it.weights;
        if (raio <= 0)
        {
            rewardID = it.id;
            resp.set_rewardid(it.id);
            auto prop = resp.add_rewards();
            prop->set_propsid(it.rewards.propsID);
            prop->set_number(it.rewards.number);
            break;
        }
    }

    GoodsManager::GetAGoodsCountReq goodsCountReq;
    GoodsManager::GetAGoodsCountRsp goodsCountRsp;
    goodsCountReq.uid = uid;
    goodsCountReq.goodsID = 12001;
    iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(uid)->getAGoodsCount(goodsCountReq, goodsCountRsp);
    if(iRet != 0)
    {
        ROLLLOG_DEBUG <<"getAGoodsCount err! uid: "<< uid << " goodsID:" << goodsCountReq.goodsID << endl;
    }

    //夺宝券不足
    if (goodsCountRsp.count <= 0)
    {
        mall::GoodsExchangeReq goodsExchangeReq;
        mall::GoodsExchangeResp goodsExchangeResp;
        goodsExchangeReq.uid = uid;
        goodsExchangeReq.goodsID = 12001;
        iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(uid)->goodsExchange(goodsExchangeReq, goodsExchangeResp);
        if (iRet != 0)
        {
            LOG_ERROR <<"getAGoodsCount err! uid: "<< uid << " goodsID:" << goodsCountReq.goodsID << endl;
            resp.set_resultcode(iRet);
            toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ACTIVITY_LUCKY_WHEEL_LOTTERY, XGameComm::MSGTYPE_RESPONSE, resp);
            return iRet;
        }
    }

    //扣除消耗
    GoodsManager::DeleteGoodsReq delGoodsReq;
    GoodsManager::DeleteGoodsRsp delGoodsResp;
    delGoodsReq.uid = uid;
    delGoodsReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_LUKEY_WHEEL_REWARD;
    delGoodsReq.goods.goodsID = 12001;
    delGoodsReq.goods.count = 1;
    iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(uid)->costGoods(delGoodsReq, delGoodsResp);
    if (iRet != 0 || delGoodsResp.resultCode != 0)
    {
        ROLLLOG_ERROR << " costGoods err. resultCode: " << delGoodsResp.resultCode << endl;
        resp.set_resultcode(-1);
        return iRet;
    }

    //发放奖励
    for (auto &itprops : resp.rewards())
    {
        GoodsManager::GiveGoodsReq giveGoodsReq;
        GoodsManager::GiveGoodsRsp giveGoodsRsp;
        giveGoodsReq.uid = uid;
        giveGoodsReq.goods.goodsID = itprops.propsid();
        giveGoodsReq.goods.count = itprops.number();
        giveGoodsReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_LUKEY_WHEEL_REWARD;

        iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(uid)->giveGoods(giveGoodsReq, giveGoodsRsp);
        if (iRet != 0)
        {
            LOG_ERROR << "reward gold err. iRet: " << iRet << " propID:" << itprops.propsid() << endl;
            resp.set_resultcode(iRet);
            return iRet;
        }
        LuckyWhellLog(uid, itprops.propsid(), itprops.number(), ActivityProto::TREASURE, rewardID);
    }

    ROLLLOG_DEBUG << "uid: "<< uid << " resp:" << resp.DebugString() << endl;
    
    toClientPb(pkg, sCurServrantAddr, XGameProto::ActionName::ACTIVITY_LUCKY_WHEEL_LOTTERY, XGameComm::MSGTYPE_RESPONSE, resp);

    FUNC_EXIT("", iRet);
    return 0;
}

void ActivityServantImp::LuckyWhellLog(long uid, int propsId, long propsNum, int type, int lotteryId)
{
    DaqiGame::TLog2DBReq tLog2DBReq;
    tLog2DBReq.sLogType = 34;

    vector<string> veclog;
    veclog.push_back(L2S(uid));
    veclog.push_back(I2S(propsId));
    veclog.push_back(L2S(propsNum));
    veclog.push_back(L2S(type));
    veclog.push_back(L2S(lotteryId));

    tLog2DBReq.vecLogData.push_back(veclog);
    g_app.getOuterFactoryPtr()->getLog2DBServantPrx(uid)->async_log2db(NULL, tLog2DBReq);
}

int ActivityServantImp::onJackpotGo(const XGameComm::TPackage &pkg, const ActivityProto::JackpotGoReq &req, ActivityProto::JackpotGoResp &resp)
{
    FUNC_ENTRY("");
    int iRet = 0;
    __TRY__

    long uid = pkg.stuid().luid();
    
    config::JackpotCfgResp jackpotCfg;
    iRet = g_app.getOuterFactoryPtr()->getConfigServantPrx()->getJackpotCfg(jackpotCfg);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "getJackpotCfg err, iRet: " << iRet << endl;
        return iRet;
    }

    auto itCfg = jackpotCfg.data.find(req.blindlevel());
    if (itCfg == jackpotCfg.data.end())
    {
        ROLLLOG_ERROR << "jackpotCfg.data.find err: req.blindlevel:" << req.blindlevel() << endl;
        return -1; 
    }

    iRet = -1;
    for (auto &iter : itCfg->second)
    {
        if (req.amount() != iter.amount)
            continue;

        int sumWeight = 0;
        for (auto &it : iter.probility)
        {
            sumWeight += it;
        }

        int raio = timeutil::randomNumber(0, sumWeight);
        for (size_t i = 0; i < iter.probility.size(); ++i)
        {
            raio -= iter.probility[i];
            if (raio <= 0)
            {
                auto amount = iter.miltipes[i] * req.amount();
                resp.set_amount(amount);
                break;
            }
        }
        iRet = 0;
        break;
    }

    ROLLLOG_DEBUG << "req.blindlevel: " << req.blindlevel() << ", req.amount:" << req.amount() << " resp.amount:" << resp.amount() << " iRet:" << iRet << endl;
    if (iRet != 0)
    {
        ROLLLOG_ERROR << " weights err. iRet:  " << iRet  << endl;
        return iRet;        
    }

    userinfo::GetUserBasicReq basicReq;
    basicReq.uid = uid;
    userinfo::GetUserBasicResp basicResp;
    iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(uid)->getUserBasic(basicReq, basicResp);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << " getUserBasic err. iRet:  " << iRet  << endl;
        return iRet;        
    }

    if (basicResp.userinfo.gold < req.amount())
    {
        ROLLLOG_ERROR << " gold not enough err. iRet:  " << iRet  << endl;
        return XGameRetCode::ROOM_GOLD_NOT_ENOUGH; 
    }

    if (resp.amount() == req.amount())
    {
        return 0;
    }

    userinfo::ModifyUserWealthReq  modifyUserWealthReq;
    userinfo::ModifyUserWealthResp  modifyUserWealthResp;
    modifyUserWealthReq.uid = uid;
    modifyUserWealthReq.goldChange = -req.amount();
    modifyUserWealthReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_JACKPOT_COST;
    iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(uid)->modifyUserWealth(modifyUserWealthReq, modifyUserWealthResp);
    if (iRet != 0)
    {
        LOG_ERROR << "cost gold err. iRet: " << iRet << endl;
        return iRet;
    }

    modifyUserWealthReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_JACKPOT_REWARD;
    modifyUserWealthReq.goldChange = resp.amount();
    iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(uid)->modifyUserWealth(modifyUserWealthReq, modifyUserWealthResp);
    if (iRet != 0)
    {
        LOG_ERROR << "reward gold err. iRet: " << iRet << endl;
        return iRet;
    }

    __CATCH__
    FUNC_EXIT(iRet, "");
    return iRet;
}

template<typename T>
int ActivityServantImp::toClientPb(const XGameComm::TPackage &tPackage, const std::string &sCurServrantAddr,
                                   XGameProto::ActionName actionName, XGameComm::MSGTYPE type, const T &t)
{
    XGameComm::TPackage rsp;

    auto mh = rsp.add_vecmsghead();
    mh->set_nmsgid(actionName);
    mh->set_nmsgtype(type);
    mh->set_servicetype(XGameComm::SERVICE_TYPE::SERVICE_TYPE_GOLD_PIG);
    rsp.add_vecmsgdata(pbToString(t));

    auto pPushPrx = Application::getCommunicator()->stringToProxy<JFGame::PushPrx>(sCurServrantAddr);
    if (pPushPrx)
    {
        ROLLLOG_DEBUG << "response : " << t.DebugString() << endl;
        pPushPrx->tars_hash(tPackage.stuid().luid())->async_doPushBuf(NULL, tPackage.stuid().luid(), pbToString(rsp));
    }
    else
    {
        ROLLLOG_ERROR << "pPushPrx is null: " << t.DebugString() << endl;
    }

    return 0;
}

template<typename T>
int ActivityServantImp::toClientPb(const XGameComm::TPackage &tPackage, const std::string &sCurServrantAddr, XGameProto::ActionName actionName, const T &t)
{
    XGameComm::TPackage rsp;

    auto mh = rsp.add_vecmsghead();
    mh->set_nmsgid(actionName);
    mh->set_nmsgtype(XGameComm::MSGTYPE::MSGTYPE_RESPONSE);
    mh->set_servicetype(XGameComm::SERVICE_TYPE::SERVICE_TYPE_LOTTERY);
    rsp.add_vecmsgdata(pbToString(t));

    auto pPushPrx = Application::getCommunicator()->stringToProxy<JFGame::PushPrx>(sCurServrantAddr);
    if (pPushPrx)
        pPushPrx->tars_hash(tPackage.stuid().luid())->async_doPushBuf(NULL, tPackage.stuid().luid(), pbToString(rsp));
    else
        ROLLLOG_ERROR << "pPushPrx is null"  << endl;

    return 0;
}

tars::Int32 ActivityServantImp::doCustomMessage(bool bExpectIdle)
{
    if (bExpectIdle)
    {
        /*LOG_DEBUG << "-----------------ActivityServantImp::doCustomMessage()-------------------------" << endl;*/
        //新赛季
        if((TNOW - g_app.getOuterFactoryPtr()->getLeagueConfStartTime()) % g_app.getOuterFactoryPtr()->getLeagueConfIntervalTime() == 0)
        {
            DBOperatorSingleton::getInstance()->reset_new_league_info();
        }
        if((TNOW - g_app.getOuterFactoryPtr()->getLeagueConfStartTime()) % g_app.getOuterFactoryPtr()->getLeagueConfNewHandIntervalTime() == 0)
        {
            DBOperatorSingleton::getInstance()->check_add_newhand();
        }
        if((TNOW - g_app.getOuterFactoryPtr()->getLeagueConfStartTime()) % g_app.getOuterFactoryPtr()->getLeagueConfChangeScoreIntervalTime() == 0)
        {
            DBOperatorSingleton::getInstance()->change_robot_league_score();
        }
    }

    return 0;
}
