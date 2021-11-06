<a href="https://github.com/Ohto-Ai/PostLive/blob/main/LICENSE"><img alt="GitHub license" src="https://img.shields.io/github/license/Ohto-Ai/PostLive"></a>
<a href="https://github.com/Ohto-Ai/PostLive/issues"><img alt="GitHub issues" src="https://img.shields.io/github/issues/Ohto-Ai/PostLive"></a>
<a href="https://github.com/Ohto-Ai/PostLive/network"><img alt="GitHub forks" src="https://img.shields.io/github/forks/Ohto-Ai/PostLive"></a>
<a href="https://github.com/Ohto-Ai/PostLive/stargazers"><img alt="GitHub stars" src="https://img.shields.io/github/stars/Ohto-Ai/PostLive"></a>
<a href="https://github.com/Ohto-Ai/PostLive/stargazers"><img src="https://img.shields.io/badge/WELCOME-STAR-red"></a>
<p align="center">
 <img width="100%" src="https://camo.githubusercontent.com/15d6e5fefc9de9b33d8776b8a9a6a15a8e823cea/68747470733a2f2f692e6c6f6c692e6e65742f323032302f31302f31382f39485054534b7a5152794e663741772e706e67" align="center" alt="GitHub Readme Stats" />
 <h2 align="center">Live Platform</h2>
 <p align="center">学习直播平台-客户端</p>

# 界面设计

-   登录  
-   ![效果图](https://i.loli.net/2020/09/27/DtsjhlAfMHndr2q.png)
-   登录设置  
-   ![遮罩图](https://i.loli.net/2020/09/27/nAdxPvT23F5bY64.png)
-   主窗口  
-   ![主窗口](https://i.loli.net/2020/09/29/UbYiFs1nlQPeEXc.png)

# 功能计划

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
    -   [x] 视频预览
        -   [ ] 预览开关
    -   [x] ffmpeg推流及停止
-   [ ] Bug修复
    -   [ ] 遮罩窗口最小化还原后无法显示遮罩层 [commit 96e5347](https://github.com/NOPornLivePlatform/LivePlatform/commit/a1393e960577cd6f3df00ff979d7e90c2ef7f899)
-   [ ] 其他
    -   [ ] 最小化到托盘

# 用户名和密码正则

-   用户名

`^[A-Za-z][A-Za-z0-9_-]{3,15}$`

-   密码

`^[A-Za-z0-9._~!@#$^&*]{6,24}$`

# API说明

[内容在此文档](API.md)
