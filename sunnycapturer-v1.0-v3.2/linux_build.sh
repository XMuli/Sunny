#!/bin/bash

# Step 1: 检查依赖项并安装
echo
echo "===================================="
echo "Step 1: Checking and installing required dependencies..."
DEPENDENCIES=("libfuse2" "libxcb-cursor0" "libxcb-cursor-dev" "git" "vim" "cmake" "wget" "tar" "libgl1-mesa-dev" "libglu1-mesa-dev" "freeglut3-dev" "dos2unix" "libxrandr-dev")

# 判断当前操作系统版本
OS_VERSION=$(lsb_release -r | awk '{print $2}')
IS_UBUNTU_2004=false
COMPILER_DEPENDENCIES=()

if [[ "$OS_VERSION" == "20.04" ]]; then
    IS_UBUNTU_2004=true
    echo "Detected Ubuntu 20.04. Using gcc-10 and g++-10 as compilers."
    COMPILER_DEPENDENCIES=("gcc-10" "g++-10")
else
    echo "Detected non-Ubuntu 20.04. Using default gcc and g++ compilers."
    COMPILER_DEPENDENCIES=("gcc" "g++")
fi

# 合并所有依赖项
ALL_DEPENDENCIES=("${DEPENDENCIES[@]}" "${COMPILER_DEPENDENCIES[@]}")

# 安装缺失的依赖项
for dep in "${ALL_DEPENDENCIES[@]}"; do
    dpkg -l | grep -qw "$dep" || {
        echo "$dep is not installed. Installing..."
        sudo apt-get install -y "$dep" || { echo "Failed to install $dep. Exiting."; exit 1; }
    }
done

echo "All required dependencies are installed."

# Step 2: 修改 glog/CMakeLists.txt 文件
echo
echo "===================================="
echo "Step 2: Modifying 3rdparty/glog/CMakeLists.txt..."
echo "Current path: $(pwd)"
CMAKE_FILE="3rdparty/glog/CMakeLists.txt"
if $IS_UBUNTU_2004; then
    if [[ -f "$CMAKE_FILE" ]]; then
        sed -i '1s/cmake_minimum_required (VERSION 3\.21)/cmake_minimum_required (VERSION 3.16)/' "$CMAKE_FILE"
        echo "Updated cmake_minimum_required version to 3.16 in $CMAKE_FILE"
    else
        echo "File $CMAKE_FILE not found. Skipping."
    fi
else
    echo "Skipping glog CMakeLists modification for non-Ubuntu 20.04."
fi

# Step 3: 转换文件换行符为 UNIX 格式
echo
echo "===================================="
echo "Step 3: Converting files to UNIX format..."
echo "Current path: $(pwd)"
find . -type f -exec dos2unix {} \;

# Step 4: 创建 bin 目录
echo
echo "===================================="
echo "Step 4: Creating bin directory..."
echo "Current path: $(pwd)"
mkdir -p bin

# Step 5: 进入 bin 目录
echo
echo "===================================="
echo "Step 5: Changing directory to bin..."
echo "Current path: $(pwd)"
cd bin

# Step 6: 执行 cmake 配置
echo
echo "===================================="
echo "Step 6: Running cmake configuration..."
echo "Current path: $(pwd)"
if $IS_UBUNTU_2004; then
    echo "Running Ubuntu 20.04-specific cmake command..."
    cmake -DCMAKE_C_COMPILER=/usr/bin/gcc-10 \
          -DCMAKE_CXX_COMPILER=/usr/bin/g++-10 \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_PREFIX_PATH=~/Qt/6.7.3/gcc_64 \
          -DQt6_DIR=~/Qt/6.x.x/gcc_64/lib/cmake/Qt6 ..
else
    echo "Running default cmake command..."
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_PREFIX_PATH=~/Qt/6.7.3/gcc_64 \
          -DQt6_DIR=~/Qt/6.x.x/gcc_64/lib/cmake/Qt6 ..
fi

# Step 7: 编译
echo
echo "===================================="
echo "Step 7: Building the project..."
echo "Current path: $(pwd)"
# 使用当前系统的最大内核数进行并行编译
MAKE_JOBS=$(nproc)  # 获取系统的核心数
echo "Using $MAKE_JOBS cores for compilation (make -j$MAKE_JOBS)"
make -j$MAKE_JOBS

