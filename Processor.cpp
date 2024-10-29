#include "Processor.h"
#include "TimeUtil.h"
//

/**
 *
*/
Processor::Processor()
{
}

/**
 *
*/
Processor::~Processor()
{

}

int Processor::readDataFromDB(long uid, const string& table_name, const std::vector<string>& col_name, const map<string, string>& whlist, const string& order_col, int limit_num,  dbagent::TDBReadRsp &dataRsp)
{
    int iRet = 0;
    dbagent::TDBReadReq rDataReq;
    rDataReq.keyIndex = 0;
    rDataReq.queryType = dbagent::E_SELECT;
    rDataReq.tableName = table_name;

    vector<dbagent::TField> fields;
    dbagent::TField tfield;
    tfield.colArithType = E_NONE;
    for(auto item : col_name)
    {
        tfield.colName = item;
        fields.push_back(tfield);
    }
    rDataReq.fields = fields;

    //where条件组
    if(!whlist.empty())
    {
        vector<dbagent::ConditionGroup> conditionGroups;
        dbagent::ConditionGroup conditionGroup;
        conditionGroup.relation = dbagent::AND;
        vector<dbagent::Condition> conditions;
        for(auto item : whlist)
        {
            dbagent::Condition condition;
            condition.condtion = dbagent::E_EQ;
            condition.colType = dbagent::STRING;
            condition.colName = item.first;
            condition.colValues = item.second;
            conditions.push_back(condition);
        }
        conditionGroup.condition = conditions;
        conditionGroups.push_back(conditionGroup);
        rDataReq.conditions = conditionGroups;
    }
   
    //order by字段
    if(!order_col.empty())
    {
        vector<dbagent::OrderBy> orderBys;
        dbagent::OrderBy orderBy;
        orderBy.sort = dbagent::DESC;
        orderBy.colName = order_col;
        orderBys.push_back(orderBy);
        rDataReq.orderbyCol = orderBys;
    }

    if(limit_num > 0)
    {
        //指定返回的行数的最大值
        rDataReq.limit = limit_num;
        //指定返回的第一行的偏移量
        rDataReq.limit_from = 0;
    }

    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(uid)->read(rDataReq, dataRsp);
    if (iRet != 0 || dataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "read data from dbagent failed, rDataReq:" << printTars(rDataReq) << ",dataRsp: " << printTars(dataRsp) << endl;
        return -1;
    }
    return 0;
}

int Processor::writeDataFromDB(dbagent::Eum_Query_Type dBOPType, long uid, const string& table_name, const std::map<string, string>& col_info, const map<string, string>& whlist)
{
    int iRet = 0;
    //更新上个赛季状态
    dbagent::TDBWriteReq uDataReq;
    uDataReq.keyIndex = 0;
    uDataReq.queryType = dBOPType;
    uDataReq.tableName = table_name;

    vector<dbagent::TField> fields;
    dbagent::TField tfield;

    for(auto item : col_info)
    {
        tfield.colArithType = E_NONE;
        tfield.colType = dbagent::STRING;
        tfield.colName = item.first;
        tfield.colValue = item.second;
        fields.push_back(tfield);
    }
    uDataReq.fields = fields;

    //where条件组
    vector<dbagent::ConditionGroup> conditionGroups;
    dbagent::ConditionGroup conditionGroup;
    conditionGroup.relation = dbagent::AND;
    vector<dbagent::Condition> conditions;
    dbagent::Condition condition;
    for(auto item : whlist)
    {
        dbagent::Condition condition;
        condition.condtion = dbagent::E_EQ;
        condition.colType = dbagent::STRING;
        condition.colName = item.first;
        condition.colValues = item.second;
        conditions.push_back(condition);
    }
    conditionGroup.condition = conditions;
    conditionGroups.push_back(conditionGroup);
    uDataReq.conditions = conditionGroups;

    dbagent::TDBWriteRsp uDataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(uid)->write(uDataReq, uDataRsp);
    if (iRet != 0 || uDataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "update data to dbagent failed, uDataReq:" << printTars(uDataReq) << ",uDataRsp: " << printTars(uDataRsp) << endl;
        return -1;
    }
    return 0;
}


