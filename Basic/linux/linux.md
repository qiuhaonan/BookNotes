

# chapter 1 linux

------

## 1.about make grpc

2016.10.3 
linux下从源码安装grpc时需要先安装一些依赖库
安装方式如下：
sudo apt-get install build-essential autoconf libtool
然后还要安装protobuf以及protoc编译工具
使用git获取源码之后需要进行make编译
这会对源码里面的文件进行编译链接
然后执行sudo make install 从源码编译的文件链接
成动态库放到/usr/local/lib路径下面，每次如果有修改源码并
想重新将动态库更新到系统目录下，就要使用make clean清理先前make
产生的编译结果，然后再重新执行make 以及make install命令
然而这只是对系统的动态库进行了更新，如果自己写的程序文件需要用
make进行编译生成可执行文件时，就要注意此文件是否将动态库编译进了文件
中，如果是，那么即使更新了系统的动态库，你去运行程序时也会发现
调用的动态库执行结果仍然是先前没有修改过的执行代码，因为你执行的
可执行文件里的动态库没有进行更新，所以要对自己写的代码进行重新make
也就是说要先make clean然后再make将新的系统动态库链接到程序中来

## 2.about root

普通用户要使用sudo权限，需要在/etc/sudoers文件里进行修改，先在root用户下使用vim对该文件进行编辑，在root ALL=(ALL)  ALL下面添加一行，把名字修改为名然后保存即可，之后该用户就可以使用root权限进行操作。

## 3.about gdb

linux下边编译程序通常会出现core dump现象，此时一般会在编译的文件夹下生成core文件
,此时使用gdb对core文件进行调试，然后使用bt或者where命令即可查看函数调用栈信息，进而观察出错信息。

## 4.about make file

makefile规则  

​	target：prerequisites...   （target 可以是object文件也可以是执行文件或者标签文件）（prerequisite是要生成target文件所需要的文件）

​	【此处有一个tab】	command    （shell命令）（prerequisite中有一个以上的文件比target文件新的话，command命令就会执行）