@echo off
setlocal enabledelayedexpansion

:: 设置默认变量
set "VERSION=3.2.0"
set "NAME=SunnyCapturer"
set "TYPE=protable" 
set "CPU_MODE=offline_ocr_cpu" 
set "GPU_MODE=offline_ocr_gpu" 
set "TOOL_7Z=C:\Program Files\7-Zip\7z.exe" 

:: 配置文件路径和变量
set "SOURCE_BIN=.\bin"
set "TARGET_FOLDER_CPU=.\bin_%CPU_MODE%"
set "TARGET_FOLDER_GPU=.\bin_%GPU_MODE%"
set "OCR_CPU_SRC=D:\project\OCR\PaddleOCR_v2.9.1\release_dependency\Release_2.9.1_cpu"
set "OCR_GPU_SRC=D:\project\OCR\PaddleOCR_v2.9.1\release_dependency\Release_2.9.1_gpu"

echo "****************** 开始操作 ******************"

:: 确保源文件夹存在
if not exist "%SOURCE_BIN%" (
    echo 源文件夹 "%SOURCE_BIN%" 不存在!
    exit /b 1
)

:: 清理目标文件夹（如果存在）
if exist "%TARGET_FOLDER_CPU%" (
    echo 删除现有的 "%TARGET_FOLDER_CPU%" 文件夹...
    rmdir /s /q "%TARGET_FOLDER_CPU%"
)
if exist "%TARGET_FOLDER_GPU%" (
    echo 删除现有的 "%TARGET_FOLDER_GPU%" 文件夹...
    rmdir /s /q "%TARGET_FOLDER_GPU%"
)

:: 创建输出文件夹
set "OUTPUT_FOLDER=.\Output_%VERSION%"
if exist "%OUTPUT_FOLDER%" (
    echo 删除现有的 "%OUTPUT_FOLDER%" 文件夹...
    rmdir /s /q "%OUTPUT_FOLDER%"
)

if not exist "%OUTPUT_FOLDER%" (
    echo 创建文件夹 "%OUTPUT_FOLDER%"...
    mkdir "%OUTPUT_FOLDER%"
)

:: 确认输出文件夹已存在
if not exist "%OUTPUT_FOLDER%" (
    echo 输出文件夹 "%OUTPUT_FOLDER%" 创建失败，请检查路径！
    exit /b 1
)

:: 复制原始 bin 文件夹到两个目标文件夹
echo 复制原始文件夹到 CPU 和 GPU 文件夹...
xcopy /e /h /i /q "%SOURCE_BIN%" "%TARGET_FOLDER_CPU%"
xcopy /e /h /i /q "%SOURCE_BIN%" "%TARGET_FOLDER_GPU%"

:: 拷贝 OCR 相关文件到目标文件夹
echo 拷贝 CPU OCR 文件到 "%TARGET_FOLDER_CPU%"...
xcopy /e /h /i /q "%OCR_CPU_SRC%\*" "%TARGET_FOLDER_CPU%"

echo 拷贝 GPU OCR 文件到 "%TARGET_FOLDER_GPU%"...
xcopy /e /h /i /q "%OCR_GPU_SRC%\*" "%TARGET_FOLDER_GPU%"

:: 压缩文件夹
echo 压缩文件夹为 .7z 文件...
"%TOOL_7Z%" a "%OUTPUT_FOLDER%\%NAME%_%TYPE%_%VERSION%_x64.7z" ".\%SOURCE_BIN%\*"
"%TOOL_7Z%" a "%OUTPUT_FOLDER%\%NAME%_%TYPE%_%VERSION%_x64_%CPU_MODE%.7z" ".\%TARGET_FOLDER_CPU%\*"
"%TOOL_7Z%" a "%OUTPUT_FOLDER%\%NAME%_%TYPE%_%VERSION%_x64_%GPU_MODE%.7z" ".\%TARGET_FOLDER_GPU%\*"

echo 压缩完成!

"C:\Program Files (x86)\Inno Setup 6\Compil32.exe" /cc ".\setup_package.iss"

:: 获取符合条件的 .exe 文件名（没有后缀）
for %%f in (%NAME%_setup_*_x64.exe) do (
    set "FILENAME=%%~nf"
    echo 文件名（没有后缀）: !FILENAME!
)

:: 将生成的 exe 文件移动到 Output 文件夹
echo 将生成的 setup exe 文件剪切到 "%OUTPUT_FOLDER%" 文件夹中...
move /y "%FILENAME%.exe" "%OUTPUT_FOLDER%"