int Processor::getUserAddress(tars::Int64 uid, std::string &address)
{
    FUNC_ENTRY("");
    int iRet = 0;

    dataproxy::TReadDataReq dataReq;
    dataReq.resetDefautlt();
    dataReq.keyName = I2S(E_REDIS_TYPE_HASH) + ":" + I2S(USER_STATE_ONLINE) + ":" + L2S(uid);
    dataReq.operateType = E_REDIS_READ;
    dataReq.clusterInfo.resetDefautlt();
    dataReq.clusterInfo.busiType = E_REDIS_PROPERTY;
    dataReq.clusterInfo.frageFactorType = E_FRAGE_FACTOR_USER_ID;
    dataReq.clusterInfo.frageFactor = uid;
    dataReq.paraExt.resetDefautlt();
    dataReq.paraExt.queryType = E_SELECT;

    vector<TField> fields;
    TField tfield;
    tfield.colArithType = E_NONE;
    tfield.colName = "gwaddr";
    tfield.colType = STRING;
    fields.push_back(tfield);
    tfield.colName = "gwcid";
    tfield.colType = STRING;
    fields.push_back(tfield);
    dataReq.fields = fields;

    TReadDataRsp dataRsp;
    iRet = g_app.getOuterFactoryPtr()->getDBAgentServantPrx(uid)->redisRead(dataReq, dataRsp);
    ROLLLOG_DEBUG << "getUserAddress, iRet: " << iRet << ", dataRsp: " << printTars(dataRsp) << endl;
    if (iRet != 0 || dataRsp.iResult != 0)
    {
        ROLLLOG_ERROR << "getUserAddress err, iRet: " << iRet << ", iResult: " << dataRsp.iResult << endl;
        return -1;
    }

    string gwAddr = "";
    for (auto it = dataRsp.fields.begin(); it != dataRsp.fields.end(); ++it)
    {
        for (auto itaddr = it->begin(); itaddr != it->end(); ++itaddr)
        {
            if (itaddr->colName == "gwaddr")
            {
                gwAddr = itaddr->colValue;
            }
        }
    }

    address = gwAddr;

    FUNC_EXIT("", iRet);
    return iRet;
}

//查询
int Processor::getUserInfo(long uid, std::map<string, string>& mapUserInfo)
{
    userinfo::GetUserBasicReq basicReq;
    basicReq.uid = uid;
    userinfo::GetUserBasicResp basicResp;
    int iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(uid)->getUserBasic(basicReq, basicResp);
    if (iRet != 0)
    {
        ROLLLOG_ERROR << "getUserBasic failed, uid: " << uid << endl;
        return -1;
    }

    mapUserInfo["head_str"] = basicResp.userinfo.head;
    mapUserInfo["nickname"] = basicResp.userinfo.name;
    mapUserInfo["gender"] = basicResp.userinfo.gender;
    mapUserInfo["level"] = basicResp.userinfo.level;

    return 0;
}


int Processor::query_scratch_detail(long uid, ScratchProto::QueryScratchDetailResp &resp)
{
    int iRet = 0;

    string table_name = "tb_scratch";
    std::vector<string> col_name = {"props_id", "scratch_num", "scratch_info", "received", "total_reward_num", "play_ad_time"};
    std::map<string, string> whlist = {
            {"uid", L2S(uid)},
        };
    dbagent::TDBReadRsp dataRsp;
    iRet = readDataFromDB(uid, table_name, col_name, whlist, "", 0, dataRsp);
    if(iRet != 0)
    {
        LOG_ERROR<<"query_scratch_detail err! uid: "<< uid << endl;
        return iRet;
    }
 
    map<int, ScratchProto::ScratchDetail > m_datail;
    for (auto it = dataRsp.records.begin(); it != dataRsp.records.end(); ++it)
    {
        std::vector<std::vector<int>> scratch_info;
        ScratchProto::ScratchDetail detail;
        long total_reward_num = 0;
        
        for (auto itfield = it->begin(); itfield != it->end(); ++itfield)
        {
            if (itfield->colName == "props_id")
            {
                detail.set_props_id(S2I(itfield->colValue));
            }
            if (itfield->colName == "scratch_num")
            {
                detail.set_scratch_num(S2I(itfield->colValue));
            }
            if (itfield->colName == "received")
            {
                detail.set_received(S2I(itfield->colValue));
            }
            if (itfield->colName == "play_ad_time")
            {
                detail.set_play_ad_time(S2I(itfield->colValue));
            }
            if (itfield->colName == "total_reward_num")
            {
                total_reward_num = S2I(itfield->colValue);
            }
            if (itfield->colName == "scratch_info")
            {
                if(!itfield->colValue.empty())
                {
                    g_app.getOuterFactoryPtr()->splitStringFromat(itfield->colValue, scratch_info);
                    prase_scratch_info(scratch_info, detail); 
                }
            }
        }

        LOG_DEBUG<<"uid: "<< uid<< ", props_id: "<< detail.props_id() << ", scratch_num: "<< detail.scratch_num() << ", received: "<< detail.received() << endl;
        if(detail.scratch_num() > 0 && (detail.reward_info_size() == 0 || detail.received() > 0))
        {
            scratch_info.clear();
            auto str_scratch_info = create_scratch_info(uid, detail, total_reward_num);
            if(str_scratch_info.empty())
            {
                LOG_ERROR<<"create_scratch_info err. uid: "<< uid<< ", props_id: "<< detail.props_id() << endl;
                iRet = -2;
            }
            g_app.getOuterFactoryPtr()->splitStringFromat(str_scratch_info, scratch_info);
            prase_scratch_info(scratch_info, detail);
        }
        m_datail.insert(std::make_pair(detail.props_id(), detail));
    }

    //初始化 返回结构体
    config::ScratchConfigResp cfg;
    g_app.getOuterFactoryPtr()->getConfigServantPrx()->getScratchCfg(cfg);
    auto init_scratch_list = g_app.getOuterFactoryPtr()->getScratchList();
    for(auto props_id : init_scratch_list)
    {
        auto itcfg = cfg.data.find(S2I(props_id));
        if(itcfg == cfg.data.end())
        {
            LOG_ERROR<<" scratch cfg err. props_id: "<< props_id << endl;
            continue;
        }

        long max_reward_num = 0;
        long repeat_num = itcfg->second.repeat_num;
        int ad_cd_time = itcfg->second.ad_cd_time;
        std::vector<std::vector<int>> vec_scratch_info;
        std::map<int, std::vector<int>> map_scratch_info;
        g_app.getOuterFactoryPtr()->splitStringFromat(itcfg->second.reward_param, vec_scratch_info);
        for(auto item : vec_scratch_info)
        {
            if(item.size() == 6)
            {
                map_scratch_info.insert(std::make_pair(item[0], item));
            }  
        }
        
        if(map_scratch_info.size() > 2)
        {
            int max_mult = 1;
            auto scratch_mult_list = g_app.getOuterFactoryPtr()->getScratchMultList();
            if(scratch_mult_list.size() > 0)
            {
                max_mult = S2I(*scratch_mult_list.rbegin());
            }

            int select_count = 0;
            for(auto iter = map_scratch_info.rbegin(); iter != map_scratch_info.rend(); iter++)
            {
                if(select_count >= 2) break;
                max_reward_num += iter->first;
                select_count++;
            }
            max_reward_num *= max_mult;
        }

        auto it = m_datail.find(S2I(props_id));
        if(it != m_datail.end())
        {
            it->second.set_repeat_num(repeat_num);
            it->second.set_ad_cd_time(ad_cd_time);
            it->second.set_max_reward_num(max_reward_num); 
            (*resp.mutable_detail_info())[it->first] = it->second;
        }
        else
        {
            ScratchProto::ScratchDetail detail;
            detail.set_props_id(S2I(props_id));
            detail.set_repeat_num(repeat_num);
            detail.set_ad_cd_time(ad_cd_time);
            detail.set_max_reward_num(max_reward_num); 
            (*resp.mutable_detail_info())[S2I(props_id)] = detail;
        }
    }
    LOG_DEBUG<<"init_scratch_list size: "<< init_scratch_list.size() << endl;
    return iRet;
}

