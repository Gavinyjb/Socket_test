### //关于编译

gcc -o client client.c -w -lpthread

### //chat_server 编译自chat_server.c

全功能 服务器 可以用来进行数据的选择发送

### //chat_server-v2 编译自chat_server-v2.c

被用来测试a客户端经由B服务端到达C客户端的时间

### //chat_server-v3 编译自chat_server-v3.c

被用来测试A服务端到达C客户端的时间
### //绕经测试

#### 1.查看服务器B网络信息 

```
ifconfig
```


例如:

```
root@master:/home/yjb/Socket_test# ifconfig
enp3s0    Link encap:Ethernet  HWaddr 10:bf:48:0b:e2:df  
          inet addr:172.19.205.30  Bcast:172.19.255.255  Mask:255.255.0.0
          inet6 addr: fc00:5a24:1958:1:12bf:48ff:fe0b:e2df/64 Scope:Global
          inet6 addr: fe80::12bf:48ff:fe0b:e2df/64 Scope:Link
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:3870023 errors:0 dropped:0 overruns:0 frame:0
          TX packets:102967 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000 
          RX bytes:274786424 (274.7 MB)  TX bytes:31675739 (31.6 MB)
          Interrupt:35 Memory:fbce0000-fbd00000 

enp6s0f0  Link encap:Ethernet  HWaddr a0:36:9f:1f:ac:3c  
          inet addr:172.20.205.30  Bcast:172.20.255.255  Mask:255.255.0.0
          inet6 addr: fe80::a236:9fff:fe1f:ac3c/64 Scope:Link
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:74322 errors:0 dropped:0 overruns:0 frame:0
          TX packets:292 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000 
          RX bytes:25154329 (25.1 MB)  TX bytes:25587 (25.5 KB)

ib0       Link encap:UNSPEC  HWaddr A0-00-02-20-FE-80-00-00-00-00-00-00-00-00-00-00  
          inet addr:172.21.205.30  Bcast:172.21.255.255  Mask:255.255.0.0
          inet6 addr: fe80::12d2:c910:0:9541/64 Scope:Link
          UP BROADCAST RUNNING MULTICAST  MTU:2044  Metric:1
          RX packets:6940243 errors:0 dropped:0 overruns:0 frame:0
          TX packets:3314759 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:256 
          RX bytes:2302760464 (2.3 GB)  TX bytes:3732013818 (3.7 GB)

lo        Link encap:Local Loopback  
          inet addr:127.0.0.1  Mask:255.0.0.0
          inet6 addr: ::1/128 Scope:Host
          UP LOOPBACK RUNNING  MTU:65536  Metric:1
          RX packets:279076 errors:0 dropped:0 overruns:0 frame:0
          TX packets:279076 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1 
          RX bytes:2191364163 (2.1 GB)  TX bytes:2191364163 (2.1 GB)
```

则：

```
root@master:/home/yjb/Socket_test# ./chat_server 11111 enp3s0 22222 enp6s0f0
main() : creating thread, 0
main() : creating thread, 1
<[ SERVER STARTED ]>
<[ SERVER STARTED ]>
```

#### 2.在客户端A和C上分别连接不同的Socket

A：

```
./client 172.19.205.30 11111
```

```
root@slave1:/home/yjb/RTT# ./client 172.19.205.30 11111
flagflagflag
1618316298186922 <-收时间戳(微秒) flagflagflag
1618316298187105 <-收时间戳(微秒) flagflagflag
1618316298187310 <-收时间戳(微秒) flagflagflag
1618316298187545 <-收时间戳(微秒) flagflagflag
1618316298187744 <-收时间戳(微秒) flagflagflag
1618316298187874 <-收时间戳(微秒) flagflagflag

```

> 详细数据见5-4.txt

C：

```
./client 172.20.205.30 22222
```

```
root@slave2:/home/yjb/RTT# ./client 172.20.205.30 22222
flagflagflag
flagflagflag
1618316298217655 <-收时间戳(微秒) flagflagflag
1618316298217812 <-收时间戳(微秒) flagflagflag
1618316298218051 <-收时间戳(微秒) flagflagflag
1618316298218254 <-收时间戳(微秒) flagflagflag
1618316298218420 <-收时间戳(微秒) flagflagflag
1618316298218570 <-收时间戳(微秒) flagflagflag
1618316298218771 <-收时间戳(微秒) flagflagflag

```

> 数据见5-5.txt

#### 3.利用ping命令直接连接A与C

```
ping 172.19.205.50
```

> 数据见ping 1000To1000.txt

```
ping 172.20.205.50
```

> 数据见ping 1000To10000.txt

#### 4. 把A作为服务端 C作为客户端

建立服务端：

```
ifconfig //查看A的网络信息
```

```
root@slave1:/home/yjb/RTT# ./chat_server-v3 11111 enp3s0 22222 enp6s0f0
main() : creating thread, 0
main() : creating thread, 1
<[ SERVER STARTED ]>
<[ SERVER STARTED ]>
```

建立客户端：

```
root@slave2:/home/yjb/RTT# ./client 172.20.205.30 22222
```

发送消息 获取数据

```
root@slave2:/home/yjb/RTT# ./client 172.20.205.30 22222
tttttttttttt
tttttttttttt
1618361813834498 <-收时间戳(微秒) tttttttttttt
1618361813834708 <-收时间戳(微秒) tttttttttttt
1618361813834892 <-收时间戳(微秒) tttttttttttt
1618361813835110 <-收时间戳(微秒) tttttttttttt
1618361813835289 <-收时间戳(微秒) tttttttttttt
1618361813835496 <-收时间戳(微秒) tttttttttttt
1618361813835671 <-收时间戳(微秒) tttttttttttt
1618361813835870 <-收时间戳(微秒) tttttttttttt
1618361813836031 <-收时间戳(微秒) tttttttttttt
```

> 见AToC.txt

#### 4.数据处理

> 数据见RTT.XLSX