:: 删除源 bin 文件夹
:: echo 删除源文件夹 "%SOURCE_BIN%"...
:: rmdir /s /q "%SOURCE_BIN%"
echo 源文件夹 "%SOURCE_BIN%" 改名为 "%SOURCE_BIN%_original"
ren "%SOURCE_BIN%" "bin_original"

:: 重命名 %TARGET_FOLDER_CPU% 为 bin
echo 将 "%TARGET_FOLDER_CPU%" 重命名为 "%SOURCE_BIN%"...
ren "%TARGET_FOLDER_CPU%" bin

"C:\Program Files (x86)\Inno Setup 6\Compil32.exe" /cc ".\setup_package.iss"
ren "%FILENAME%.exe" "%FILENAME%_%CPU_MODE%.exe"
move /y "%FILENAME%_%CPU_MODE%.exe" "%OUTPUT_FOLDER%"

:: 删除源 bin 文件夹
echo 删除源文件夹 "%SOURCE_BIN%"...
rmdir /s /q "%SOURCE_BIN%"

:: 重命名 %TARGET_FOLDER_GPU% 为 bin
echo 将 "%TARGET_FOLDER_GPU%" 重命名为 "%SOURCE_BIN%"...
ren "%TARGET_FOLDER_GPU%" bin

"C:\Program Files (x86)\Inno Setup 6\Compil32.exe" /cc ".\setup_package.iss"
ren "%FILENAME%.exe" "%FILENAME%_%GPU_MODE%.exe"
move /y "%FILENAME%_%GPU_MODE%.exe" "%OUTPUT_FOLDER%"

:: 删除源 bin 文件夹
echo 删除源文件夹 "%SOURCE_BIN%"...
rmdir /s /q "%SOURCE_BIN%"

ren ".\bin_original" bin

"C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe" sign /sha1 "8e383d678f5e22ddcdd42c9571d3b9a4bdbd2860" /tr http://time.certum.pl /td sha256 /fd sha256 /v "%OUTPUT_FOLDER%\%NAME%_setup*.exe"

echo 操作完成!

:: 检查并清空输出文件（如果已存在）
:: 配置输出文件路径
set "OUTPUT_FILE=%OUTPUT_FOLDER%\checksums.txt"

:: 如果输出文件不存在，则创建文件
if not exist "%OUTPUT_FILE%" (
    echo 创建输出文件: %OUTPUT_FILE%
    echo. > "%OUTPUT_FILE%"
)

:: 检查并清空输出文件（如果已存在）
if exist "%OUTPUT_FILE%" del /f /q "%OUTPUT_FILE%"

:: 遍历指定文件计算校验值
for /r "%OUTPUT_FOLDER%" %%f in (*) do (
    echo 处理文件: %%f

    :: 初始化哈希值
    set "MD5="
    set "SHA1="
    set "SHA256="

    :: 获取并输出 MD5 校验值，去除多余的提示信息，只保留哈希值
    for /f "skip=1 tokens=*" %%a in ('certutil -hashfile "%%f" MD5 ^| findstr /v /c:"CertUtil:"') do (
        set "MD5=%%a"
    )
    set "MD5=!MD5: =!"

    :: 获取并输出 SHA1 校验值，去除多余的提示信息，只保留哈希值
    for /f "skip=1 tokens=*" %%a in ('certutil -hashfile "%%f" SHA1 ^| findstr /v /c:"CertUtil:"') do (
        set "SHA1=%%a"
    )
    set "SHA1=!SHA1: =!"

    :: 获取并输出 SHA256 校验值，去除多余的提示信息，只保留哈希值
    for /f "skip=1 tokens=*" %%a in ('certutil -hashfile "%%f" SHA256 ^| findstr /v /c:"CertUtil:"') do (
        set "SHA256=%%a"
    )
    set "SHA256=!SHA256: =!"

    :: 输出计算出来的校验值到文件，使用制表符进行对齐
    echo %%~nxf >> "%OUTPUT_FILE%"
    echo MD5:     !MD5! >> "%OUTPUT_FILE%"
    echo SHA-1:   !SHA1! >> "%OUTPUT_FILE%"
    echo SHA-256: !SHA256! >> "%OUTPUT_FILE%"
    echo. >> "%OUTPUT_FILE%"
)

echo 校验值生成完成! 校验信息已写入到 %OUTPUT_FILE%

pause