string Processor::create_scratch_info(long uid, ScratchProto::ScratchDetail &detail, long total_reward_num)
{

    LOG_DEBUG<<"create_scratch_info uid: "<< uid << ", props_id: "<< detail.props_id() << endl;
    int iRet = 0;
    config::ScratchConfigResp cfg;
    g_app.getOuterFactoryPtr()->getConfigServantPrx()->getScratchCfg(cfg);
    auto it = cfg.data.find(detail.props_id());
    if(it == cfg.data.end())
    {
        LOG_ERROR <<" scratch cfg err." << endl;
        return "";
    }
    std::vector<std::vector<int>> vec_scratch_info;
    std::map<int, std::vector<int>> map_scratch_info;
    std::vector<int> vec_keynum;
    long accumulate_weight = 0; 
    g_app.getOuterFactoryPtr()->splitStringFromat(it->second.reward_param, vec_scratch_info);
    LOG_ERROR <<" reward_param: "<< it->second.reward_param << ", vec_scratch_info size: "<< vec_scratch_info.size() << endl;
    for(auto item : vec_scratch_info)
    {
        if(item.size() != 6)
        {
            continue;
        }
        accumulate_weight += item[1] * 1000;
        vec_keynum.push_back(item[0]);
        map_scratch_info.insert(std::make_pair(item[0], item));
    }

    if(accumulate_weight == 0 || vec_keynum.size() == 0 || map_scratch_info.size() == 0)
    {
        LOG_ERROR <<" scratch param err. props_id: "<< detail.props_id() << endl;
        return "";
    }

    //随机中奖因子, 生成中奖号码
    long reward_factor = rand() % accumulate_weight;
    long current_weight = 0; 
    long reward_number = 0;
    for(auto item : map_scratch_info)
    {   
        current_weight += item.second[1] * 1000;
        if(current_weight >= reward_factor )
        {
            reward_number = item.first;
            break;
        }
    }
    if(reward_number <= 0 )
    {
        LOG_ERROR<<"create reward_number err." << endl;
        return "";
    }

    //构造 刮刮卡本体
    int number = it->second.limit_num;
    std::map<int, std::vector<int>> map_result;// key_num:[count, type]
    for(int i = 0; i < it->second.repeat_num; i++)// step1: 中奖数字
    {
        auto it = map_result.find(reward_number);
        if(it == map_result.end())
        {
            std::vector<int> vt = {1, 0};
            map_result.insert(std::make_pair(reward_number, vt));
        }
        else
        {
            it->second[0] += 1;  
        }
    }
    number -= it->second.repeat_num;
        
    //step2: 倍数
    int mult_index = -1;
    auto itt = map_scratch_info.find(reward_number);
    if(itt != map_scratch_info.end())
    {
        long mult_weight = itt->second[2] + itt->second[3] + itt->second[4] + itt->second[5];
        if(mult_weight > 0)
        {
            long mult_factor = rand() % 100;
            LOG_DEBUG<<"mult_factor: "<< mult_factor << endl;

            long current_factor = 0;
            for(unsigned int j = 0; j < itt->second.size(); j++)
            {
                if(j <= 1 || itt->second[j] <= 0)
                {
                    continue;
                }
                current_factor += itt->second[j];
                if(current_factor >= mult_factor)
                {
                    mult_index = j - 2;
                    break;
                }
            }
        }
        LOG_DEBUG<<"mult_weight: "<< mult_weight << ", mult_index: "<< mult_index << endl;
    }
    if(mult_index > -1)
    {
        auto scratch_mult_list = g_app.getOuterFactoryPtr()->getScratchMultList();
        std::vector<int> vt = {1, 1};
        map_result.insert(std::make_pair(S2I(scratch_mult_list[mult_index]), vt));
        number--;
    }

    for(int i = 0; i < number; i++)//step3: 剩余数字
    {
        std::random_shuffle(vec_keynum.begin(), vec_keynum.end());
        for(auto key : vec_keynum)
        {
            auto subit = map_result.find(key);
            if(key == reward_number || (subit != map_result.end() && subit->second[0] >= it->second.repeat_num -1 ))
            {
                continue;
            }
            if(subit == map_result.end())
            {
                std::vector<int> vt = {1, 0};
                map_result.insert(std::make_pair(key, vt));
            }
            else
            {
                subit->second[0] += 1;  
            }
            break;
        }
    }
    
    
    std::vector<string> vec_result;
    for(auto iter = map_result.begin(); iter != map_result.end(); iter++)
    {
        if(iter->second.size() != 2)
        {
            continue;
        }
        for(int i = 0; i< iter->second[0]; i++)
        {
            vec_result.push_back(I2S(iter->first) + ":" + I2S(iter->second[1]));
        }
    }
    
    string result = g_app.getOuterFactoryPtr()->joinStringFromat(vec_result);
    LOG_DEBUG<<"scratch info: "<< result << ", uid: "<< uid << ", props_id: "<< detail.props_id() << endl;

    //更新数据库
    string table_name = "tb_scratch";
    std::map<string, string> col_info = {
            {"scratch_info", result},
            {"received", "0"},
    };
    std::map<string, string> whlist = {
            {"uid", L2S(uid)},
            {"props_id", L2S(detail.props_id())},
    };
    iRet = writeDataFromDB(dbagent::E_UPDATE, uid, table_name, col_info, whlist);
    if(iRet != 0)
    {
        LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
        return "";
    }
    return result;
}

