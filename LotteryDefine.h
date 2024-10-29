#ifndef __LOTTERYDEFINE_H__
#define __LOTTERYDEFINE_H__

#include <string>

/**
 *
*/
typedef struct _Award
{
    int dropId;
    int Id;
    int Type;
    int PropId;
    int Number;
    int probability;
} Award;

/**
 *
*/
typedef struct _RotationInfo
{
    int rotateId;	// 旋转组ID
    //int order;	// 旋转顺序
    std::string icon;
    std::string cartoon;//动画
    std::string name;
    int time; //间隔--命名与Excel表一致
    int accumulate;//这个好像默认都是1,现在或许可以不管  TODO
    int dropId;
} RotationInfo;

/**
 *
*/
typedef struct _Synopsis
{
    int type;//旋转分类 --成员名和配置文件一致
    int frequency;//累计次数 --TODO 这个好像用string更方便,因为普通旋转配置为NULL,NULL用数字怎么样去表示呢
    int rotateId;
} Synopsis;

#endif
