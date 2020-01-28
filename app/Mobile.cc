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
    stack_ = new EventStack();
    if (stack_ == nullptr) {
        MSF_FATAL << "Fail to alloc event stack for mobile.";
        return;
    }
    agent_ = new AgentClient();
    agent_->init(stack_->getBaseLoop());
    pool_ = new MemPool();
    pool_->init();

    char *buff = static_cast<char*>(pool_->alloc(500));
    memset(buff, 0, 512);

    memcpy(buff, "hhhhhhhhhhkkkgkjfjfhhfhffffh", sizeof("hhhhhhhhhhkkkgkjfjfhhfhffffh"));
    MSF_INFO << "Mp : " << buff;
    pool_->free(buff);

    channel_ = new ATChannel(nullptr, nullptr, nullptr);
    acm_ = new ATCmdManager();
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
    config_ = "/home/share/tomato/mod/libmsf/app/mobile/conf/Mobile.conf";
    if (config_.empty()) {
        config_ = "/home/luotang.me/mobile/Mobile.conf";
        MSF_INFO << "Use default config: " << config_;
    }
    // if (!File::isFileExist(config_))  {
    //     MSF_INFO << "Config path not exist: " << config_;
    //     return false;
    // }

    return true;
}

void Mobile::start(const std::vector<struct ThreadArg> & threadArgs)
{
    stack_->setThreadArgs(threadArgs);
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
    std::vector<struct ThreadArg> threadArgs;
    threadArgs.push_back(std::move(ThreadArg("ReadLoop")));
    threadArgs.push_back(std::move(ThreadArg("SendLoop")));
    threadArgs.push_back(std::move(ThreadArg("DialLoop")));
    threadArgs.push_back(std::move(ThreadArg("StatLoop")));

    // mob.setAgent("/var/tmp/mobile.sock");
    mob.setAgent("127.0.0.1", 8888);
    mob.start(threadArgs);
    return 0;
}


