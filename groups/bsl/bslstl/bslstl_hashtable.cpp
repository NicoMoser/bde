// bslstl_hashtable.cpp                                               -*-C++-*-
#include <bslstl_hashtable.h>

#include <bsls_ident.h>
BSLS_IDENT("$Id$ $CSID$")

#include <bslstl_equalto.h>            // for testing only
#include <bslstl_hash.h>               // for testing only
#include <bslstl_hashtableiterator.h>  // for testing only
#include <bslstl_iterator.h>           // for testing only
#include <bslstl_stdexceptutil.h>


#include <bsls_nativestd.h>

#include <algorithm>         // 'lower_bound'
#include <limits>

namespace BloombergLP
{
namespace bslstl
{

                    // --------------------------
                    // class HashTable_ImpDetails
                    // --------------------------

bslalg::HashTableBucket *HashTable_ImpDetails::defaultBucketAddress()
{
    static bslalg::HashTableBucket s_bucket = {0 , 0};
                                                  // Aggregative initialization
                                                  // of a POD should be thread-
                                                  // safe static initialization

    // These two tests should not be necessary, but will catch corruption in
    // components that try to write to the shared bucket.

    BSLS_ASSERT_SAFE(!s_bucket.first());
    BSLS_ASSERT_SAFE(!s_bucket.last());

    return &s_bucket;
}

native_std::size_t HashTable_ImpDetails::nextPrime(native_std::size_t n)
{   // An abbreviated list of prime numbers in the domain of 32-bit
    // unsigned integers.  Essentially, a subset where each successive
    // element is the next prime after doubling.
    // Note that at least one of these numbers was mis-computed and
    // undershoots, messing up the doubling pattern, not critical while the
    // code remains proof-of-concept code.

    static const native_std::size_t s_primes[] = { 2, 5, 13, 29, 61,
        127, 257, 521, 1049, 2099, 4201, 8419, 16843, 33703, 67409, 134837,
        269513, 539039, 1078081, 2156171, 5312353, 10624709, 21249443,
        42498893, 84997793, 169995589, 339991181, 679982363, 1359964751,
        2719929503u

    };
    static const native_std::size_t s_nPrimes =
                                         sizeof(s_primes)/sizeof(&s_primes[0]);
    static const native_std::size_t *const s_beginPrimes = s_primes;
    static const native_std::size_t *const s_endPrimes = s_primes + s_nPrimes;

    const native_std::size_t *result = native_std::lower_bound(s_beginPrimes,
                                                               s_endPrimes,
                                                               n);
    if (s_endPrimes == result) {
        StdExceptUtil::throwLengthError("HashTable ran out of prime numbers.");
    }

    return *result;
}

size_t HashTable_ImpDetails::growBucketsForLoadFactor(size_t *capacity,
                                                      size_t  minElements,
                                                      size_t  requestedBuckets,
                                                      double  maxLoadFactor)
{
    BSLS_ASSERT_SAFE(  0 != capacity);
    BSLS_ASSERT_SAFE(  0  < minElements);
    BSLS_ASSERT_SAFE(  0  < requestedBuckets);
    BSLS_ASSERT_SAFE(0.0  < maxLoadFactor);

    static const size_t MAX_SIZE_T = native_std::numeric_limits<size_t>::max();
    static const double MAX_AS_DBL = static_cast<double>(MAX_SIZE_T);

    struct Impl {
        // This local utility class provides a couple of methods for converting
        // 'double' values to 'size_t' values, applying different policies in
        // event of an overflow.  This clarifies the main logic of the function
        // rather than placing this logic inline a number of times.

        // CLASS METHODS
        static size_t roundToMax(double d)
            // Return the integer value corresponding to the specified 'd', or
            // the highest unsigned value representable by 'size_t' if 'd' is
            // larger.  The behaviour is undefine unless '0.0 <= d'.
        {
            BSLS_ASSERT_SAFE(0.0 <= d);

            return d < MAX_AS_DBL
                 ? static_cast<size_t>(d)
                 : MAX_SIZE_T;
        }

        static size_t throwIfOverMax(double d)
            // Throw a 'std::length_error' exception if 'd' is  larger than the
            // highest unsigned value representable by 'size_t'.  Return the
            // integer value corresponding to the specified 'd', rounding down.
            // The behaviour is undefine unless '0.0 <= d'.
        {
            BSLS_ASSERT_SAFE(0.0 <= d);

            if (d > MAX_AS_DBL) {
                StdExceptUtil::throwLengthError(
                                           "The number of buckets overflows.");
            }

            return static_cast<size_t>(d);
        }
    };


    // This check is why 'minElements' must be at least one - so that we do not
    // allocate a number of buckets that cannot hold at least one element, and
    // then throw the unexpected 'logic_error' on the first 'insert'.  We make
    // it a pre-condition of the function, as some callers have contextual
    // knowledge that the argument must be non-zero, and so avoid a redundant
    // 'min' call.

    size_t result = native_std::max(
                            requestedBuckets,
                            Impl::throwIfOverMax(minElements / maxLoadFactor));

    result = nextPrime(result);  // throws if too large

    double newCapacity = static_cast<double>(result) * maxLoadFactor;

    while (minElements > newCapacity ) {
        result  = nextPrime(2 * result);  // throws if too large
        newCapacity = static_cast<double>(result) * maxLoadFactor;
    }

    *capacity = Impl::roundToMax(newCapacity);
    return result;
}

}  // close package namespace
}  // close enterprise namespace
// ---------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2012
//      All Rights Reserved.
//      Property of Bloomberg L.P. (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ---------------------------------
