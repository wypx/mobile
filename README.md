# **Linux平台开源拨号应用程序-mobile**

[![Build Status](https://travis-ci.org/alibaba/fastjson.svg?branch=master)](https://travis-ci.org/alibaba/fastjson)
[![Codecov](https://codecov.io/gh/alibaba/fastjson/branch/master/graph/badge.svg)](https://codecov.io/gh/alibaba/fastjson/branch/master)
[![Maven Central](https://maven-badges.herokuapp.com/maven-central/com.alibaba/fastjson/badge.svg)](https://maven-badges.herokuapp.com/maven-central/com.alibaba/fastjson/)
[![GitHub release](https://img.shields.io/github/release/alibaba/fastjson.svg)](https://github.com/alibaba/fastjson/releases)
[![License](https://img.shields.io/badge/license-Apache%202-4EB1BA.svg)](https://www.apache.org/licenses/LICENSE-2.0.html)

### __主要特性包括:__
 * 提供`USB，DIAL，NET，SMS`等多个基础组件化模块，可以快速动态适配新模块
 * 支持多种模块: 龙尚, 华为, 中兴，诺控, 芯讯通, 方正, 移远等厂商模块
 * 提供3G，4G等`VPDN`的企业客户的拨号入网，典型场景：公安天网，电网的企业客户
 * 提供`定时重启，定时拨号，进程监控，欠费检测, 弱网流控`等增值特性
 * 提供`短信控制上下线，短信获取设备信息`等设置和获取参数特性
 * 提供`3G，4G接入云物联网平台`功能，可远程配置，远程拍照，远程视频等 （待开发）
 * 提供`3G，4G的移动热点SOFTAP`功能，在热点范围内共享流量，可类似局域网接入（待开发）

![libmsf](https://github.com/wypx/mobile/blob/master/doc/5G.jpeg "libmsf")

## __快速开始__

### 环境准备
### 安装环境
```sh

Linux发行版本 : 支持Debian, Ubuntu，Centos等发行版本
Linux架构版本 : 目前仅测试支持armv7, aarch64架构的嵌入式设备

测试Linux设备 : Linux raspberrypi 4.14.52-v7+
测试GCC版本   : gcc version 6.4.1 20171012 (Linaro GCC 6.4-2017.11)

测试Linux设备 : Linux jetson 4.9.140-tegra aarch64 aarch64 aarch64 GNU/Linux 
测试GCC版本   : gcc version 7.4.0 (Ubuntu/Linaro 7.4.0-1ubuntu1~18.04.1) 

```
### 下载开源库

- [微服务框架库][1]
``` groovy
git clone https://github.com/wypx/libmsf/
```

- [拨号Mobile库][2]
``` groovy
下载到app目录的mobile子目录
git clone https://github.com/wypx/mobile/
```

[1]: https://github.com/wypx/libmsf/
[2]: https://github.com/wypx/mobile/

Please see this [Wiki Download Page][Wiki] for more repository infos.

[Wiki]: https://github.com/wypx/mobile

### 编译开源库

```sh
root@a3efcdb0894d:/home/share/tomato/mod/libmsf# python build.py    


          Welcome to join me http://luotang.me                 
          Welcome to join me https://github.com/wypx/libmsf    

******************* Micro Service Framework Build Starting **************************

-- Dir 'build/lib' has already exist now.
-- Dir 'build/app' has already exist now.
-- INFO: Numa library is found.
-- Current project compile in debug mode.
-- Configuring done
-- Generating done
-- Build files have been written to: /home/share/tomato/mod/libmsf/build/lib
[  3%] Built target msf
[ 10%] Built target msf_encrypt
[ 18%] Built target msf_agent
[ 36%] Built target msf_event
[ 43%] Built target msf_sock
[100%] Built target msf_base
-- Configuring done
-- Generating done
-- Build files have been written to: /home/share/tomato/mod/libmsf/build/app

******************* Micro Service Framework Build Ending ****************************
```

```sh
root@a3efcdb0894d:/home/share/tomato/mod/libmsf/build/app# ls -l
total 28
-rw-r--r--  1 root root 13581 Jan 27 12:39 CMakeCache.txt
drwxr-xr-x 12 root root   384 Jan 29 03:29 CMakeFiles
-rw-r--r--  1 root root  5528 Jan 29 03:29 Makefile
-rw-r--r--  1 root root  2234 Jan 27 12:39 cmake_install.cmake
drwxr-xr-x  6 root root   192 Jan 29 03:29 mobile                 
drwxr-xr-x  6 root root   192 Jan 29 03:29 msf_agent_server       各个服务进程之间的通信代理服务端程序
drwxr-xr-x  6 root root   192 Jan 29 03:29 msf_guard              用于守护监控代理进程msf_guard
drwxr-xr-x  6 root root   192 Jan 29 03:29 msf_shell              壳子程序，用于记载配置文件中的插件
root@a3efcdb0894d:/home/share/tomato/mod/libmsf/build/app# ls mobile/ -l
total 7952
drwxr-xr-x 5 root root     160 Jan 29 03:29 CMakeFiles
-rw-r--r-- 1 root root   12734 Jan 27 12:39 Makefile
-rwxr-xr-x 1 root root 7159504 Jan 29 03:29 Mobile                独立编译的mobile拨号程序
-rw-r--r-- 1 root root    1130 Jan 27 12:39 cmake_install.cmake
root@a3efcdb0894d:/home/share/tomato/mod/libmsf/build/app#
```

### 运行开源库
```xml
1. 执行样例程序
   $ ./Mobile
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
* Copyright (c) 2017-2021, luotang.me <wypx520@gmail.com>, China.
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

