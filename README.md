# LivePlatform

## 登录界面[UI]

-   设计稿

![设计稿](https://i.loli.net/2020/09/24/3PsKl2gHXV8ErAQ.png)  

-   施工稿

    ![施工稿](https://i.loli.net/2020/09/24/5coQnqsKPRE8ZlL.png)

-   效果

![pilipili-login-example](https://i.loli.net/2020/09/24/QdH3lpCrbzt1PMa.png)

## 用户名和密码正则

-   Account

    `^[A-Za-z][A-Za-z0-9_-]{3,15}$`

    3-15位大小写字母数字以及减号和下划线组成，第一个字符必须为字母

-   Password

    `^[A-Za-z0-9\`~!@#$%^&*()=+?.:\|/<>_-]{6,24}\$`

    `