void Processor::prase_scratch_info(std::vector<std::vector<int>> &scratch_info, ScratchProto::ScratchDetail &detail)
{
    detail.clear_reward_info();
    for(auto item : scratch_info)
    {
        if(item.size() != 2)
        {
            continue;
        }
        auto info = detail.add_reward_info();
        info->set_key_num(item[0]);
        info->set_type(item[1]);
    }
}


int Processor::reward_scratch(long uid, int props_id, ScratchProto::ScratchRewardResp &resp)
{
    LOG_DEBUG<<"reward_scratch uid: "<< uid << ", props_id: "<< props_id << endl;

    int iRet = 0;
    string table_name = "tb_scratch";
    std::vector<string> col_name = {"scratch_num", "scratch_info", "total_reward_num"};
    std::map<string, string> whlist = {
            {"uid", L2S(uid)},
            {"props_id", L2S(props_id)},
            {"received", "0"},
        };
    dbagent::TDBReadRsp dataRsp;
    iRet = readDataFromDB(uid, table_name, col_name, whlist, "", 0, dataRsp);
    if(iRet != 0)
    {
        LOG_ERROR<<"reward_scratch err! uid: "<< uid << endl;
        return iRet;
    }
 
    map<int, ScratchProto::ScratchDetail > m_datail;
    for (auto it = dataRsp.records.begin(); it != dataRsp.records.end(); ++it)
    {
        std::vector<std::vector<int>> vec_scratch_info;
        long total_reward_num = 0;
        int scratch_num = 0;
        
        for (auto itfield = it->begin(); itfield != it->end(); ++itfield)
        {
            if (itfield->colName == "total_reward_num")
            {
                total_reward_num = S2L(itfield->colValue);
            }
            if (itfield->colName == "scratch_num")
            {
                scratch_num = S2L(itfield->colValue);
            }
            if (itfield->colName == "scratch_info")
            {
                if(!itfield->colValue.empty())
                {
                    g_app.getOuterFactoryPtr()->splitStringFromat(itfield->colValue, vec_scratch_info);
                }
            }
        }

        std::map<int, std::vector<int>> map_scratch_info;
        for(auto item : vec_scratch_info)
        {
            if(item.size() != 2)
            {
                continue;
            }
            auto it = map_scratch_info.find(item[0]);
            if(it == map_scratch_info.end())
            {
                std::vector<int> vt = {1, item[1]};
                map_scratch_info.insert(std::make_pair(item[0], vt));
            }
            else
            {
                it->second[0] += 1;  
            }
        }

        long base_reward_num = 0;
        int mult = 1;
        for(auto item : map_scratch_info )
        {
            if(item.second[0] == 3)
            {
                base_reward_num += item.first;
            }
            if(item.second[1] == 1)
            {
                mult = item.first;
            }
        }
        long current_reward_num = base_reward_num * mult;//本次奖励数
        LOG_DEBUG<<"base_reward_num: "<< base_reward_num << ", mult: "<< mult << ", current_reward_num: "<< current_reward_num << endl;

        //更新数据库
        string table_name = "tb_scratch";
        std::map<string, string> col_info = {
                {"scratch_num", L2S(scratch_num -1)},
                {"received", "1"},
        };
        std::map<string, string> whlist = {
                {"uid", L2S(uid)},
                {"props_id", L2S(props_id)},
        };
        iRet = writeDataFromDB(dbagent::E_UPDATE, uid, table_name, col_info, whlist);
        if(iRet != 0)
        {
            LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
            return iRet;
        }

        col_info.clear();
        whlist.clear();
        col_info = {{"total_reward_num", L2S(total_reward_num + current_reward_num)},};
        whlist = {{"uid", L2S(uid)},};
        iRet = writeDataFromDB(dbagent::E_UPDATE, uid, table_name, col_info, whlist);
        if(iRet != 0)
        {
            LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
            return iRet;
        }

        resp.set_props_id(props_id);
        resp.set_reward_num(current_reward_num);
        resp.set_scratch_num(scratch_num);

        //发送金币
        GoodsManager::GiveGoodsReq giveGoodsReq;
        GoodsManager::GiveGoodsRsp giveGoodsRsp;
        giveGoodsReq.uid = uid;
        giveGoodsReq.goods.goodsID = 20000;
        giveGoodsReq.goods.count = abs(current_reward_num);
        giveGoodsReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_SCRATCH_REWARD;
        iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(uid)->giveGoods(giveGoodsReq, giveGoodsRsp);
        if(iRet != 0)
        {
            LOG_ERROR<<"reward gold err. iRet: "<< iRet << endl;
            return iRet;
        }
    }
    return 0;
}