# Step 8: 检查并下载 linuxdeploy-x86_64.AppImage 和 linuxdeploy-plugin-qt-x86_64.AppImage
echo
echo "===================================="
echo "Step 8: Checking for required AppImages..."
echo "Current path: $(pwd)"

# 根目录的同级目录
PARENT_DIR=$(dirname "$(pwd)")
LINUXDEPLOY_FILE="$PARENT_DIR/../linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_PLUGIN_FILE="$PARENT_DIR/../linuxdeploy-plugin-qt-x86_64.AppImage"

# 检查文件是否已存在
if [[ ! -f "$LINUXDEPLOY_FILE" ]]; then
    echo "$LINUXDEPLOY_FILE not found. Downloading..."
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/2.0.0-alpha-1-20241106/linuxdeploy-x86_64.AppImage -P "$PARENT_DIR/.."
else
    echo "$LINUXDEPLOY_FILE already exists. Skipping download."
fi

if [[ ! -f "$LINUXDEPLOY_PLUGIN_FILE" ]]; then
    echo "$LINUXDEPLOY_PLUGIN_FILE not found. Downloading..."
    wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/2.0.0-alpha-1-20241106/linuxdeploy-plugin-qt-x86_64.AppImage -P "$PARENT_DIR/.."
else
    echo "$LINUXDEPLOY_PLUGIN_FILE already exists. Skipping download."
fi

# 添加运行权限
if [[ -f "$LINUXDEPLOY_FILE" ]]; then
    echo "Adding execute permissions to $LINUXDEPLOY_FILE..."
    chmod +x "$LINUXDEPLOY_FILE"
fi

if [[ -f "$LINUXDEPLOY_PLUGIN_FILE" ]]; then
    echo "Adding execute permissions to $LINUXDEPLOY_PLUGIN_FILE..."
    chmod +x "$LINUXDEPLOY_PLUGIN_FILE"
fi

# Step 9: 执行 export PATH 环境变量
echo
echo "===================================="
echo "Step 9: Exporting PATH variable..."
echo "Current path: $(pwd)"
export PATH=~/Qt/6.7.3/gcc_64/bin:$PATH

# Step 10: 回到根目录
echo
echo "===================================="
echo "Step 10: Changing directory to the root of the project..."
echo "Current path: $(pwd)"
cd ..

# Step 11: 使用 linuxdeploy 创建 AppImage
echo
echo "===================================="
echo "Step 11: Creating the AppImage..."
echo "Current path: $(pwd)"

