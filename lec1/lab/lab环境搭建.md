# 参考
https://pdos.csail.mit.edu/6.828/2020/tools.html
Installing via APT (Debian/Ubuntu)章节

# 遇到的问题

## 更新openssl

1. 下载[ openssl-1.1.1i.tar.gz](https://www.openssl.org/source/openssl-1.1.1i.tar.gz)
2. tar xvf
3. cd openssl
4. ./config
5. make
6. make  install

## 安装Ninja
sudo apt-get install ninja-build

# 下载lab代码
```
$ git clone git://g.csail.mit.edu/xv6-labs-2020
Cloning into 'xv6-labs-2020'...
...
$ cd xv6-labs-2020
$ git checkout util
Branch 'util' set up to track remote branch 'util' from 'origin'.
Switched to a new branch 'util'
```