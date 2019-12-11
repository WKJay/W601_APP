## 使用步骤

1. 编译前需要将该工程文件夹放入 RT-Thread_W60X_SDK/examples 目录下
[RT-Thread_W60X_SDK 获取](https://github.com/RT-Thread/W601_IoT_Board)

2. pkgs --update 更新软件包
3. scons --target=mdk5 -s 编译
4. 下载程序
5. 等待W601连接网络后，用[adb工具](http://packages.rt-thread.org/detail.html?package=adbd)上传[W601_WEB](https://github.com/WKJay/W601_WEB)中的网页到W601 WebNet分区中
6. 重启W601，输入网址进入网页


### 注意事项

1. 由于网络连接为wifi，adb在传输文件时可能不是很稳定，若发生错误需要多尝试几次。
2. 由于网页文件较大，第一次打开网页时需要等待的时间可能稍长。 

## 网页说明

### 默认页面 

![default](/dist/default.png)

### 邮箱设置

![smtp](/dist/smtp.png)

### 邮箱配置示例

![smtp_config](/dist/smtp_config.png)

