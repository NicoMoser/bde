// baejsn_decoder.h                                                   -*-C++-*-
#ifndef INCLUDED_BAEJSN_DECODER
#define INCLUDED_BAEJSN_DECODER

#ifndef INCLUDED_BDES_IDENT
#include <bdes_ident.h>
#endif
BDES_IDENT("$Id: $")

//@PURPOSE: Provide a JSON decoder for 'bdeat' compatible types.
//
//@CLASSES:
//  baejsn_Decoder: JSON decoder for 'bdeat' compliant types
//
//@SEE_ALSO: baejsn_encoder, baejsn_parserutil, baejsn_parser
//
//@AUTHOR: Raymond Chiu (schiu49), Rohan Bhindwale (rbhindwa)
//
//@DESCRIPTION: This component provides utility functions for decoding a JSON
// string.

#ifndef INCLUDED_BAESCM_VERSION
#include <baescm_version.h>
#endif

#ifndef INCLUDED_BAEJSN_DECODEROPTIONS
#include <baejsn_decoderoptions.h>
#endif

#ifndef INCLUDED_BAEJSN_PARSERUTIL
#include <baejsn_parserutil.h>
#endif

#ifndef INCLUDED_BAEJSN_PARSER
#include <baejsn_parser.h>
#endif

#ifndef INCLUDED_BDEAT_CHOICEFUNCTIONS
#include <bdeat_choicefunctions.h>
#endif

#ifndef INCLUDED_BDEAT_CUSTOMIZEDTYPEFUNCTIONS
#include <bdeat_customizedtypefunctions.h>
#endif

#ifndef INCLUDED_BDEAT_ENUMFUNCTIONS
#include <bdeat_enumfunctions.h>
#endif

#ifndef INCLUDED_BDEAT_SEQUENCEFUNCTIONS
#include <bdeat_sequencefunctions.h>
#endif

#ifndef INCLUDED_BDEAT_TYPECATEGORY
#include <bdeat_typecategory.h>
#endif

#ifndef INCLUDED_BDEAT_VALUETYPEFUNCTIONS
#include <bdeat_valuetypefunctions.h>
#endif

#ifndef INCLUDED_BSLMF_ASSERT
#include <bslmf_assert.h>
#endif

#ifndef INCLUDED_BSLS_ASSERT
#include <bsls_assert.h>
#endif

#ifndef INCLUDED_BSLS_TYPES
#include <bsls_types.h>
#endif

#ifndef INCLUDED_BSL_SSTREAM
#include <bsl_sstream.h>
#endif

#ifndef INCLUDED_BSL_STREAMBUF
#include <bsl_streambuf.h>
#endif

#ifndef INCLUDED_BSL_STRING
#include <bsl_string.h>
#endif

namespace BloombergLP {

                        // ====================
                        // class baejsn_Decoder
                        // ====================

class baejsn_Decoder {

    // DATA
    bsl::ostringstream           d_logStream;         // stream to store error
                                                      // message
    baejsn_Parser                d_parser;            // JSON parser
    bsl::string                  d_elementName;       // current element name
    const baejsn_DecoderOptions *d_decoderOptions_p;  // decoder options
    int                          d_currentDepth;      // current decoding depth

    // FRIENDS
    friend struct baejsn_Decoder_DecodeImpProxy;
    friend struct baejsn_Decoder_ElementVisitor;

    // PRIVATE MANIPULATORS
    int decodeBinaryArray(bsl::vector<char> *value);

    template <typename TYPE>
    int decodeImp(TYPE *value, bdeat_TypeCategory::DynamicType);

    template <typename TYPE>
    int decodeImp(TYPE *value, bdeat_TypeCategory::Sequence);

    template <typename TYPE>
    int decodeImp(TYPE *value, bdeat_TypeCategory::Choice);

    template <typename TYPE>
    int decodeImp(TYPE *value, bdeat_TypeCategory::Enumeration);

    template <typename TYPE>
    int decodeImp(TYPE *value, bdeat_TypeCategory::CustomizedType);

    template <typename TYPE>
    int decodeImp(TYPE *value, bdeat_TypeCategory::Simple);

    template <typename TYPE>
    int decodeImp(TYPE *value, bdeat_TypeCategory::Array);

    template <typename TYPE>
    int decodeImp(TYPE *value, bdeat_TypeCategory::NullableValue);

    template <typename TYPE, typename ANY_CATEGORY>
    int decodeImp(TYPE *object, ANY_CATEGORY category);

