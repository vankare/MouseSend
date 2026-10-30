# FFST(Free Fast Send Tool)
`FFST` 是一个基于`NAT`打洞的文件传输工具。


# 服务端搭建
...

# 客户端使用
...

# FFST 协议
| 字段 | 长度 | 说明 |
|------|------|------|
| Type | 2B   | 报文类型 |
| Seq  | 8B   | 数据包序号 |

| Type | 含义         | 是否需要回复     |
|------|--------------|--------------  --|
| 0x01 | SYN          | 是(ACK)          |
| 0x02 | SYN\_ACK     | 是(ACK)          |
| 0x03 | ACK          | 否               |
| 0x05 | SIGNAL       | 是(ACK)          |
| 0x06 | FIN          | 是(ACK)          |
| 0x07 | HANDSHAKE1st | 是(HANDSHAKE2nd) |
| 0x08 | HANDSHAKE2nd | 是(HANDSHAKE3rd) |
| 0x09 | HANDSHAKE3rd | 否

`{{Type}}{{Seq}}{{Payload}}`

**规定总长度不超过10240字节, 即：data长度不超过10230字节。

## 三次握手阶段
第一次握手
A --> B:
```
type=HANDSHAKE1st, seq=0, payload=null
```

第二次握手
B --> A:
```
type=HANDSHAKE2nd, seq=0, payload=null
```

第三次握手
A --> B:
```
type=HANDSHAKE3rd, seq=0, payload=null
```

## 数据传输阶段
A --> B(A向B传输偏移fragment\_offset的数据):
```
type=syn, seq=fragment_offset(文件分片偏移), payload=data
```

B --> A(B向A回复偏移为fragment\_offset的数据):
```
type=ack, seq=fragment_offset, payload= null
```

## 分手阶段
