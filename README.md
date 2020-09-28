# LivePlatform

## 界面设计

-   登录  
-   ![效果图](https://i.loli.net/2020/09/27/DtsjhlAfMHndr2q.png)
-   登录设置  
-   ![遮罩图](https://i.loli.net/2020/09/27/nAdxPvT23F5bY64.png)
-   主窗口  
-   ![主窗口](https://i.loli.net/2020/09/28/Ps4RUzmKDl1ZX3w.png)

## 功能计划

-   [ ] 登录界面
    -   [x] UI设计
    -   [x] 头像自动获取及显示
    -   [x] 用户名和密码自动填充(填充假密码)
    -   [x] 用户信息同步
    -   [ ] 自动登录
    -   [ ] 设置界面
        -   [x] 遮罩窗口
        -   [ ] 服务器设置
        -   [ ] 官网
    -   [x] 登录token

-   [x] 测试服务器
    -   [x] 用户登录验证
    -   [x] 用户信息获取
-   [ ] 主窗口
    -   [ ] 摄像头
        -   [x] 获取并选择摄像头
        -   [ ] 获取并选择分辨率
    -   [ ] 麦克风
        -   [ ] 获取并选择麦克风
    -   [ ] 输出分辨率选择
    -   [ ] 码率设置等
    -   [ ] 设置窗口
    -   [ ] 视频预览
    -   [x] ffmpeg推流及停止
-   [ ] Bug修复
    -   [ ] 遮罩窗口最小化还原后无法显示遮罩层 [commit 96e5347](https://github.com/NOPornLivePlatform/LivePlatform/commit/a1393e960577cd6f3df00ff979d7e90c2ef7f899)
-   [ ] 其他
    -   [ ] 最小化到托盘

## 用户名和密码正则

-   用户名

`^[A-Za-z][A-Za-z0-9_-]{3,15}$`

-   密码

`^[A-Za-z0-9._~!@#$^&*]{6,24}$`

## API说明

[内容在此文档](API.md)