  public:
    // CREATORS
    explicit baejsn_Decoder(bslma_Allocator *basicAllocator = 0);
        // Construct a decoder object using the optionally specified
        // 'basicAllocator'.  If 'basicAllocator' is 0, the default allocator
        // is used.
        //
        // DEPRECATED ?

    baejsn_Decoder(const baejsn_DecoderOptions *options,
                   bslma_Allocator             *basicAllocator = 0);
        // Construct a decoder object using the specified 'options'.
        // Optionally specify 'basicAllocator' for allocating memory.  If
        // 'basicAllocator' is 0, the default allocator is used.

    // MANIPULATORS
    template <typename TYPE>
    int decode(bsl::streambuf *streamBuf, TYPE *variable);
        // Decode an object of (template parameter) 'TYPE' from the specified
        // 'streamBuf' and load the result into the specified modifiable
        // 'variable'.  Return 0 on success, and a non-zero value otherwise.

    template <typename TYPE>
    int decode(bsl::istream& stream, TYPE *variable);
        // Decode an object of (template parameter) 'TYPE' from the specified
        // 'stream' and load the result into the specified modifiable
        // 'variable'.  Return 0 on success, and a non-zero value otherwise.
        // Note that 'stream' will be invalidated if the decoding fails.

    // ACCESSORS
    bsl::string loggedMessages() const;
        // Return a string containing any error, warning, or trace messages
        // that were logged during the last call to the 'decode' method.  The
        // log is reset each time 'decode' is called.
};

                 // ====================================
                 // struct baejsn_Decoder_ElementVisitor
                 // ====================================

struct baejsn_Decoder_ElementVisitor {
    // COMPONENT-PRIVATE CLASS.  DO NOT USE OUTSIDE OF THIS COMPONENT.

    // DATA
    baejsn_Decoder *d_decoder_p;

    // CREATORS
    // Creators have been omitted to allow simple static initialization of
    // this struct.

    // MANIPULATORS
    template <typename TYPE>
    int operator()(TYPE *value);

    template <typename TYPE, typename INFO>
    int operator()(TYPE *value, const INFO&);
};

                 // ====================================
                 // struct baejsn_Decoder_DecodeImpProxy
                 // ====================================

struct baejsn_Decoder_DecodeImpProxy {
    // COMPONENT-PRIVATE CLASS.  DO NOT USE OUTSIDE OF THIS COMPONENT.

    // DATA
    baejsn_Decoder *d_decoder_p;

    // CREATORS
    // Creators have been omitted to allow simple static initialization of
    // this struct.

    // MANIPULATORS
    template <typename TYPE>
    int operator()(TYPE *, bslmf_Nil);

    template <typename TYPE, typename ANY_CATEGORY>
    int operator()(TYPE *object, ANY_CATEGORY category);
};

// ============================================================================
//                      INLINE FUNCTION DEFINITIONS
// ============================================================================

