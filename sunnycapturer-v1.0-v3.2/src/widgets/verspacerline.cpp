// SPDX-SnippetCopyrightText: 2024-2025 SunnyCapturer Project
// SPDX-Author: Wengeng Zhang <xmulitech@gmail.com>
#include "verspacerline.h"

VerSpacerLine::VerSpacerLine(int length, QWidget *parent, Qt::WindowFlags f)
{
    setFrameShape(VLine);
    setFrameShadow(Sunken);
    setLineWidth(0);
    setMidLineWidth(2);
    setFixedHeight(length);
}
