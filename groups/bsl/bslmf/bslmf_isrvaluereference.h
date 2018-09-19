// bslmf_isrvaluereference.h                                          -*-C++-*-
#ifndef INCLUDED_BSLMF_ISRVALUEREFERENCE
#define INCLUDED_BSLMF_ISRVALUEREFERENCE

#ifndef INCLUDED_BSLS_IDENT
#include <bsls_ident.h>
#endif
BSLS_IDENT("$Id: $")

//@PURPOSE: Provide a compile-time check for rvalue reference types.
//
//@CLASSES:
//  bsl::is_rvalue_reference: standard meta-function for rvalue reference types
//  bsl::is_rvalue_reference_v: the result value of the standard meta-function
//
//@SEE_ALSO: bslmf_integralconstant
//
//@AUTHOR:
//
//@DESCRIPTION: This component defines a meta-function,
// 'bsl::is_rvalue_reference' and a template variable
// 'bsl::is_rvalue_reference_v', that represents the result value of the
// 'bsl::is_rvalue_reference' meta-function, that may be used to query whether
// a type is an rvalue reference type.
//
// 'bsl::is_rvalue_reference' meets the requirements of the
// 'is_rvalue_reference' template defined in the C++11 standard
// [meta.unary.cat].
//
// Note that the template variable 'is_rvalue_reference_v' is defined in the
// C++17 standard as an inline variable.  If the current compiler supports the
// inline variable C++17 compiler feature, 'bsl::is_rvalue_reference_v' is
// defined as an 'inline constexpr bool' variable.  Otherwise, if the compiler
// supports the variable templates C++14 compiler feature,
// 'bsl::is_rvalue_reference_v' is defined as a non-inline 'constexpr bool'
// variable.  See 'BSLS_COMPILERFEATURES_SUPPORT_INLINE_VARIABLES' and
// 'BSLS_COMPILERFEATURES_SUPPORT_VARIABLE_TEMPLATES' macros in
// bsls_compilerfeatures component for details.
//
///Usage
///-----
// In this section we show intended use of this component.
//
///Example 1: Verify Rvalue Reference Types
/// - - - - - - - - - - - - - - - - - - - -
// Suppose that we want to assert whether a set of types are rvalue reference
// types.
//
// Now, we instantiate the 'bsl::is_rvalue_reference' template for both a
// non-reference type and an rvalue reference type, and assert the 'value'
// static data member of each instantiation:
//..
//  assert(false == bsl::is_rvalue_reference<int>::value);
//#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES)
//  assert(true  == bsl::is_rvalue_reference<int&&>::value);
//#endif
//..
// Note that rvalue reference is a feature introduced in the C++11 standard,
// and may not be supported by all compilers.
//
// Also note that if the current compiler supports the variable templates C++14
// feature then we can re-write the snippet of code above using the
// 'bsl::is_rvalue_reference_v' variable as follows:
//..
//#ifdef BSLS_COMPILERFEATURES_SUPPORT_VARIABLE_TEMPLATES
//  assert(false == bsl::is_rvalue_reference_v<int>);
//  assert(true  == bsl::is_rvalue_reference_v<int&&>);
//#endif
//..

#ifndef INCLUDED_BSLSCM_VERSION
#include <bslscm_version.h>
#endif

#ifndef INCLUDED_BSLMF_INTEGRALCONSTANT
#include <bslmf_integralconstant.h>
#endif

#ifndef INCLUDED_BSLS_COMPILERFEATURES
#include <bsls_compilerfeatures.h>
#endif

#ifndef INCLUDED_BSLS_KEYWORD
#include <bsls_keyword.h>
#endif

namespace bsl {

                          // ==========================
                          // struct is_rvalue_reference
                          // ==========================

template <class TYPE>
struct is_rvalue_reference : false_type {
    // This 'struct' template provides a meta-function to determine whether the
    // (template parameter) 'TYPE' is a (possibly cv-qualified) rvalue
    // reference type.  This generic default template derives from
    // 'bsl::false_type'.  A template specialization is provided (below) that
    // derives from 'bsl::true_type'.
};

#if defined(BSLS_COMPILERFEATURES_SUPPORT_RVALUE_REFERENCES)

template <class TYPE>
struct is_rvalue_reference<TYPE&&> : true_type {
    // This partial specialization of 'is_rvalue_reference' derives from
    // 'bsl::true_type' for when the (template parameter) 'TYPE' is an rvalue
    // reference type.
};

#endif

#ifdef BSLS_COMPILERFEATURES_SUPPORT_VARIABLE_TEMPLATES
template <class TYPE>
BSLS_KEYWORD_INLINE_VARIABLE
constexpr bool is_rvalue_reference_v = is_rvalue_reference<TYPE>::value;
    // This template variable represents the result value of the
    // 'bsl::is_rvalue_reference' meta-function.
#endif

}  // close namespace bsl

#endif

// ----------------------------------------------------------------------------
// Copyright 2013 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------