int Processor::scratch_board(ScratchProto::QueryScratchBoardResp &resp)
{
    int iRet = 0;
    string table_name = "tb_scratch";
    std::vector<string> col_name = {"uid", "total_reward_num"};
    std::map<string, string> whlist = {
            {"1", "1"},
        };
    dbagent::TDBReadRsp dataRsp;
    iRet = readDataFromDB(0, table_name, col_name, whlist, "total_reward_num", 20, dataRsp);
    if(iRet != 0)
    {
        LOG_ERROR<<"scratch_board err!"<< endl;
        return iRet;
    }
 
    std::vector<pair<long, long>> vec_reward_info;
    map<long, long> m_reward_info;//  uid: reward_num
    for (auto it = dataRsp.records.begin(); it != dataRsp.records.end(); ++it)
    {
        long uid = 0; 
        long total_reward_num = 0;
        for (auto itfield = it->begin(); itfield != it->end(); ++itfield)
        {
            if (itfield->colName == "uid")
            {
                uid = S2L(itfield->colValue);
            }
            if (itfield->colName == "total_reward_num")
            {
                total_reward_num = S2L(itfield->colValue);
            }
        }
        auto subit = std::find_if(vec_reward_info.begin(), vec_reward_info.end(), [uid](const pair<long, long> &item)->bool{
            return uid == item.first;
        });
        if(subit == vec_reward_info.end())
        {
            vec_reward_info.push_back( pair<long, long>(uid, total_reward_num));
        }
    }
    LOG_DEBUG<<"records size: "<< dataRsp.records.size() << ", vec_reward_info size: "<< vec_reward_info.size() << endl;

    //排序，取前三名
    std::sort(vec_reward_info.begin(), vec_reward_info.end(),[](const pair<long, long> &l, const pair<long, long> &r)->bool{
        return l.second > r.second;
    });
    for(unsigned int i = 1; i <= vec_reward_info.size(); i++)
    {
        if(i > 3)
        {
            break;
        }
        ScratchProto::QueryScratchBoardResp_Item item;
        item.set_uid(vec_reward_info[i - 1].first);
        item.set_reward_num(vec_reward_info[i - 1].second);

        std::map<string, string> mapUserInfo;
        if(getUserInfo(item.uid(), mapUserInfo) == 0)
        {
            item.set_head(mapUserInfo["head_str"]);
        }
        (*resp.mutable_board_info())[i] = item;
    }
    return iRet;
}

