#pragma once

#include "reader.h"

#include <string>

reader * createvolumereader(std::wstring const volumename);	// like C:

void freevolumereader(reader * r);