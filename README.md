# **Linux平台开源拨号APP-mobile**

[![Build Status](https://travis-ci.org/alibaba/fastjson.svg?branch=master)](https://travis-ci.org/alibaba/fastjson)
[![Codecov](https://codecov.io/gh/alibaba/fastjson/branch/master/graph/badge.svg)](https://codecov.io/gh/alibaba/fastjson/branch/master)
[![Maven Central](https://maven-badges.herokuapp.com/maven-central/com.alibaba/fastjson/badge.svg)](https://maven-badges.herokuapp.com/maven-central/com.alibaba/fastjson/)
[![GitHub release](https://img.shields.io/github/release/alibaba/fastjson.svg)](https://github.com/alibaba/fastjson/releases)
[![License](https://img.shields.io/badge/license-Apache%202-4EB1BA.svg)](https://www.apache.org/licenses/LICENSE-2.0.html)

Mobile for raspberry, support LongSung U8300C,9300C, Huawei ME909s, NL660, SIMCOM, N720 and etc

### __主要特性包括:__
 * 提供`USB，DIAL，NET，SMS`等多个基础组件化模块，可以快速动态适配新模块
 * 提供3G，4G等`VPDN`的企业客户的拨号入网，典型场景：公安天网，电网的企业客户
 * 提供`定时重启，定时拨号，进程拨号监控，欠费检测机制`等增值特性
 * 提供`短信控制上下线，短信获取设备信息`等设置和获取参数特性
 * 提供`3G，4G接入云物联网平台`功能，可远程配置，远程拍照，远程视频等 （待开发）
 * 提供`3G，4G的移动热点SOFTAP`功能，在热点范围内共享流量，可类似局域网接入（待开发）

![libmsf](doc/5G.jpeg "libmsf")

## __快速开始__
### 安装环境
```xml
安装Ubuntu, Debian等Linux, ARM环境
测试环境（Linux KaliCI 4.19.0-kali3-amd64）: gcc version 8.2.0 (Debian 8.2.0-14)
测试环境（Linux raspberrypi 4.14.52-v7+）：gcc version 6.4.1 20171012 (Linaro GCC 6.4-2017.11)
```
### 下载开源库

- [微服务框架库][1]
``` groovy
git clone https://github.com/wypx/libmsf/
```
- [微服务通信框库][2]
``` groovy
git clone https://github.com/wypx/librpc/
```
- [拨号Mobile库][3]
``` groovy
git clone https://github.com/wypx/mobile/
```
[1]: https://github.com/wypx/libmsf/
[2]: https://github.com/wypx/librpc/
[3]: https://github.com/wypx/mobile/

Please see this [Wiki Download Page][Wiki] for more repository infos.

[Wiki]: https://github.com/wypx/mobile

### 编译开源库
```xml
root@KaliCI:/media/psf/tomato/mod/mobile make
Make Subdirs msf
make[1]: Entering directory '/media/psf/tomato/mod/libmsf/msf
arm-linux-gnueabihf-gcc lib/src/msf_log.o
.................
make[1]: Leaving directory '/media/psf/tomato/mod/libmsf/msf_daemon

root@KaliCI:/media/psf/tomato/mod/librpc/server make
arm-linux-gnueabihf-gcc bin/src/conn.o
arm-linux-gnueabihf-gcc bin/src/config.o
.......

root@KaliCI:/media/psf/tomato/mod/mobile# make
arm-linux-gnueabihf-gcc bin/src/at_channel.o
arm-linux-gnueabihf-gcc bin/src/at_tok.o
arm-linux-gnueabihf-gcc bin/src/at_mod.o
arm-linux-gnueabihf-gcc bin/src/at_sms.o
arm-linux-gnueabihf-gcc bin/src/at_usb.o
arm-linux-gnueabihf-gcc bin/src/at_dial.o
arm-linux-gnueabihf-gcc bin/src/mobile.o
arm-linux-gnueabihf-gcc /media/psf/tomato/packet/binary/msf_mobile
arm-linux-gnueabihf-strip /media/psf/tomato/packet/binary/msf_mobile
======================
```

```xml
msf_agent  各个服务进程之间的通信代理服务端程序
msf_dlna   测试程序 - 独立微服务进程客户端DLNA
msf_upnp   测试程序 - 独立微服务进程客户端UPNP
msf_daemon 用于守护监控代理进程msf_agent
msf_shell  壳子程序，用于记载配置文件中的插件
msf_mobile 独立编译的mobile拨号程序

libipc.so 提供给各个微服务进程连接的通信代理客户端库
libipc.so 提供给各个微服务进程的基础设施库
          包括：网络 管道 Epoll等事件驱动 日志 共享内存 内存池 
          串口通信 线程 进程 CPU工具 文件 加密 微服务框架 定时器
```

### 运行开源库
```xml
1. 执行样例程序
   $ ./msf_mobile
2. 查看运行日志
   运行结果
```
<img src="http://luotang.me/wp-content/uploads/2018/02/raspberry_mobile-1024x768.jpg" width="600" height="450" />
<img src="http://luotang.me/wp-content/uploads/2018/02/mobile_app_1.png)" width="600" height="450" />
<img src="http://luotang.me/wp-content/uploads/2018/02/mobile_app_2.png)" width="600" height="450" />
<img src="http://luotang.me/wp-content/uploads/2018/02/mobile_app_3.png)" width="600" height="450" />

### 硬件平台适配
``` groovy
根据开发者目标平台以的不同，需要进行相应的适配
```

### ___参考文章___
- [树莓派内核编译支持4G](http://luotang.me/raspberry_mobile.html)
- [Mobile源码解析](http://luotang.me/raspberry_mobile.html)

### *License*

Libmsf is released under the [Gnu 2.0 license](license.txt).
```
/**************************************************************************
*
* Copyright (c) 2017-2019, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
```