                        // --------------------
                        // class baejsn_Decoder
                        // --------------------

// PRIVATE MANIPULATORS
template <typename TYPE>
inline
int baejsn_Decoder::decodeImp(TYPE *value, bdeat_TypeCategory::DynamicType)
{
    baejsn_Decoder_DecodeImpProxy proxy = { this };
    return bdeat_TypeCategoryUtil::manipulateByCategory(value, proxy);
}

template <typename TYPE>
int baejsn_Decoder::decodeImp(TYPE *value, bdeat_TypeCategory::Sequence)
{
    ++d_currentDepth;
    if (d_decoderOptions_p
     && d_decoderOptions_p->maxDepth() > d_currentDepth) {
        d_logStream << "Maximum allowed decoding depth reached: "
                    << d_currentDepth << "\n";
        return -1;                                                    // RETURN
    }

    if (baejsn_Parser::BAEJSN_START_OBJECT != d_parser.tokenType()) {
        d_logStream << "Could not decode sequence, missing starting '{'\n";
        return -1;                                                    // RETURN
    }

    int rc = d_parser.advanceToNextToken();
    if (rc) {
        d_logStream << "Could not decode sequence, "
                    << "error reading token after '{'\n";
        return -1;                                                    // RETURN
    }

    while (baejsn_Parser::BAEJSN_ELEMENT_NAME == d_parser.tokenType()) {
        bslstl::StringRef elementName;
        rc = d_parser.value(&elementName);
        if (rc) {
            d_logStream << "Error reading attribute name after '{'\n";
            return -1;                                                // RETURN
        }

        d_elementName = elementName;

        rc = d_parser.advanceToNextToken();
        if (rc) {
            d_logStream << "Error reading value for attribute '"
                        << d_elementName << "' \n";
            return -1;                                                // RETURN
        }

        baejsn_Decoder_ElementVisitor visitor = { this };
        if (0 != bdeat_SequenceFunctions::manipulateAttribute(
                                   value,
                                   visitor,
                                   d_elementName.data(),
                                   static_cast<int>(d_elementName.length()))) {
            d_logStream << "Could not decode sequence, error decoding element "
                        << "or bad element name '"
                        << d_elementName << "' \n";
            return -1;                                                // RETURN
        }

        rc = d_parser.advanceToNextToken();
        if (rc) {
            d_logStream << "Could not decode sequence, error reading token "
                        << "after value for attribute '"
                        << d_elementName << "' \n";
            return -1;                                                // RETURN
        }
    }

    if (baejsn_Parser::BAEJSN_END_OBJECT != d_parser.tokenType()) {
        d_logStream << "Could not decode sequence, "
                    << "missing terminator '}' or seperator ','\n";
        return -1;                                                    // RETURN
    }
    return 0;
}

template <typename TYPE>
int baejsn_Decoder::decodeImp(TYPE *value, bdeat_TypeCategory::Choice)
{
    ++d_currentDepth;
    if (d_decoderOptions_p
     && d_decoderOptions_p->maxDepth() > d_currentDepth) {
        d_logStream << "Maximum allowed decoding depth reached: "
                    << d_currentDepth << "\n";
        return -1;                                                    // RETURN
    }

    if (baejsn_Parser::BAEJSN_START_OBJECT != d_parser.tokenType()) {
        d_logStream << "Could not decode choice, missing starting {\n";
        return -1;                                                    // RETURN
    }

    int rc = d_parser.advanceToNextToken();
    if (rc) {
        d_logStream << "Could not decode choice, "
                    << "error reading token after {\n";
        return -1;                                                    // RETURN
    }

    if (baejsn_Parser::BAEJSN_ELEMENT_NAME == d_parser.tokenType()) {
        bslstl::StringRef selectionName;
        rc = d_parser.value(&selectionName);
        if (rc) {
            d_logStream << "Error reading selection name after '{'\n";
            return -1;                                                // RETURN
        }

        if (0 != bdeat_ChoiceFunctions::makeSelection(
                                   value,
                                   selectionName.data(),
                                   static_cast<int>(selectionName.length()))) {
            d_logStream << "Could not decode choice, bad selection name '"
                        << selectionName << "' \n";
            return -1;                                                // RETURN
        }

        rc = d_parser.advanceToNextToken();
        if (rc) {
            d_logStream << "Could not decode choice, error reading value \n";
            return -1;                                                // RETURN
        }

        baejsn_Decoder_ElementVisitor visitor = { this };
        if (0 != bdeat_ChoiceFunctions::manipulateSelection(value, visitor)) {
            d_logStream << "Could not decode choice, selection "
                        << "was not decoded\n";
            return -1;                                                // RETURN
        }

        rc = d_parser.advanceToNextToken();
        if (rc) {
            d_logStream << "Could not decode sequence, error reading token "
                        << "after value for selection \n";
            return -1;                                                // RETURN
        }
    }

    if (baejsn_Parser::BAEJSN_END_OBJECT != d_parser.tokenType()) {
        d_logStream << "Could not decode choice, "
                    << "missing terminator '}'\n";
        return -1;                                                    // RETURN
    }
    return 0;
}

template <typename TYPE>
inline
int baejsn_Decoder::decodeImp(TYPE *value, bdeat_TypeCategory::Enumeration)
{
    enum { BAEJSN_MIN_ENUM_STRING_LENGTH = 2 };

    if (baejsn_Parser::BAEJSN_ELEMENT_VALUE != d_parser.tokenType()) {
        d_logStream << "Error reading enumeration value\n";
        return -1;                                                    // RETURN
    }

    bslstl::StringRef dataValue;
    int rc = d_parser.value(&dataValue);
    if (rc
     || dataValue.length() <= BAEJSN_MIN_ENUM_STRING_LENGTH
     || '"'                != dataValue[0]
     || '"'                != dataValue[dataValue.length() - 1]) {
        d_logStream << "Error reading enumeration value\n";
        return -1;                                                    // RETURN
    }

    dataValue.assign(dataValue.begin() + 1, dataValue.end() - 1);
    rc = bdeat_EnumFunctions::fromString(value,
                                         dataValue.data(),
                                         static_cast<int>(dataValue.length()));

    if (rc) {
        d_logStream << "Could not decode Enum String, value not allowed \""
                    << dataValue << "\"\n";
    }
    return rc;
}

template <typename TYPE>
int baejsn_Decoder::decodeImp(TYPE *value, bdeat_TypeCategory::CustomizedType)
{
    if (baejsn_Parser::BAEJSN_ELEMENT_VALUE != d_parser.tokenType()) {
        d_logStream << "Error reading customized type value\n";
        return -1;                                                    // RETURN
    }

    bslstl::StringRef dataValue;
    int rc = d_parser.value(&dataValue);
    if (rc) {
        d_logStream << "Error reading customized type value\n";
        return -1;                                                    // RETURN
    }

    typename bdeat_CustomizedTypeFunctions::BaseType<TYPE>::Type valueBaseType;

    rc = baejsn_ParserUtil::getValue(&valueBaseType, dataValue);
    if (rc) {
        d_logStream << "Could not decode Enum Customized, "
                    << "value not allowed \"" << valueBaseType << "\"\n";
        return -1;                                                    // RETURN
    }

    rc = bdeat_CustomizedTypeFunctions::convertFromBaseType(value,
                                                            valueBaseType);
    if (rc) {
        d_logStream << "Could not decode Enum Customized, "
                    << "value not allowed \"" << valueBaseType << "\"\n";
    }
    return rc;
}

template <typename TYPE>
int baejsn_Decoder::decodeImp(TYPE *value, bdeat_TypeCategory::Simple)
{
    if (baejsn_Parser::BAEJSN_ELEMENT_VALUE != d_parser.tokenType()) {
        d_logStream << "Error reading simple value\n";
        return -1;                                                    // RETURN
    }

    bslstl::StringRef dataValue;
    int rc = d_parser.value(&dataValue);
    if (rc) {
        d_logStream << "Error reading simple value\n";
        return -1;                                                    // RETURN
    }

    return baejsn_ParserUtil::getValue(value, dataValue);
}

template <>
inline
int baejsn_Decoder::decodeImp(bsl::vector<char> *value,
                              bdeat_TypeCategory::Array)
{
    return decodeBinaryArray(value);
}

template <typename TYPE>
int baejsn_Decoder::decodeImp(TYPE *value, bdeat_TypeCategory::Array)
{
    if (baejsn_Parser::BAEJSN_START_ARRAY != d_parser.tokenType()) {
        d_logStream << "Could not decode vector, missing start [\n";
        return -1;                                                    // RETURN
    }

    int rc = d_parser.advanceToNextToken();
    if (rc) {
        return -1;                                                    // RETURN
    }

    int i = 0;
    while (baejsn_Parser::BAEJSN_END_ARRAY != d_parser.tokenType()) {
        // TBD: What about vector<vector<TYPE> > ?

        if (baejsn_Parser::BAEJSN_ELEMENT_VALUE == d_parser.tokenType()
         || baejsn_Parser::BAEJSN_START_OBJECT  == d_parser.tokenType()) {

            ++i;
            bdeat_ArrayFunctions::resize(value, i);

            baejsn_Decoder_ElementVisitor visitor = { this };
            if (0 != bdeat_ArrayFunctions::manipulateElement(value,
                                                             visitor,
                                                             i - 1)) {
                d_logStream << "Error adding element '" << i - 1 << "'\n";
                return -1;                                            // RETURN
            }

            rc = d_parser.advanceToNextToken();
            if (rc) {
                d_logStream << "Error reading token after element " << i - 1
                            << " value\n";
                return -1;                                            // RETURN
            }
        }
    }

    if (baejsn_Parser::BAEJSN_END_ARRAY != d_parser.tokenType()) {
        d_logStream << "Could not decode vector, missing end ]\n";
        return -1;                                                    // RETURN
    }

    return 0;
}

template <typename TYPE>
int baejsn_Decoder::decodeImp(TYPE *value, bdeat_TypeCategory::NullableValue)
{
    enum { BAEJSN_NULL_VALUE_LENGTH = 4 };

    if (baejsn_Parser::BAEJSN_ELEMENT_VALUE == d_parser.tokenType()) {
        bslstl::StringRef dataValue;
        int rc = d_parser.value(&dataValue);
        if (rc) {
            return -1;                                                // RETURN
        }

        if (BAEJSN_NULL_VALUE_LENGTH == dataValue.length()
         && 0 == bsl::strcmp("null", dataValue.data())) {
            return 0;                                                 // RETURN
        }
    }

    bdeat_NullableValueFunctions::makeValue(value);

    baejsn_Decoder_ElementVisitor visitor = { this };
    return bdeat_NullableValueFunctions::manipulateValue(value, visitor);
}

template <typename TYPE, typename ANY_CATEGORY>
inline
int baejsn_Decoder::decodeImp(TYPE *, ANY_CATEGORY)
{
    BSLS_ASSERT_OPT(!"Unreachable");

    return -1;
}

// CREATORS
inline
baejsn_Decoder::baejsn_Decoder(bslma_Allocator *basicAllocator)
: d_logStream(basicAllocator)
, d_parser(basicAllocator)
, d_elementName(basicAllocator)
, d_decoderOptions_p(0)
, d_currentDepth(0)
{
}

inline
baejsn_Decoder::baejsn_Decoder(const baejsn_DecoderOptions *decoderOptions,
                               bslma_Allocator             *basicAllocator)
: d_logStream(basicAllocator)
, d_parser(basicAllocator)
, d_elementName(basicAllocator)
, d_decoderOptions_p(decoderOptions)
, d_currentDepth(0)
{
}

// MANIPULATORS
template <typename TYPE>
int baejsn_Decoder::decode(bsl::streambuf *streamBuf, TYPE *variable)
{
    BSLS_ASSERT(streamBuf);
    BSLS_ASSERT(variable);

    bdeat_TypeCategory::Value category =
                                bdeat_TypeCategoryFunctions::select(*variable);

    if (bdeat_TypeCategory::BDEAT_SEQUENCE_CATEGORY != category
     && bdeat_TypeCategory::BDEAT_CHOICE_CATEGORY   != category
     && bdeat_TypeCategory::BDEAT_ARRAY_CATEGORY    != category) {
        d_logStream << "The object being decoded must be a Sequence, "
                    << "Choice, or Array type\n";
        return -1;                                                    // RETURN
    }

    d_parser.reset(streamBuf);

    d_logStream.clear();
    d_logStream.str("");

    bdeat_ValueTypeFunctions::reset(variable);

    typedef typename bdeat_TypeCategory::Select<TYPE>::Type TypeCategory;

    const int rc = d_parser.advanceToNextToken();
    if (rc) {
        return -1;
    }

    return decodeImp(variable, TypeCategory());
}

template <typename TYPE>
int baejsn_Decoder::decode(bsl::istream& stream, TYPE *value)
{
    if (!stream.good()) {
        return -1;                                                    // RETURN
    }

    if (0 != decode(stream.rdbuf(), value)) {
        stream.setstate(bsl::ios_base::failbit);
        return -1;                                                    // RETURN
    }

    return 0;
}

// ACCESSORS
inline
bsl::string baejsn_Decoder::loggedMessages() const
{
    return d_logStream.str();
}

