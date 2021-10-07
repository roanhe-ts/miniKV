//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/page/b_plus_tree_leaf_page.cpp
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <sstream>

#include <algorithm>
#include "src/Common/Utils.h"
#include "src/Storage/Page/BPlusTreePage.h"
#include "src/Storage/Page/BPlusTreeLeafPage.h"

namespace miniKV {

/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/

/**
 * Init method after creating a new leaf page
 * Including set page type, set current size to zero, set page id/parent id, set
 * next page id and set max size
 */
    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE::Init(page_id_t page_id, page_id_t parent_id, int max_size) {
        // Default values:
        // parent_id = INVALID_PAGE_ID;
        // max_size = LEAF_PAGE_SIZE.

        SetPageType(IndexPageType::LEAF_PAGE);
        SetSize(0);
        SetPageId(page_id);
        SetParentPageId(parent_id);
        SetNextPageId(INVALID_PAGE_ID);
        SetMaxSize(max_size);
    }

/**
 * Helper methods to set/get next page id
 */
    INDEX_TEMPLATE_ARGUMENTS
            page_id_t B_PLUS_TREE_LEAF_PAGE::GetNextPageId() const { return next_page_id_; }

    INDEX_TEMPLATE_ARGUMENTS
    void B_PLUS_TREE_LEAF_PAGE::SetNextPageId(page_id_t next_page_id) { next_page_id_ = next_page_id; }

/**
 * Helper method to find the first index i so that array[i].first >= key
 * NOTE: This method is only used when generating index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
int B_PLUS_TREE_LEAF_PAGE::KeyIndex(const KeyType &key) const {
    const MappingType *p = std::lower_bound(
            array, array + GetSize(), key,
            [&](const MappingType &item, const KeyType &k) { return item.first < k;});

    return std::distance(array, p);
}

/*
 * Helper method to find and return the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
        KeyType B_PLUS_TREE_LEAF_PAGE::KeyAt(int index) const { return array[index].first; }

/*
 * Helper method to find and return the key & value pair associated with input
 * "index"(a.k.a array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
const MappingType &B_PLUS_TREE_LEAF_PAGE::GetItem(int index) { return array[index]; }

//*****************************************************************************
//* INSERTION
//*****************************************************************************

/*
 * Insert key & value pair into leaf page ordered by key
 * @return  page size after insertion
 */
INDEX_TEMPLATE_ARGUMENTS
int B_PLUS_TREE_LEAF_PAGE::Insert(const KeyType &key, const ValueType &value) {
    /* find insert position [binary search] */
    int insert_position = KeyIndex(key);

    // make room
    for (int i = GetSize() - 1; i >= insert_position; i--) {
        array[i + 1] = array[i];
    }

    // insert key-value pair
    array[insert_position] = MappingType{key, value};

    // update size
    IncreaseSize(1);

    return GetSize();
}

//*****************************************************************************
// SPLIT
//*****************************************************************************

/**
 * Remove half of key & value pairs from this page to "recipient" page
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE::MoveHalfTo(BPlusTreeLeafPage *recipient) {
    // Currently, this is only called when recipient is empty (in Split)

    // Move array[(size+1)/2 : size-1].
    // Number of elements moved: size-1 - (size+1)/2 + 1 = size-(size+1)/2 = size-ceil(size/2) = floor(size/2)
    // After move, this->GetSize() >= recipient->GetSize().
    int move_start = (GetSize() + 1) / 2;
    int num_moved = GetSize() - move_start;
    recipient->CopyNFrom(&array[move_start], num_moved);

    IncreaseSize(-num_moved);
    recipient->IncreaseSize(num_moved);
}

/*
 * Copy starting from items, and copy {size} number of elements into me.
 * [Attention] The caller should update size. This function doesn't.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE::CopyNFrom(MappingType *items, int size) {
if (GetSize() + size > GetMaxSize()) {
throw std::string("CopyNFrom: will overflow page");
}

int old_size = GetSize();
for (int i = 0; i < size; i++) {
array[old_size + i] = items[i];
}
}

//*****************************************************************************
//* LOOKUP
//*****************************************************************************

/*
 * For the given key, check to see whether it exists in the leaf page. If it
 * does, then store its corresponding value in input "value" and return true.
 * If the key does not exist, then return false
 */
INDEX_TEMPLATE_ARGUMENTS
bool B_PLUS_TREE_LEAF_PAGE::Lookup(const KeyType &key, ValueType *value) const {
    /* Linear search */
    // for (int i = 0; i < GetSize(); i++) {
    //   if (comparator(array[i].first, key) == 0) {
    //     if (value != nullptr) {
    //       *value = array[i].second;
    //     }
    //     return true;
    //   }
    // }
    // return false;

    /* Binary search */
    int pos = KeyIndex(key);
    if (pos < GetSize() && (array[pos].first == key)) {
        if (value != nullptr) {
            *value = array[pos].second;
        }
        return true;
    }
    return false;
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * First look through leaf page to see whether delete key exist or not. If
 * exist, perform deletion, otherwise return immediately.
 * NOTE: store key&value pair continuously after deletion
 * @return   page size after deletion
 */
INDEX_TEMPLATE_ARGUMENTS
int B_PLUS_TREE_LEAF_PAGE::RemoveAndDeleteRecord(const KeyType &key) {
    /* Binary search */
    int pos = KeyIndex(key);
    if (pos < GetSize() && (array[pos].first == key)) {
        for (int j = pos + 1; j < GetSize(); j++) {
            array[j - 1] = array[j];
        }
        IncreaseSize(-1);
    }
    return GetSize();
}

//*****************************************************************************
//* MERGE
//*****************************************************************************

/*
 * Remove all of key & value pairs from this page to "recipient" page. Don't forget
 * to update the next_page id in the sibling page.
 *
 * This function updates the size of two nodes.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE::MoveAllTo(BPlusTreeLeafPage *recipient) {
    // Assume we are moving to the left sibling
    // If we are moving to the right sibling (recipient), we need to update the left sibling
    int sz = GetSize();

    recipient->CopyNFrom(array, sz);
    recipient->SetNextPageId(GetNextPageId());

    recipient->IncreaseSize(sz);
    IncreaseSize(-sz);
}

/*****************************************************************************
 * REDISTRIBUTE
 *****************************************************************************/
/*
 * Remove the first key & value pair from this page to "recipient" page.
 * This function updates the size for both nodes.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE::MoveFirstToEndOf(BPlusTreeLeafPage *recipient) {
    // Copy element to recipient's last position
    recipient->CopyLastFrom(array[0]);

    // Remove first element from my array
    for (int i = 1; i < GetSize(); i++) {
        array[i - 1] = array[i];
    }

    // update size for both nodes
    IncreaseSize(-1);
    recipient->IncreaseSize(1);
}

/*
 * Remove the last key & value pair from this page to "recipient" page.
 * This function updates the size for both nodes.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE::MoveLastToFrontOf(BPlusTreeLeafPage *recipient) {
    // Copy element to recipient's front position
    recipient->CopyFirstFrom(array[GetSize() - 1]);

    // No need to remove the last element. The caller can just update the size.

    // update size for both nodes
    IncreaseSize(-1);
    recipient->IncreaseSize(1);
}

/*
 * Copy the item into the end of my item list. (Append item to my array)
 * [Attention] This function doesn't update size.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE::CopyLastFrom(const MappingType &item) { array[GetSize()] = item; }

/*
 * Insert item at the front of my items. Move items accordingly.
 * [Attention] this function doesn't update size.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE::CopyFirstFrom(const MappingType &item) {
    // make space
    for (int i = GetSize() - 1; i >= 0; i--) {
        array[i + 1] = array[i];
    }

    // copy element
    array[0] = item;
}

template class BPlusTreeLeafPage<key_t, value_t>;
}