# 确保目标目录存在
mkdir -p "./AppDir/opt/SunnyCapturer/translations"
# 复制所有翻译文件到目标路径
cp -r "$(pwd)/bin/translations"/*.qm "./AppDir/opt/SunnyCapturer/translations/"
ls "./AppDir/opt/SunnyCapturer/translations"

echo "Current path: $(pwd)"

../linuxdeploy-x86_64.AppImage --appdir AppDir \
    -e ./bin/SunnyCapturer \
    -d ./bin/resources/linux_script/sunnycapturer_appimage.desktop \
    -i ./bin/resources/logo/logo.svg \
    --icon-filename sunnycapturer \
    -p qt -o appimage

# Step 12: 拷贝 lib 和 plugins 文件夹，并删除 .debug 文件
echo
echo "===================================="
echo "Step 12: Copying lib and plugins directories and removing .debug files..."
echo "Current path: $(pwd)"

# 创建新的文件夹
NEW_DEPENDENCY_DIR="linux_qt6_dependency"
mkdir -p "$NEW_DEPENDENCY_DIR/lib" "$NEW_DEPENDENCY_DIR/plugins"

# 拷贝整个 lib 目录并保留目录结构，同时去掉 AppDir/usr
echo "Copying ./AppDir/usr/lib to $NEW_DEPENDENCY_DIR/lib..."
cp -r ./AppDir/usr/lib/* "$NEW_DEPENDENCY_DIR/lib/"

# 拷贝整个 plugins 目录并保留目录结构，同时去掉 AppDir/usr
echo "Copying ./AppDir/usr/plugins to $NEW_DEPENDENCY_DIR/plugins..."
cp -r ./AppDir/usr/plugins/* "$NEW_DEPENDENCY_DIR/plugins/"

# 遍历删除所有 .debug 文件
echo "Removing .debug files from $NEW_DEPENDENCY_DIR..."
find "$NEW_DEPENDENCY_DIR" -type f -name "*.debug" -exec rm -f {} \;

echo "Completed copying files to $NEW_DEPENDENCY_DIR with .debug files removed."

# Step 13: 创建压缩文件夹并自定义文件名
echo
echo "===================================="
echo "Step 13: Creating zip file..."
echo "Current path: $(pwd)"

# 获取当前操作系统的名称和版本
OS_NAME=$(lsb_release -si)  # 获取操作系统名称（如 Ubuntu）
OS_VERSION=$(lsb_release -r | awk '{print $2}')  # 获取操作系统版本（如 20.04）

# 构造zip文件的名称
if $IS_UBUNTU_2004; then
    GCC_VERSION=$(gcc-10 --version | head -n 1 | awk '{print $3}' | cut -d'-' -f1)
else
    GCC_VERSION=$(gcc --version | head -n 1 | awk '{print $3}' | cut -d'-' -f1)
fi

# 构造zip文件的名称
ZIP_FILE="${NEW_DEPENDENCY_DIR}_${OS_NAME}${OS_VERSION}_gcc${GCC_VERSION}_remove_debugfile.zip"

# 创建zip文件
zip -r "$ZIP_FILE" "$NEW_DEPENDENCY_DIR"
echo "Created zip file: $ZIP_FILE"

# Step 14: 进入 bin 目录
echo
echo "===================================="
echo "Step 14: Changing directory to bin..."
echo "Current path: $(pwd)"
cd ./bin

# Step 15: 执行 cpack 打包为 .deb 文件
echo
echo "===================================="
echo "Step 15: Creating DEB package using cpack..."
echo "Current path: $(pwd)"
cpack -G DEB
echo "Generated DEB package: $(ls | grep .deb)"

# Step 16: 创建新的文件夹并移动文件
echo
echo "===================================="
echo "Step 16: Creating new folder and moving files..."
echo "Current path: $(pwd)"
TIMESTAMP=$(date "+%Y%m%d_%H%M%S")
DEST_DIR="../Output_SunnyCapturer_$TIMESTAMP"
mkdir -p "$DEST_DIR"
echo "Created directory: $DEST_DIR"

# Step 16.1: 获取 .deb 文件的修改时间戳
# 使用版本号和时间戳创建新文件名
DEB_FILE=$(ls *.deb)  # 取得最新的 .deb 文件名
NAME=$(basename "$DEB_FILE" .deb)
NEW_NAME="${NAME}_x64_${TIMESTAMP}"
NEW_DEB_NAME="${NEW_NAME}.deb"
NEW_APPIMAGE_NAME="${NEW_NAME}.AppImage"

echo "--->${NEW_DEB_NAME}"
echo "--->${NEW_APPIMAGE_NAME}"
echo "--->${ZIP_FILE}"

# Step 16.3: 重命名 .AppImage 文件
echo "Renaming .AppImage file to: $NEW_APPIMAGE_NAME"
mv "../SunnyCapturer-x86_64.AppImage" "$DEST_DIR/$NEW_APPIMAGE_NAME"

# Step 16.4: 重命名 .deb 文件
echo "Renaming .deb file to: $NEW_DEB_NAME"
mv "$DEB_FILE" "$DEST_DIR/$NEW_DEB_NAME"

# Step 16.5: 拷贝 .zip 备用
mv "../${ZIP_FILE}" "$DEST_DIR"

# 给 .deb 和 .AppImage 文件添加运行权限
chmod +x "$DEST_DIR"/*.deb "$DEST_DIR"/*.AppImage
echo "Added execute permissions to .deb and .AppImage files in $DEST_DIR"

ls -al ./Output*
echo "===================================="
echo "Script completed!"

