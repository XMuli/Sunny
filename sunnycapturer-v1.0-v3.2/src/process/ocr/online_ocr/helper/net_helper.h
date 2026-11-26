// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#ifndef NET_HELPER_H
#define NET_HELPER_H

#include <vector>
#include <QObject>
#include <QVector>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QString>
#include <QRect>

enum class BaiduOcrChannel
{
    BOC_high_precision_location,       // 通用文字识别（高精度含位置版）     500 次/month
    BOC_standard_location,             // 通用文字识别（标准含位置版）      1000 次/month
    BOC_high_precision,                // 通用文字识别（高精度版）         1000 次/month
    BOC_standard,                      // 通用文字识别（标准版）           1000 次/month
};
Q_DECLARE_METATYPE(BaiduOcrChannel)                     // 可以被 QVariant 类型存储
BaiduOcrChannel& operator++(BaiduOcrChannel& channel);      // 使枚举支持自增操作(前缀自增运算符 ++channel)
BaiduOcrChannel operator++(BaiduOcrChannel& channel, int);  // 支持后缀自增运算符

struct Trans_Tenent
{
    QString source;
    QString target;
    QString source_name;  // 翻译名
    QString target_name;  // 翻译名
    Trans_Tenent() {}
};

enum class ImgCacheDir
{
    ICD_out_img_extract_text,  // 提取文字后保存的路径
    ICD_out_img_translate,     // 图片翻译后保存的路径
    ICD_all                    // 创建上面所有路径
};
Q_DECLARE_METATYPE(ImgCacheDir)                     // 可以被 QVariant 类型存储

QString getCacheFolder(const ImgCacheDir type);    // 确保和验证文件路径是存在的
QString getImgOrigFileName(const QString& path);   // 传入原始图片带路径，返回出来

std::vector<std::pair<QString, QString>> ListExtractText();
std::vector<std::pair<QString, QString>> ListImageTranslate();

// 输出对应的支持语言清单
std::vector<std::pair<QString, QString> > transListTenent(const bool source);
std::vector<std::pair<QString, QString> > transListBaidu(const bool source);
std::vector<std::pair<QString, QString> > transListYoudao(const bool source);
QString findKeyByValue(const std::vector<std::pair<QString, QString>> vec, const QString &value);

QFont adjustFontSizeToFitRect(QPainter& pa, const QString& text, const QRect& rt, int maxFontSize = 100);


#endif // NET_HELPER_H
