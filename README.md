# Winsock Ping Demo  

### 关于  

  这是一个简单的Ping程序，适用于Windows.  

之所以会有这个，是因为我的计算机网络课程要求写一个课设……用C/C++实现Ping程序。写完之后感觉放着有点浪费，遂上传过来。  

### 如何构建  

  正如代码中的注释提到的，这份程序在Windows 10 x64下使用gcc 9.2.0 (tdm-1)编译通过。编译时使用 `-lws2_32` `-liphplapi` `-ldnsapi`选项指定链接到Winsock2、IPHelpAPI和DNSAPI静态库，整个的编译指令如下：  
  `g++ smping.cpp --std=c++11 -lws2_32 -liphlpapi -ldnsapi -o smping.exe`  
  事就这么成了（

### 使用说明  

  ~注释里其实也有说，复制粘贴过来：~  
  > 使用方法：smping \[-t\] \[-l size\] \[-n count\] ~\[-4\] \[-6\]~ \[--debug\] target_name  
  > -t 一直ping指定的主机 直至用户强制停止（Ctrl + C）  
  > -l size 指定要发送的ICMP报文大小  
  > -n count 指定要发送的次数  
  > ~-4 强制使用IPv4~  
  > ~-6 强制使用IPv6~  
  > --debug 输出用于debug的额外信息  

  ~*IPv6还没有实现，等我什么时候摸鱼摸够了想起来吧*~  
  *因为校园网内没有测试IPv6的条件 估计是要咕了*  

### 进度  

 * 2021/03/31 新建文件 smping.cpp  
 * 2021/04/** 摸鱼 摸鱼 摸鱼  
 * 2021/04/18 完成基本的Ping功能，但不能解析参数  
 * 2021/04/19 仔细研究了CheckSum() 现在总算算出来是对的了  
 * 2021/04/20 重写了checkArgs() 现在可以解析 -t -l -n 参数了；另外还加上了很多（我不喜欢的）大括号，提高可读性  
 * 实现了NsLookup() 本来想自己发送DNS包查询的，但是卡在第一步 获取DNS服务器上  
 * 大概是我用的gcc链接Windows自带库有问题 又不想把DNS服务器写死在代码里，遂用Winsock提供的api偷懒了
 * 2021/04/21 增加了NsLookupFull() 如果NsLookup()失败（确实会失败，在解析局域网内时）就自己发送DNS请求查询
 * 2021/04/** 摸鱼，然后捡到bug  
 * 2021/04/25 DNS请求确实是发出去了，但解析回应太麻烦了而且情况有很多种 决定使用DNSAPI（libdnsapi.a，或者VS里的dnsapi.lib）来节省脑力  
 * 2021/04/26 完成NsLookupFull()，但在`windns.h`中的函数声明似乎和`libdnsapi.a`中的实现有些许不同，导致Lint一直报错。以gcc提示的具体实现为准了。  
 * 2021/04/27 完善了统计信息部分，把一些变量和函数名统一规范了。可能之后不会再来动这个项目了。  
