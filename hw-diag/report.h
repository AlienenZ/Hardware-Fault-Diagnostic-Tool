#pragma once
#include <string>
#include "detect.h"

bool ExportHtml(const std::wstring& path, const DetectResult& result);
bool ExportTxt(const std::wstring& path, const DetectResult& result);