                    // ------------------------------------
                    // struct baejsn_Decoder_ElementVisitor
                    // ------------------------------------

template <typename TYPE>
inline
int baejsn_Decoder_ElementVisitor::operator()(TYPE *value)
{
    typedef typename bdeat_TypeCategory::Select<TYPE>::Type TypeCategory;
    return d_decoder_p->decodeImp(value, TypeCategory());
}

template <typename TYPE, typename INFO>
inline
int baejsn_Decoder_ElementVisitor::operator()(TYPE *value, const INFO&)
{
    return (*this)(value);
}

                    // ------------------------------------
                    // struct baejsn_Decoder_DecodeImpProxy
                    // ------------------------------------

// MANIPULATORS
template <typename TYPE>
inline
int baejsn_Decoder_DecodeImpProxy::operator()(TYPE *, bslmf_Nil)
{
    BSLS_ASSERT_OPT(!"Unreachable");

    return -1;
}

template <typename TYPE, typename ANY_CATEGORY>
inline
int baejsn_Decoder_DecodeImpProxy::operator()(TYPE         *object,
                                              ANY_CATEGORY  category)
{
    return d_decoder_p->decodeImp(object, category);
}

}  // close namespace BloombergLP

#endif

// ----------------------------------------------------------------------------
// NOTICE:
//      Copyright (C) Bloomberg L.P., 2012
//      All Rights Reserved.
//      Property of Bloomberg L.P.  (BLP)
//      This software is made available solely pursuant to the
//      terms of a BLP license agreement which governs its use.
// ----------------------------- END-OF-FILE ----------------------------------