int Processor::query_achievement_info(long lUin, std::map<int, std::vector<long>>& mapResult)
{
    int iRet = 0;
    string table_name = "tb_achievement";
    std::vector<string> col_name = {"bet_id", "champion_num", "reward_num", "ach_condition", "status", "is_unlock"};
    std::map<string, string> whlist = {
            {"uid", L2S(lUin)},
    };
    dbagent::TDBReadRsp dataRsp;
    iRet = readDataFromDB(0, table_name, col_name, whlist, "", 0, dataRsp);
    if(iRet != 0)
    {
        LOG_ERROR<<"readDataFromDB err!"<< endl;
        return iRet;
    }
    for (auto it = dataRsp.records.begin(); it != dataRsp.records.end(); ++it)
    {
        std::vector<long> ItemResult = {0, 0, 0, 0, 0, 0};
        for (auto itfield = it->begin(); itfield != it->end(); ++itfield)
        {
            if (itfield->colName == "bet_id")
            {
                ItemResult[0] += S2L(itfield->colValue);
            }
            else if (itfield->colName == "champion_num")
            {
                ItemResult[1] += S2L(itfield->colValue);
            }
            else if (itfield->colName == "reward_num")
            {
                ItemResult[2] += S2L(itfield->colValue);
            }
            else if (itfield->colName == "ach_condition")
            {
                ItemResult[3] += S2L(itfield->colValue);
            }
            else if (itfield->colName == "status")
            {
                ItemResult[4] += S2L(itfield->colValue);
            }
            else if (itfield->colName == "is_unlock")
            {
                ItemResult[5] += S2L(itfield->colValue);
            }
        }
        mapResult[ItemResult[0]] = ItemResult;
    }
    return 0;
}

int Processor::update_achievement_status(long lUin, const ScratchProto::AchievementStatusReq &req)
{
    int iRet = 0;
    if(req.status_size() > 1)
    {
        LOG_ERROR<<"update_achievement_status err. req: "<< logPb(req) << endl;
        return -1;
    }
    string table_name = "tb_achievement";
    std::map<string, string> whlist = {
            {"uid", L2S(lUin)},
    };
    std::map<string, string> col_info = {{"status", "0"},};
    iRet = writeDataFromDB(dbagent::E_UPDATE, lUin, table_name, col_info, whlist);
    if(iRet != 0)
    {
        LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
        return iRet;
    }

    LOG_DEBUG << "req:" << req.DebugString() << endl;

    for(auto item : req.status())
    {
        if(item.second)
        {
            whlist.clear();
            col_info.clear();
            std::map<string, string> whlist = {
                {"uid", L2S(lUin)},
                {"bet_id", L2S(item.first)},
            };
            col_info = {{"status", "1"},};

            iRet = writeDataFromDB(dbagent::E_UPDATE, lUin, table_name, col_info, whlist);
            if(iRet != 0)
            {
                LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
                return iRet;
            }
        }
    }
    return 0;
}

int Processor::update_achievement_unlock(long lUin, const ScratchProto::AchievementUnlockReq &req)
{
    int iRet = 0;
    string table_name = "tb_achievement";
    std::map<string, string> whlist = {
            {"uid", L2S(lUin)},
            {"bet_id", L2S(req.bet_id())}
    };
    std::map<string, string> col_info = {{"is_unlock", "1"},};
    iRet = writeDataFromDB(dbagent::E_UPDATE, lUin, table_name, col_info, whlist);
    if(iRet != 0)
    {
        LOG_ERROR << "write into db err. iRet: "<< iRet << ", req.bet_id:" << req.bet_id() << endl;
        return iRet;
    }

    return 0;
}

int Processor::update_achievement_info(long lUin, int iRank, long lRewardNum, int BetID, int iCondition, std::vector<long>& vResult)
{
    int iRet = 0;
    //更新数据库
    string table_name = "tb_achievement";
    std::vector<string> col_name = {"champion_num", "reward_num", "game_num", "status"};
    std::map<string, string> whlist = {
            {"uid", L2S(lUin)},
            {"bet_id", L2S(BetID)},
    };
    dbagent::TDBReadRsp dataRsp;
    iRet = readDataFromDB(0, table_name, col_name, whlist, "", 0, dataRsp);
    if(iRet != 0)
    {
        LOG_ERROR<<"scratch_board err!"<< endl;
        return iRet;
    }
    if(dataRsp.records.size() == 0)//无记录, 直接插入
    {
        std::map<string, string> col_info = {{"uid", L2S(lUin)}, {"bet_id", L2S(BetID)}, 
                    {"champion_num", L2S(iRank == 1 ? 1 : 0)}, {"game_num", "1"},
                    {"reward_num", L2S(lRewardNum)}, {"ach_condition", L2S(iCondition)},
        };
        iRet = writeDataFromDB(dbagent::E_INSERT, lUin, table_name, col_info, whlist);
        if(iRet != 0)
        {
            LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
        }
        vResult = {BetID, lRewardNum, 1, 0};
        return iRet;
    }

    int champion_num = iRank == 1 ? 1 : 0;
    long reward_num = lRewardNum;
    int status = 0;
    long game_num = 1;
    for (auto it = dataRsp.records.begin(); it != dataRsp.records.end(); ++it)
    {
        for (auto itfield = it->begin(); itfield != it->end(); ++itfield)
        {
            if (itfield->colName == "champion_num")
            {
                champion_num += S2L(itfield->colValue);
            }
            if (itfield->colName == "reward_num")
            {
                reward_num += S2L(itfield->colValue);
            }
            if (itfield->colName == "status")
            {
                status = S2L(itfield->colValue);
            }
            if (itfield->colName == "game_num")
            {
                game_num += S2L(itfield->colValue);
            }
        }
    }
    std::map<string, string> col_info = { {"champion_num", L2S(champion_num)}, {"reward_num", L2S(reward_num)}, {"game_num", L2S(game_num)}};
    iRet = writeDataFromDB(dbagent::E_UPDATE, lUin, table_name, col_info, whlist);
    if(iRet != 0)
    {
        LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
        return iRet;
    }
    vResult = {BetID, reward_num, champion_num, status};
    return 0;
}

