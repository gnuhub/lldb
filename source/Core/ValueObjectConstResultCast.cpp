//===-- ValueObjectConstResultCast.cpp --------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/Core/ValueObjectConstResultCast.h"

#include "lldb/Core/ValueObjectConstResult.h"
#include "lldb/Core/ValueObjectList.h"

#include "lldb/Symbol/ClangASTContext.h"

using namespace lldb_private;

ValueObjectConstResultCast::ValueObjectConstResultCast(
    ValueObject &parent,
    const ConstString &name,
    const ClangASTType &cast_type,
    lldb::addr_t live_address) :
    ValueObjectCast (parent, name, cast_type),
    m_impl(this, live_address)
{
    m_name = name;
}

ValueObjectConstResultCast::~ValueObjectConstResultCast()
{
}

lldb::ValueObjectSP
ValueObjectConstResultCast::Dereference (Error &error)
{
    return m_impl.Dereference(error);
}

lldb::ValueObjectSP
ValueObjectConstResultCast::GetSyntheticChildAtOffset(uint32_t offset,
                                                      const ClangASTType& type,
                                                      bool can_create)
{
    return m_impl.GetSyntheticChildAtOffset(offset, type, can_create);
}

lldb::ValueObjectSP
ValueObjectConstResultCast::AddressOf (Error &error)
{
    return m_impl.AddressOf(error);
}

ValueObject *
ValueObjectConstResultCast::CreateChildAtIndex (size_t idx,
                                                bool synthetic_array_member,
                                                int32_t synthetic_index)
{
    return m_impl.CreateChildAtIndex(
        idx, synthetic_array_member, synthetic_index);
}

size_t
ValueObjectConstResultCast::GetPointeeData (DataExtractor& data,
                                             uint32_t item_idx,
                                             uint32_t item_count)
{
    return m_impl.GetPointeeData(data, item_idx, item_count);
}

lldb::ValueObjectSP
ValueObjectConstResultCast::Cast (const ClangASTType &clang_ast_type)
{
    return m_impl.Cast(clang_ast_type);
}
