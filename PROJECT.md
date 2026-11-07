# FFST 协议
| 字段 | 长度 | 说明 |
|------|------|------|
| Type | 2B   | 报文类型 |
| Seq  | 8B   | 数据包序号 |

| Type | 含义         | 是否需要回复     | 说明                   |
|------|--------------|------------------|------------------------|
| 0x01 | SYN          | 是(ACK)          | 数据序列号             |
| 0x02 | ACK          | 否               | 应答序列号             |
| 0x04 | SIGNAL       | 是(SIGNAL)       | 获取对方地址           |
| 0x05 | FIN          | 是(FIN)          | 断开连接               |
| 0x06 | HANDSHAKE1st | 是(HANDSHAKE2nd) | 第一次握手             |
| 0x07 | HANDSHAKE2nd | 是(HANDSHAKE3rd) | 第二次握手             |
| 0x08 | HANDSHAKE3rd | 否               | 第三次握手             |
| 0x09 | CONNECT      | 是(CONNECT)      | 申请连接Signal Server  |

`{{Type}}{{Seq}}{{Payload}}`

# 申请连接信令服务器
该流程是caller, callee向signal\_server发送连接请求，表示对端机器上线。
caller, callee --> signal\_server
```
{{CONNECT}}{{0}}{{self_id}}
```


signal\_server需要保存caller, callee地址。
signal\_server --> caller, callee 
```
{{CONNECT}}{{0}}{{NULL}}
```

# 获取对方地址
该流程是self\_id的对端机器向signal\_server发送获取id为peer\_id的地址请求。
caller --> signal\_server
```
{{SIGNAL}}{{0}}{{self_id&peer_id}}
```

signal\_server返回请求的地址。
signal\_server --> caller
```
{{SIGNAL}}{{0}}{{NULL}}
```

# 数据传输
该过程是caller获取callee地址后，正式发送报文阶段。
该过程需要引入超时重传策略、静态窗口(后续考虑升级为滑动窗口)确保一定的可靠性。
caller --> callee
```
{{SYN}}{{seq}}{{data}}
```

callee返回确认报文，表示收到了序列号为seq的数据报。
callee --> caller
```
{{ACK}}{{seq}}{{NULL}}
```

# 断开连接
该过程为caller与callee数据传输完后(或者数据传输失败)后的行为。
caller --> callee, signal\_server
```
{{FIN}}{{0}}{{NULL}}
```
