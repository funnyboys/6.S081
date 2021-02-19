# 非root用户安装python3.8.7
## 下载源码
官网选择 source code ，下载对应版本。
## 编译
```
./configure --prefix=/home/huangchangwei/bin/Python-3.8.7
make install
```
configure后面的prefix为要安装python的bin路径。
# 非root用户安装qemu-5.2.0
## 下载源码
官网下载源码后，解压。
## 设置编译变量
./configure用于检测安装平台的目标特性，比如会检测是否有GCC。
configure只是个 shell 脚本，这里并不会实际去调用。
```
./configure --prefix=/home/huangchangwei/bin/qemu 
```
注意这里设置 prefix 会指定安装路径。
### configure中出现的问题
#### python版本过低
报错信息为：
```
huangchangwei@ubuntu:~/qemu-5.2.0$ ./configure --prefix=/home/huangchangwei/bin/qemu
Using './build' as the directory for build output
ERROR: Cannot use '/usr/bin/python3', Python >= 3.6 is required.
       Use --python=/path/to/python to specify a supported Python.
```
由于服务器默认的 python 版本太低，需要指定自己安装的 python 路径。
```
./configure --prefix=/home/huangchangwei/bin/qemu --python=/home/huangchangwei/bin/Python-3.8.7/bin/python3.8
```
#### 缺少ninja
报错信息为：
```
huangchangwei@ubuntu:~/qemu-5.2.0$ ./configure --prefix=/home/huangchangwei/bin/qemu --python=/home/huangchangwei/bin/Python-3.8.7/bin/python3.8
Using './build' as the directory for build output

ERROR: Cannot find Ninja
```
1. 下载ninja源码：
https://github.com/ninja-build/ninja/releases
2. 参考readme编译ninja：
```
./configure.py --bootstrap
```
3. 将ninja路径添加到.profile中
增加后如下：
```
PATH="/home/huangchangwei/bin/ninja-1.10.2:/opt/toolchains/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-elf/bin:/opt/toolchains/gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu/bin:/opt/toolchains/gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabi/bin:/opt/toolchains/gcc-linaro-6.4.1-2017.08-x86_64_arm-linux-gnueabihf/bin:$PATH"
```
## 编译
make 用来编译，它从 Makefile 中读取指令，然后编译。
## 安装
make install 用来安装，它从 Makefile 中读取指令，安装到指定的位置。