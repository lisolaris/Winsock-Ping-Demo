# Winsock Ping Demo  

### 关于  

  这是一个简单的Ping程序，适用于Windows.  

之所以会有这个，是因为我的计算机网络课程要求写一个课设……用C/C++实现Ping程序。写完之后感觉放着有点浪费，遂上传过来。  

### 如何构建  

  正如代码中的注释提到的，这份程序在Windows 10 x64下使用gcc 9.2.0编译通过。编译时使用 `-lws2_32` `-liphplapi` 选项指定链接到Winsock2和IPHelpAPI静态库，整个的编译指令如下：  
  `g++ smping.cpp --std=c++11 -lws2_32 -liphlpapi -o smping.exe`  
  事就这么成了（

### 使用说明  

  注释里其实也有说，复制粘贴过来：  
  > 使用方法：smping \[-t\] \[-l size\] \[-n count\] \[-4\] \[-6\] \[--debug\] target_name  
  > -t 一直ping指定的主机 直至用户强制停止（Ctrl + C）  
  > -l size 指定要发送的ICMP报文大小  
  > -n count 指定要发送的次数  
  > -4 强制使用IPv4  
  > -6 强制使用IPv6  
  > --debug 输出用于debug的额外信息  

  *IPv6还没有实现，等我什么时候摸鱼摸够了想起来吧*  

### 进度  

* 2021/03/31 新建文件 smping.cpp
* 2021/04/** 摸鱼 摸鱼 摸鱼
* 2021/04/18 完成基本的ping功能，但不能解析参数
* 2021/04/19 仔细研究了checkSum() 现在总算算出来是对的了
* 2021/04/20 重写了checkArgs() 现在可以解析 -t -l -n 参数了；另外还加上了很多（我不喜欢的）大括号，提高可读性实现了nslookup() 本来想自己发送DNS包查询的，但是卡在第一步 获取DNS服务器上 大概是我用的gcc链接Windows自带库有问题 又不想把DNS服务器写死在代码里，遂用Winsock提供的api偷懒了
* 2021/04/21 解决了上面那个问题（参考微软给的示例） 增加了nslookupFull()：如果nslookup()失败（确实会失败，在解析局域网内域名时）就自己发送DNS请求到系统默认的DNS服务器查询 *未完成*