int Processor::query_box(long uid, Box::QueryBoxResp &resp)
{
    WriteLocker lock(m_rwlock);
    int iRet = 0;
    string table_name = "tb_precious_box";
    std::vector<string> col_name = {"betId", "progressRate", "number", "todayNumber", "lastTime"};
    std::map<string, string> whlist = {
        {"uid", L2S(uid)},
    };

    dbagent::TDBReadRsp dataRsp;
    iRet = ProcessorSingleton::getInstance()->readDataFromDB(uid, table_name, col_name, whlist, "", 0, dataRsp);
    if(iRet != 0)
    {
        LOG_ERROR<<"query_box err! uid: "<< uid << endl;
        return iRet;
    }

    time_t lastTime = 0;
    for (auto it = dataRsp.records.begin(); it != dataRsp.records.end(); ++it)
    {
        int betId = 0;
        int number = 0;
        int todayNumber = 0;
        int progressRate = 0;
        for (auto itfield = it->begin(); itfield != it->end(); ++itfield)
        {
            if (itfield->colName == "betId")
            {
                betId = S2L(itfield->colValue);
            }
            else if (itfield->colName == "progressRate")
            {
                progressRate = S2L(itfield->colValue);
            }
            else if (itfield->colName == "number")
            {
                number = S2L(itfield->colValue);
            }
            else if (itfield->colName == "todayNumber")
            {
                todayNumber = S2L(itfield->colValue);
            }
            else if (itfield->colName == "lastTime")
            {
                lastTime = S2L(itfield->colValue);
            }
        }
        resp.mapBox[betId].number = number;
        resp.mapBox[betId].todayNumber = todayNumber;
        resp.mapBox[betId].progressRate = progressRate;
        resp.totalNumber += number;
        resp.totalTodayNumber += todayNumber;
    }
    
    if (!timeutil::isSameDay(TNOW, lastTime))
    {
        LOG_DEBUG << "lastTime:" << timeutil::DatetimeToString(lastTime) << endl;
        reset_box_today_number(uid);
        resp.totalTodayNumber = 0;
        for (auto &it : resp.mapBox)
            it.second.todayNumber = 0;
    }

    return iRet;
}

int Processor::query_box_by_betid(long uid, int betid, Box::QueryBoxResp &resp)
{
    int iRet = 0;
    string table_name = "tb_precious_box";
    std::vector<string> col_name = {"progressRate", "number", "todayNumber", "lastTime"};
    std::map<string, string> whlist = {
        {"uid", L2S(uid)},
        {"betId", L2S(betid)}
    };

    dbagent::TDBReadRsp dataRsp;
    iRet = ProcessorSingleton::getInstance()->readDataFromDB(uid, table_name, col_name, whlist, "", 0, dataRsp);
    if(iRet != 0)
    {
        LOG_ERROR<<"query_box_by_betid err! uid: "<< uid << endl;
        return iRet;
    }

    time_t lastTime = 0;
    for (auto it = dataRsp.records.begin(); it != dataRsp.records.end(); ++it)
    {
        auto &boxInfo = resp.mapBox[betid];
        for (auto itfield = it->begin(); itfield != it->end(); ++itfield)
        {
            if (itfield->colName == "progressRate")
            {
                boxInfo.progressRate = S2L(itfield->colValue);
            }
            if (itfield->colName == "number")
            {
                boxInfo.number = S2L(itfield->colValue);
            }
            if (itfield->colName == "todayNumber")
            {
                boxInfo.todayNumber = S2L(itfield->colValue);
            }
            if (itfield->colName == "lastTime")
            {
                lastTime = S2L(itfield->colValue);
            }
        }
        resp.totalNumber += boxInfo.number;
        resp.totalTodayNumber += boxInfo.todayNumber;
    }

    if (!timeutil::isSameDay(lastTime, TNOW))
    {
        reset_box_today_number(uid);
        resp.totalTodayNumber = 0;
    }

    return iRet;
}

int Processor::update_box_progress_rate(long uid, int betid, int progressRate, int num, int todayNum)
{
    int iRet = 0;
    string table_name = "tb_precious_box";
    std::map<string, string> whlist = {
        {"uid", L2S(uid)},
        {"betId", L2S(betid)}
    };

    std::map<string, string> col_info = { 
        {"uid", L2S(uid)},
        {"betId", L2S(betid)},
        {"progressRate", L2S(progressRate)}, 
        {"number", L2S(num)}, 
        {"todayNumber", L2S(todayNum)},
        {"lastTime", L2S(TNOW)}
    };
    iRet = writeDataFromDB(dbagent::E_REPLACE, uid, table_name, col_info, whlist);
    if(iRet != 0)
    {
        LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
        return iRet;
    }

    return iRet;
}

