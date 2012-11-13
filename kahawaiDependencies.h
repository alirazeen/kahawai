//Library includes

#ifndef KAHAWAI_DEP
#define KAHAWAI_DEP

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

extern "C" {
#include "inttypes.h"
#include "x264.h"
#include <libswscale/swscale.h>
}

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#endif

#include "utils.h"
#include "KahawaiTypes.h"

#endif