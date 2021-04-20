# Winsock Ping Demo  

### 关于  

  这是一个简单的Ping程序，适用于Windows.  

之所以会有这个，是因为我的计算机网络课程要求写一个课设……用C/C++实现Ping程序。写完之后感觉放着有点浪费，遂上传过来。  

### 如何构建

  正如代码中的注释提到的，这份程序在Windows 10 x64下使用gcc 9.2.0编译通过。编译时使用 `-lws2_32` 选项指定链接到Winsock2静态库，整个的编译指令如下：  
  `g++ smping.cpp --std=c++11 -lws2_32 -o smping.exe`  
  事就这么成了（

### 使用说明

  注释里其实也有说，复制粘贴过来：  
  > 使用方法：smping \[-t\] \[-l size\] \[-n count\] \[-4\] \[-6]\ \[--debug\] target_name  
  > -t 一直ping指定的主机 直至用户强制停止（Ctrl + C）  
  > -l size 指定要发送的ICMP报文大小  
  > -n count 指定要发送的次数  
  > -4 强制使用IPv4  
  > -6 强制使用IPv6  
  > --debug 输出用于debug的额外信息  

  *IPv6还没有实现，等我什么时候摸鱼摸够了想起来吧*
