## 使用步骤

1. 编译前需要将该工程文件夹放入 RT-Thread_W60X_SDK/examples 目录下
2. pkgs --update 更新软件包
3. scons --target=mdk5 -s 编译
4. 下载程序
5. 等待W601连接网络后，用[adb工具](http://packages.rt-thread.org/detail.html?package=adbd)上传[W601_WEB](https://github.com/WKJay/W601_WEB)中的网页到W601 WebNet分区中
6. 重启W601，输入网址进入网页


## 网页说明

### 默认页面 

![default](/dist/default.png)

### 邮箱设置

![smtp](/dist/smtp.png)

