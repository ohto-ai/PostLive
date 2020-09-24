## `/api`

### 基本格式

Client

```json
{
    "account":"客户端账号",
    "token":"登录凭证",
    "type":"资源类型",
    "resources_name":"资源字段，根据需要选择"
}
```

Server

```json
{
    "account":"客户端账号",
    "token":"登录凭证",
    "success":"bool类型",
    "type":"资源类型",
    "info":"备注信息可选，信息可能会被呈现在客户端",
    "resources_name":"资源字段，根据需要选择"
}
```



### Login

>   根据login选择password或者token来验证

```json
{
    "account":"user",
    "passsword":"BD4FEB959D6CCD62DCFD6F9CAC11AEAD",
    "token":"4314C0A84B7E97588E7C35E055AC5070",
    "type":"login",
    "login":"token"
}
```

>   返回

```json
{
    "account":"user",
    "token":"4314C0A84B7E97588E7C35E055AC5070",
    "success":true,
    "type":"login",
    "info":"login success."
}
```

### Profile

>   获取个人信息

```json
{
    "account":"user",
    "token":"4314C0A84B7E97588E7C35E055AC5070",
    "type":"profile",
    "profile":["avatar","gender","abc"]
}
```

>   返回

```json
{
	"account":"user",
	"token":"4314C0A84B7E97588E7C35E055AC5070",
    "success":true,
    "type":"profile",
    "info":"",
    "profile":[
        {
            "avatar":"http://thatboy.info:5120/uploads/medium/6ace02fe577e2ddf8c8736cdf6965374.jpg"
        },
        {
            "gender":"male"
        },
        {
            "abc":null
        }
    ]
}
```

![](http://thatboy.info:5120/uploads/medium/6ace02fe577e2ddf8c8736cdf6965374.jpg)

### Query

>   查询他人信息

```json
{
    "account":"user",
    "token":"4314C0A84B7E97588E7C35E055AC5070",
    "type":"query",
    "query":[
        {
            "account":"zhouh",
            "profile":["avatar","gender","online"]
        },
		{
            "account":"kongjh",
            "profile":["avatar","gender"]
        }
    ]
}
```

>   返回

```json
{
    "account":"user",
    "token":"4314C0A84B7E97588E7C35E055AC5070",
    "success":true,
    "type":"query",
    "info":"",
    "query":[
        {
            "account":"zhouh",
            "profile":[
                {
                	"avatar":"https://cdn.jsdelivr.net/gh/zhouhuan666/BlogAssets@0.11/SettingPic/avatar.JPG"
                },
                {
                    "gender":"female"
                },
                {
                    "online":false
                }                
            ]
        },
        {
            "account":"kongjh",
            "profile":[
                {
                    "avartar":"http://thatboy.info:5120/uploads/big/41587179aa6f591b5abc2dbda3dd5a2d.png"
                },
                {
                    "gender":"unknow"
                }
            ]
        }
    ]
}
```

![](https://cdn.jsdelivr.net/gh/zhouhuan666/BlogAssets@0.11/SettingPic/avatar.JPG)

![](http://thatboy.info:5120/uploads/big/41587179aa6f591b5abc2dbda3dd5a2d.png)

### Heartbeat

>   客户端定时发送，维持在线

```json
{
    "account":"user",
    "token":"4314C0A84B7E97588E7C35E055AC5070",
    "type":"heartbeat"
}
```

```json
{
    "account":"user",
    "token":"4314C0A84B7E97588E7C35E055AC5070",
    "success":true,
    "type":"heartbeat",
    "info":""
}
```

