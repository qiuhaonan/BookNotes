chapter 1 linux
====
1.about make file
----
2016.10.3 <br/>
linux下从源码安装grpc时需要先安装一些依赖库<br/>
安装方式如下：<br/>
sudo apt-get install build-essential autoconf libtool<br/>
然后还要安装protobuf以及protoc编译工具
使用git获取源码之后需要进行make编译<br/>
这会对源码里面的文件进行编译链接<br/>
然后执行sudo make install 从源码编译的文件链接<br/>
成动态库放到/usr/local/lib路径下面，每次如果有修改源码并<br/>
想重新将动态库更新到系统目录下，就要使用make clean清理先前make<br/>
产生的编译结果，然后再重新执行make 以及make install命令<br/>
然而这只是对系统的动态库进行了更新，如果自己写的程序文件需要用<br/>
make进行编译生成可执行文件时，就要注意此文件是否将动态库编译进了文件<br/>
中，如果是，那么即使更新了系统的动态库，你去运行程序时也会发现<br/>
调用的动态库执行结果仍然是先前没有修改过的执行代码，因为你执行的<br/>
可执行文件里的动态库没有进行更新，所以要对自己写的代码进行重新make<br/>
也就是说要先make clean然后再make将新的系统动态库链接到程序中来
