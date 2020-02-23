/*
                        .::::.                                  
                        .::::::::.                                
                    :::::::::::                                
                    ..:::::::::::'                                
                '::::::::::::'                                  
                .::::::::::                                    
            '::::::::::::::..                                   
                ..::::::::::::.                                
                ``::::::::::::::::                               
                ::::``:::::::::'        .:::.                   
                ::::'   ':::::'       .::::::::.                 
            .::::'      ::::     .:::::::'::::.                
            .:::'       :::::  .:::::::::' ':::::.              
            .::'        :::::.:::::::::'      ':::::.            
            .::'         ::::::::::::::'         ``::::.          
        ...:::           ::::::::::::'              ``::.         
    ```` ':.          ':::::::::'                  ::::..      
                        '.:::::'                    ':'````.. 
 */
#include <base/Logger.h>
// #include <base/File.h>
#include "AtChannel.h"
#include "Mobile.h"

#include <vector>
#include <memory>
#include <iostream>
#include <string>
#include <cassert>

using namespace MSF::BASE;
using namespace MSF::EVENT;
using namespace MSF::MOBILE;
using namespace MSF::AGENT;

namespace MSF {
namespace MOBILE {

Mobile::Mobile(const std::string & config)
    : config_(config),
    os_(OsInfo())
{
    os_.enableCoreDump();
    debugInfo();

    logFile_ = "/var/log/luotang.me/Mobile.log";
    assert(Logger::getLogger().init(logFile_.c_str()));

    stack_ = new EventStack();
    if (stack_ == nullptr) {
        MSF_FATAL << "Fail to alloc event stack for mobile.";
        return;
    }
    std::vector<struct ThreadArg> threadArgs;
    threadArgs.push_back(std::move(ThreadArg("ReadLoop")));
    threadArgs.push_back(std::move(ThreadArg("SendLoop")));
    threadArgs.push_back(std::move(ThreadArg("DialLoop")));
    threadArgs.push_back(std::move(ThreadArg("StatLoop")));
    assert(stack_->startThreads(threadArgs));

    agent_ = new AgentClient(stack_->getOneLoop(), "Mobile", APP_MOBILE);
    assert(agent_);
    agent_->setReqCb(std::bind(&Mobile::onRequestCb, this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3));
    pool_ = new MemPool();
    assert(pool_->init());

    channel_ = new ATChannel(nullptr, nullptr, nullptr);
    assert(channel_);

    acm_ = new ATCmdManager();
    assert(acm_);
    channel_->registSendCmdCb(acm_);
}

Mobile::~Mobile()
{
}

void Mobile::debugInfo()
{
    MSF_DEBUG << "#####################################################";
    MSF_DEBUG << "# Name    : Mobile Process                          #";
    MSF_DEBUG << "# Author  : luotang.me                              #";
    MSF_DEBUG << "# Version : 1.1                                     #";
    MSF_DEBUG << "# Time    : 2018-01-27 - 2020-01-21                 #";
    MSF_DEBUG << "# Content : Hardware - SIM - Dial - SMS             #";
    MSF_DEBUG << "#####################################################";
}

bool Mobile::loadConfig()
{
    if (config_.empty()) {
        config_ = "/home/luotang.me/conf/Mobile.conf";
        MSF_INFO << "Use default config: " << config_;
    }
    // if (!File::isFileExist(config_))  {
    //     MSF_INFO << "Config path not exist: " << config_;
    //     return false;
    // }

    return true;
}

void Mobile::onRequestCb(char *data, uint32_t *len, const AgentCommand cmd)
{
    MSF_INFO << "Cmd: " << cmd << " len: " << len;
    if (cmd == AGENT_READ_MOBILE_PARAM) {
        MSF_INFO << "Read mobile param ====> ";

        struct ApnItem item;
        item.cid = 1;
        item.active = 2,
        MSF_INFO << "ApnItem size: " << sizeof(struct ApnItem) << " len: " << len;
        *len = (uint32_t)sizeof(struct ApnItem);
        memcpy(data, &item ,sizeof(struct ApnItem));
    }
}

void Mobile::start()
{
    // stack_->setThreadArgs(threadArgs);
    stack_->start();
}

}
}

int main(int argc, char **argv) 
{
    Mobile mob = Mobile();

    if (!mob.loadConfig()) {
        return -1;
    };


    // mob.setAgent("/var/tmp/mobile.sock");
    mob.setAgent("127.0.0.1", 8888);
    mob.start();
    return 0;
}


