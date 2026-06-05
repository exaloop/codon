// Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>

#include <algorithm>
#include <fstream>
#include <gc.h>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#ifndef _WIN32
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "codon/parser/common.h"
#include "codon/util/common.h"
#include "gtest/gtest.h"

TEST(TypeCoreTest, TestName) { ; }
