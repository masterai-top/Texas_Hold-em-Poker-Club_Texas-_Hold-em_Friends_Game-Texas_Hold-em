#pragma once
#include "globe.h"
#include "LogComm.h"
#include "ServiceDefine.h"
#include "util/tc_base64.h"
#include <util/tc_singleton.h>
#include "ActivityServant.h"
#include "activity.pb.h"

class PreciousBox
{
public:
    PreciousBox();
    ~PreciousBox();

    int QueryBox(ActivityProto::QueryBoxReq &req, ActivityProto::QueryBoxResp &resp);
    int ReportBoxProgressRate(const Box::BoxProgressRateReq &req, Box::BoxProgressRateResp &resp);
    int GetBoxReward(const ActivityProto::BoxRewardReq &req, ActivityProto::BoxRewardResp &resp);
    void PushGetBox(long uid, int betId, int num);
};

//singleton
typedef TC_Singleton<PreciousBox, CreateStatic, DefaultLifetime> PreciousBoxSingleton;
