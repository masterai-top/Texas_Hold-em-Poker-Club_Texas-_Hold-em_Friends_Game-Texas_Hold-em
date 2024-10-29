#ifndef __ACTIVITY_LOGCOMM_H__
#define __ACTIVITY_LOGCOMM_H__

//
#include <util/tc_logger.h>
#include "servant/RemoteLogger.h"

//
using namespace tars;

//
#define ROLLLOG(level) (LOG->level() << "[" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "] ")
#define ROLLLOG_DEBUG (ROLLLOG(debug))
#define ROLLLOG_INFO (ROLLLOG(info))
#define ROLLLOG_WARN (ROLLLOG(warn))
#define ROLLLOG_ERROR (ROLLLOG(error))

#define FUNC_ENTRY(in) (ROLLLOG(debug) << ">>>> Enter " << __FUNCTION__ << "() in(" << in << ")" << endl)
#define FUNC_EXIT(out, ret) (ROLLLOG(debug) << "<<<< Exit " << __FUNCTION__ << "() out[" << out << "], ret = " << ret << endl)

#define FDLOG_ERROR (FDLOG("error") << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "|")
#define FDLOG_EXCEPT (FDLOG("except") << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "|")

//配置信息
#define FDLOG_CONFIG_INFO (FDLOG("config_info") << "|")

//游戏配置数据信息
#define FDLOG_GAME_CONFIG_INFO (FDLOG("game_config_info") << "|")

//奖池变化日志
#define JACKPOT_CHANGE_LOG_TOPIC "jackpot_change_log"
#define FDLOG_JACKPOT_CHANGE_LOG (FDLOG("jackpot_change_log") << "|")

//
#define FDLOG_INIT_FORMAT(x,y,z) (TarsTimeLogger::getInstance()->initFormatWithType<LogByMinute>(x,y,z))
#define FDLOG_JACKPOT_CHANGE_LOG_FORMAT (FDLOG_INIT_FORMAT("jackpot_change_log", "%Y%m%d%H%M", 5))

//
#define TERMINATE_APPLICATION() {sleep(10);terminate();}

//
#define COST_MS 100

#endif

