#ifndef _ActivityServer_H_
#define _ActivityServer_H_

#include <iostream>
#include "servant/Application.h"
#include "OuterFactoryImp.h"
#include "DBOperator.h"

using namespace tars;

/**
 *
 **/
class ActivityServer : public Application
{
public:
    /**
     *
     */
    ActivityServer() {};

    /**
     *
     **/
    virtual ~ActivityServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();

    bool reloadSvrConfig(const string &command, const string &params, string &result);

public:
    // 初始化外部对象接口
    int initOuterFactory();

    void initDBOperator();

    // 取外部对象接口
    OuterFactoryImpPtr getOuterFactoryPtr()
    {
        return _pOuter;
    }

private:
    OuterFactoryImpPtr _pOuter;
};

extern ActivityServer g_app;

////////////////////////////////////////////
#endif
