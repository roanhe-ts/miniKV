//
// Created by 何智强 on 2021/10/3.
//

#ifndef MINIKV_CONFIG_H
#define MINIKV_CONFIG_H

#include <vector>
#include "glog/logging.h"
#include "src/Common/Macros.h"

namespace miniKV {

using page_id_t = int32_t;
using frame_id_t = int32_t;
using key_t = int64_t;
using value_t = int32_t;
using values = std::vector<value_t>;

using txn_id_t = int32_t;  // transaction id type

static constexpr int INVALID_PAGE_ID = -1;   // invalid page id
static constexpr int INVALID_TXN_ID = -1;    // invalid transaction id
static constexpr int INVALID_LSN = -1;       // invalid log sequence number
static constexpr int HEADER_PAGE_ID = 0;     // the header page id
static constexpr int PAGE_SIZE = 4096;       // size of a data page in byte
static constexpr int BUFFER_POOL_SIZE = 40;  // size of buffer pool
static constexpr int BUCKET_SIZE = 50;       // size of extendible hash bucket

};  // namespace miniKV

#endif  // MINIKV_CONFIG_H
