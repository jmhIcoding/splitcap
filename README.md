# 背景
当PCAP原始文件特别巨大的时候，整个文件直接载入内存是相当耗时的，于是一个简单的想法是将大的PCAP切分成若干小PCAP。对于这个任务，现有工具splitcap是可以完成的。无论是按照主机对、还是按照五元组信息切分，splitcap都会将原始PCAP切分的过于分散。考虑一个包括100W个会话的、文件大小为6G的原始PCAP，经过splitcap切换后可能会得到100W个小pcap文件。往文件系统写这100W个小文件可能极其耗时，同时使用第三方工具（例如flowcontainer）专门去解析这100W个小文件可能所需的时间反而远远大于直接解析6GB的PCAP文件。

在这种背景下，本项目想完成如下几个需求：

1. 将大型PCAP切分为给定数目 $M$ 个小型PCAP文件。这 $M$ 个小文件某种指标尽量相同。这种指标可能是：1. 文件大小; 2. packet数目; 3. 双向流数目 等
2. 来自同一双向流的packet必须划分到同一PCAP文件内。
3. 跨平台，支持 Win/Linux 平台

# 安装
## Windows平台
### 重新编译
### 预编译
## Linux平台
## 重新编译
## 预编译

# 使用
