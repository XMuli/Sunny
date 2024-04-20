# sunny
A screenshot tool, and to achieve the function of the general module encapsulation



```bash


# 使用 dch -i 或用 dch -v version-revision 来指定版本  https://blog.csdn.net/qq_33154343/article/details/123778207

sudo apt update
sudo apt install cmake gcc g++ wget tar dh-make debmake lintian devscripts       
sudo apt install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools qtcreator   # qt可以运行一个初始项目

#【方式一】 在 Ubuntu 22.04 打包为 .deb ----------------------------------------------



nano ~/.bashrc
# 补充 git tab 补全
if [ -f /etc/bash_completion ]; then
  . /etc/bash_completion
fi
# 全局生效 qt 的依赖
export Qt5_DIR=/opt/Qt/5.15.2/gcc_64/lib/cmake/Qt5
export QT_DIR=/opt/Qt/5.15.2/gcc_64/lib/cmake/Qt5

source ~/.bashrc

// 然后执行后，就打包成了 .deb 软件包
$ dos2unix ../src/resources/cpack/tech.xmuli.sunny.desktop
mkdir bin & cd bin
cmake ../
make -j16
cpack -G DEB

$ sudo apt install ./Sunny-1.5.0.deb    # 安装
$ sudo apt remove sunny                 # 卸载

# 注意大小写等
dpkg -I ./Sunny-1.5.0-Linux-x86_64.deb            # 查看软件包的信息
dpkg -L sunny                                     # 查看包的文件等拷贝等信息
ldd /usr/bin/Sunny                                # 查看依赖的 dll

# 临时生效
sudo apt install libqt5x11extras5
export LD_LIBRARY_PATH=/opt/Qt/5.15.2/gcc_64/lib:$LD_LIBRARY_PATH







```

