#include "cuckoo.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#ifndef CUCKOO_MALLOC
#define CUCKOO_MALLOC malloc
#define CUCKOO_CALLOC calloc
#define CUCKOO_REALLOC realloc
#define CUCKOO_FREE free
#endif

static int CuckooFilter_Grow(CuckooFilter *filter);

static int isPower2(uint32_t num) {
    return (num & (num - 1)) == 0 && num != 0;
}

static size_t getNextN2(size_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

int CuckooFilter_Init(CuckooFilter *filter, size_t capacity, uint16_t bucketSize, uint16_t maxIterations, uint16_t expansion) {
    //assert(capacity >= bucketSize);
    assert(isPower2(expansion));

    memset(filter, 0, sizeof(*filter));
    filter->expansion = expansion;
    filter->bucketSize = bucketSize;
    filter->maxIterations = maxIterations;
    filter->numBuckets = getNextN2(capacity / bucketSize);
    if (filter->numBuckets == 0) {
        filter->numBuckets = 1; 
    }
    assert(isPower2(filter->numBuckets));   

    if (CuckooFilter_Grow(filter) != 0) {
        return -1;
    }
    return 0;
}

void CuckooFilter_Free(CuckooFilter *filter) {
    for (size_t ii = 0; ii < filter->numFilters; ++ii) {
        CUCKOO_FREE(filter->filters[ii].data);
    }
    CUCKOO_FREE(filter->filters);
}

static int CuckooFilter_Grow(CuckooFilter *filter) {
    size_t *numFilters = &filter->numFilters;
    SubCF *filtersArray = (SubCF *)CUCKOO_REALLOC(filter->filters,
                           sizeof(*filtersArray) * (*numFilters + 1));

    if (!filtersArray) {
        return -1;
    }
    SubCF *currentFilter = filtersArray + *numFilters;

    size_t growth = pow(filter->expansion, *numFilters);
    currentFilter->bucketSize = filter->bucketSize;
    currentFilter->numBuckets = filter->numBuckets * growth;
    currentFilter->data = CUCKOO_CALLOC(currentFilter->numBuckets * filter->bucketSize,
                                        sizeof(CuckooBucket));
    if (!currentFilter->data) {
        return -1;
    }

    (*numFilters)++;
    filter->filters = filtersArray;
    return 0;
}

typedef struct {
    size_t i1;
    size_t i2;
    CuckooFingerprint fp;
} LookupParams;

static size_t getAltIndex(CuckooFingerprint fp, size_t index) {
    return ((uint32_t)(index ^ ((uint32_t)fp * 0x5bd1e995)));
}

static void getLookupParams(CuckooHash hash, LookupParams *params) {
    // Truncate the hash to uint8
    if ((params->fp = (hash >> 24)) == CUCKOO_NULLFP) {
        params->fp = 7;
    }

    params->i1 = hash;
    params->i2 = getAltIndex(params->fp, params->i1);
    // assert(getAltIndex(params->fp, params->i2, numBuckets) == params->i1);
}

static uint32_t getLocSubCF(uint32_t num, const SubCF *subCF) {
    return (num % subCF->numBuckets) * subCF->bucketSize;
}

static uint8_t *Bucket_Find(CuckooBucket bucket, uint16_t bucketSize, CuckooFingerprint fp) {
    for (uint16_t ii = 0; ii < bucketSize; ++ii) {
        if (bucket[ii] == fp) {
            return bucket + ii;
        }
    }
    return NULL;
}

static int Filter_Find(const SubCF *filter, const LookupParams *params) {
    uint8_t bucketSize = filter->bucketSize;
    uint64_t loc1 = getLocSubCF(params->i1, filter);
    uint64_t loc2 = getLocSubCF(params->i2, filter);
    return Bucket_Find(&filter->data[loc1], bucketSize, params->fp) != NULL ||
           Bucket_Find(&filter->data[loc2], bucketSize, params->fp) != NULL;
}

static int Bucket_Delete(CuckooBucket bucket, uint16_t bucketSize, CuckooFingerprint fp) {
    for (uint16_t ii = 0; ii < bucketSize; ii++) {
        if (bucket[ii] == fp) {
            bucket[ii] = CUCKOO_NULLFP;
            return 1;
        }
    }
    return 0;
}

static int Filter_Delete(const SubCF *filter, const LookupParams *params) {
    uint8_t bucketSize = filter->bucketSize;
    uint64_t loc1 = getLocSubCF(params->i1, filter);
    uint64_t loc2 = getLocSubCF(params->i2, filter);
    return Bucket_Delete(&filter->data[loc1], bucketSize, params->fp) ||
           Bucket_Delete(&filter->data[loc2], bucketSize, params->fp);
}

static int CuckooFilter_CheckFP(const CuckooFilter *filter, const LookupParams *params) {
    for (size_t ii = 0; ii < filter->numFilters; ++ii) {
        if (Filter_Find(&filter->filters[ii], params)) {
            return 1;
        }
    }
    return 0;
}

int CuckooFilter_Check(const CuckooFilter *filter, CuckooHash hash) {
    LookupParams params;
    getLookupParams(hash, &params);
    return CuckooFilter_CheckFP(filter, &params);
}

static size_t bucketCount(const CuckooBucket bucket, uint16_t bucketSize, CuckooFingerprint fp) {
    size_t ret = 0;
    for (uint16_t ii = 0; ii < bucketSize; ++ii) {
        if (bucket[ii] == fp) {
            ret++;
        }
    }
    return ret;
}

//static size_t filterCount(const CuckooBucket *filter, uint16_t bucketSize, const LookupParams *params) {
static size_t filterCount(const SubCF *filter, const LookupParams *params) {
    uint8_t bucketSize = filter->bucketSize;
    uint64_t loc1 = getLocSubCF(params->i1, filter);
    uint64_t loc2 = getLocSubCF(params->i2, filter);

    size_t ret = bucketCount(&filter->data[loc1], bucketSize, params->fp);
  /*  if (params->i1 != params->i2) { // TODO: check if feasible*/
        ret += bucketCount(&filter->data[loc2], bucketSize, params->fp);
    //}

    return ret;
}

size_t CuckooFilter_Count(const CuckooFilter *filter, CuckooHash hash) {
    LookupParams params;
    getLookupParams(hash, &params);
    size_t ret = 0;
    for (size_t ii = 0; ii < filter->numFilters; ++ii) {
        ret += filterCount(&filter->filters[ii], &params);
    }
    return ret;
}

int CuckooFilter_Delete(CuckooFilter *filter, CuckooHash hash) {
    LookupParams params;
    getLookupParams(hash, &params);
    for (size_t ii = 0; ii < filter->numFilters; ++ii) {
        if (Filter_Delete(&filter->filters[ii], &params)) {
            filter->numItems--;
            filter->numDeletes++;
            if (filter->numFilters > 1 && filter->numDeletes > (double)filter->numItems * 0.10) {
                CuckooFilter_Compact(filter);
            }
            return 1;
        }
    }
    return 0;
}

static uint8_t *Bucket_FindAvailable(CuckooBucket bucket, uint16_t bucketSize) {
    for (uint16_t ii = 0; ii < bucketSize; ++ii) {
        if (bucket[ii] == CUCKOO_NULLFP) {
            return &bucket[ii];
        }
    }
    return NULL;
}

static uint8_t *Filter_FindAvailable(SubCF *filter, const LookupParams *params) {
    uint8_t *slot;
    uint8_t bucketSize = filter->bucketSize;
    uint64_t loc1 = getLocSubCF(params->i1, filter);
    uint64_t loc2 = getLocSubCF(params->i2, filter);
    if ((slot = Bucket_FindAvailable(&filter->data[loc1], bucketSize)) ||
        (slot = Bucket_FindAvailable(&filter->data[loc2], bucketSize))) {
        return slot;
    }
    return NULL;
}

/**** Unused and unfixed for SubCF struct
static uint8_t *Filter_FindAvailableDbg(CuckooBucket *filter, uint16_t bucketSize,
                                        const LookupParams *params, size_t *newIx) {
    uint8_t *slot;
    if ((slot = Bucket_FindAvailable(filter[params->i1 * bucketSize], bucketSize))) {
        *newIx = params->i1;
    } else if ((slot = Bucket_FindAvailable(filter[params->i2 * bucketSize], bucketSize))) {
        *newIx = params->i2;
    } else {
        *newIx = -1;
    }
    return slot;
}

static uint8_t *Filter_FindUnique(CuckooBucket bucket, size_t index, uint16_t bucketSize,
                                  CuckooFingerprint fp, CuckooInsertStatus *err) {
    uint8_t *firstEmpty = NULL;
    bucket += (index * bucketSize);
    for (uint16_t ii = 0; ii < bucketSize; ++ii) {
        if (bucket[ii] == fp) {
            *err = CuckooInsert_Exists;
            return NULL;
        } else if (firstEmpty == 0 && bucket[ii] == 0) {
            firstEmpty = bucket + ii;
        }
    }
    if (firstEmpty == NULL) {
        *err = CuckooInsert_NoSpace;
    }
    return firstEmpty;
}*/

static CuckooInsertStatus Filter_KOInsert(CuckooFilter *filter, SubCF *curFilter, 
                                          const LookupParams *params);

static CuckooInsertStatus CuckooFilter_InsertFP(CuckooFilter *filter, const LookupParams *params) {
    for(size_t ii = filter->numFilters; ii > 0; --ii) {
        uint8_t *slot = Filter_FindAvailable(&filter->filters[ii - 1], params);
        if (slot) {
            *slot = params->fp;
            filter->numItems++;
            return CuckooInsert_Inserted;
        }
    }

    // No space. Time to evict!
    CuckooInsertStatus status =
        Filter_KOInsert(filter, &filter->filters[filter->numFilters - 1], params);
    if (status == CuckooInsert_Inserted) {
        filter->numItems++;
        return status;
    } else if(status == CuckooInsert_NoSpace) {
        if (CuckooFilter_Grow(filter) != 0) {
            printf("Grow after max iterations\n");
            return CuckooInsert_NoSpace;
        }
    }
    // Try to insert the filter again
    return CuckooFilter_InsertFP(filter, params);
}

CuckooInsertStatus CuckooFilter_Insert(CuckooFilter *filter, CuckooHash hash) {
    LookupParams params;
    getLookupParams(hash, &params);
    return CuckooFilter_InsertFP(filter, &params);
}

CuckooInsertStatus CuckooFilter_InsertUnique(CuckooFilter *filter, CuckooHash hash) {
    LookupParams params;
    getLookupParams(hash, &params);
    if (CuckooFilter_CheckFP(filter, &params)) {
        return CuckooInsert_Exists;
    }
    return CuckooFilter_InsertFP(filter, &params);
}

static void swapFPs(uint8_t *a, uint8_t *b) {
    uint8_t temp = *a;
    *a = *b;
    *b = temp;
}

static CuckooInsertStatus Filter_KOInsert(CuckooFilter *filter, SubCF *curFilter, 
                                          const LookupParams *params) {
    uint16_t maxIterations = filter->maxIterations;
    uint32_t numBuckets = curFilter->numBuckets;
    uint16_t bucketSize = filter->bucketSize;
    CuckooFingerprint fp = params->fp;

    size_t counter = 0;
    size_t victimIx =  0;
//    size_t ii = getLocSubCF(params->i1, curFilter);
    size_t ii = params->i1 % numBuckets;
    // params = NULL; // Don't reference 'params' again!

    while (counter++ < maxIterations) {
        uint8_t *bucket = &curFilter->data[ii * bucketSize];
        //printf("fp %d victim %d \t", fp, *(bucket + victimIx));
        swapFPs(bucket + victimIx, &fp);
        //printf("fp %d victim %d \t", fp, *(bucket + victimIx));
        //printf("first loc %lu victim index %lu\n", ii / bucketSize, victimIx);
        ii = getAltIndex(fp, ii) % numBuckets;
        // Insert the new item in potentially the same bucket
        uint8_t *empty = Bucket_FindAvailable(&curFilter->data[ii * bucketSize], bucketSize);
        if (empty) {
            // printf("Found slot. Bucket[%lu], Pos=%lu\n", ii, empty - curFilter[ii]);
            // printf("Old FP Value: %d\n", *empty);
            // printf("Setting FP: %p\n", empty);
            *empty = fp;
            return CuckooInsert_Inserted;
        }
        victimIx = (victimIx + 1) % bucketSize;
    }

    counter = 0;
    // If we weren't able to insert, we must roll back for case of exponential filter growth
    while (counter++ < maxIterations) {
        //printf("reinsert %lu\n", counter);
        victimIx = (victimIx + bucketSize - 1) % bucketSize;
        ii = getAltIndex(fp, ii) % numBuckets;
        uint8_t *bucket = &curFilter->data[ii * bucketSize];
        //printf("Extraction fp %d victim %d\t", fp, *(bucket + victimIx));
        swapFPs(bucket + victimIx, &fp);
        //printf("fp %d victim %d *** loc %lu vicIdx %lu\n", fp, *(bucket + victimIx), ii / bucketSize, victimIx);
    }

    return CuckooInsert_NoSpace;
}

#define RELOC_EMPTY 0
#define RELOC_OK 1
#define RELOC_FAIL -1

/**
 * Attempt to move a slot from one bucket to another filter
 */
static int relocateSlot(CuckooFilter *cf, CuckooBucket bucket, size_t filterIx, size_t bucketIx,
                        size_t slotIx) {
    LookupParams params = { 0 };
    if ((params.fp = bucket[slotIx]) == CUCKOO_NULLFP) {
        // Nothing in this slot.
        return RELOC_EMPTY;
    }

    // Because We try to insert in sub filter with less or equal number of
    // buckets, our current fingerprint is sufficient
    params.i1 = bucketIx;
    params.i2 = getAltIndex(params.fp, bucketIx);

    // Look at all the prior filters and attempt to find a home
    for (size_t ii = 0; ii < filterIx; ++ii) {
        uint8_t *slot = Filter_FindAvailable(&cf->filters[ii], &params);
        if (slot) {
            *slot = params.fp;
            bucket[slotIx] = CUCKOO_NULLFP;
            return RELOC_OK;
        }
    }
    return RELOC_FAIL;
}

/**
 * Attempt to strip a single filter moving it down a slot
 */
static size_t CuckooFilter_CompactSingle(CuckooFilter *cf, size_t filterIx) {
    MyCuckooBucket *filter = cf->filters[filterIx].data;
    int dirty = 0;
    size_t numRelocs = 0;

    for (size_t bucketIx = 0; bucketIx < cf->numBuckets; ++bucketIx) {
        for (size_t slotIx = 0; slotIx < cf->bucketSize; ++slotIx) {
            int status = relocateSlot(cf, &filter[bucketIx * cf->bucketSize], filterIx, bucketIx, slotIx);
            if (status == RELOC_FAIL) {
                dirty = 1;
            } else if (status == RELOC_OK) {
                numRelocs++;
            }
        }
    }
    if (!dirty) {
        CUCKOO_FREE(filter);
        cf->numFilters--;
    }
    return numRelocs;
}

size_t CuckooFilter_Compact(CuckooFilter *cf) {
    size_t ret = 0;
    for (size_t ii = cf->numFilters; ii > 1; --ii) {
        ret += CuckooFilter_CompactSingle(cf, ii - 1);
    }
    cf->numDeletes = 0;
    return ret;
}

/* CF.DEBUG uses another function
void CuckooFilter_GetInfo(const CuckooFilter *cf, CuckooHash hash, CuckooKey *out) {
    LookupParams params;
    getLookupParams(hash, cf->numBuckets, &params);
    out->fp = params.fp;
    out->i1 = params.i1;
    out->i2 = params.i2;
    assert(getAltIndex(params.fp, out->i1, cf->numBuckets) == out->i2);
    assert(getAltIndex(params.fp, out->i2, cf->numBuckets) == out->i1);
}*/