int Processor::update_box_number(long uid, int betid, int num)
{
    int iRet = 0;
    string table_name = "tb_precious_box";
    std::map<string, string> whlist = {
        {"uid", L2S(uid)},
        {"betId", L2S(betid)}
    };

    std::map<string, string> col_info = { 
        {"number", L2S(num)},
    };
    iRet = writeDataFromDB(dbagent::E_UPDATE, uid, table_name, col_info, whlist);
    if(iRet != 0)
    {
        LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
        return iRet;
    }

    return iRet;
}

int Processor::reset_box_today_number(long uid)
{
    int iRet = 0;
    string table_name = "tb_precious_box";
    std::map<string, string> whlist = {
        {"uid", L2S(uid)},
    };

    std::map<string, string> col_info = { 
        {"todayNumber", "0"},
        {"lastTime", L2S(TNOW)}
    };
    iRet = writeDataFromDB(dbagent::E_UPDATE, uid, table_name, col_info, whlist);
    if(iRet != 0)
    {
        LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
        return iRet;
    }

    return iRet;
}

int Processor::update_user_league_info(long lUin, long lWinNum, long lPumpNum)
{
    WriteLocker lock(m_rwlock);

    int iRet = 0;
  /*  std::map<string, string> mapUserInfo;
    if(getUserInfo(lUin, mapUserInfo) == 0)
    {
        if(S2I(mapUserInfo["level"]) < 3)
        {
            return 0;
        }
    }

    //更新数据库
    string table_name = "tb_league";
    std::vector<string> col_name = {"win_num", "pump_num"};
    std::map<string, string> whlist = {
            {"uid", L2S(lUin)},
    };
    dbagent::TDBReadRsp dataRsp;
    iRet = readDataFromDB(0, table_name, col_name, whlist, "", 0, dataRsp);
    if(iRet != 0)
    {
        LOG_ERROR<<"update_user_league_info err!"<< endl;
        return iRet;
    }
    if(dataRsp.records.size() == 0)//无记录, 直接插入
    {
        int league_id = ((TNOW - g_app.getOuterFactoryPtr()->getLeagueConfStartTime()) / g_app.getOuterFactoryPtr()->getLeagueConfIntervalTime()) + 1;
        DBOperatorSingleton::getInstance()->insert_newhand(lUin, league_id, lWinNum, lPumpNum);
        return iRet;
    }

    for (auto it = dataRsp.records.begin(); it != dataRsp.records.end(); ++it)
    {
        for (auto itfield = it->begin(); itfield != it->end(); ++itfield)
        {
            if (itfield->colName == "win_num")
            {
                lWinNum += S2L(itfield->colValue);
            }
            if (itfield->colName == "pump_num")
            {
                lPumpNum += S2L(itfield->colValue);
            }
        }
    }

    std::map<string, string> col_info = { {"win_num", L2S(lWinNum)}, {"pump_num", L2S(lPumpNum)}};
    iRet = writeDataFromDB(dbagent::E_UPDATE, lUin, table_name, col_info, whlist);
    if(iRet != 0)
    {
        LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
        return iRet;
    }
*/
    return iRet;
}


int Processor::league_reward(long uid)
{
    int iRet = 0;
    long reward_num = 0;
    string table_name = "tb_league";
    std::vector<string> col_name = {"reward_num"};
    std::map<string, string> whlist = {
            {"uid", L2S(uid)},
            {"status", "1"},
        };
    dbagent::TDBReadRsp dataRsp;
    iRet = readDataFromDB(uid, table_name, col_name, whlist, "", 0, dataRsp);
    if(iRet != 0)
    {
        LOG_ERROR<<"league_reward err! uid: "<< uid << endl;
        return iRet;
    }

    for (auto it = dataRsp.records.begin(); it != dataRsp.records.end(); ++it)
    {
        for (auto itfield = it->begin(); itfield != it->end(); ++itfield)
        {
            if (itfield->colName == "reward_num")
            {
                reward_num = S2L(itfield->colValue);
            }
        }
        //发送金币
        GoodsManager::GiveGoodsReq giveGoodsReq;
        GoodsManager::GiveGoodsRsp giveGoodsRsp;
        giveGoodsReq.uid = uid;
        giveGoodsReq.goods.goodsID = 20000;
        giveGoodsReq.goods.count = abs(reward_num);
        giveGoodsReq.changeType = XGameProto::GOLDFLOW::GOLDFLOW_ID_LEAGUE_REWARD;
        iRet = g_app.getOuterFactoryPtr()->getHallServantPrx(uid)->giveGoods(giveGoodsReq, giveGoodsRsp);
        if(iRet != 0)
        {
            LOG_ERROR<<"reward gold err. iRet: "<< iRet << endl;
            return iRet;
        }

        std::map<string, string> col_info = { {"reward_num", "0"}, {"status", "2"}};
        iRet = writeDataFromDB(dbagent::E_UPDATE, uid, table_name, col_info, whlist);
        if(iRet != 0)
        {
            LOG_ERROR << "write into db err. iRet: "<< iRet << endl;
            return iRet;
        }
    }
    return reward_num;
}
