/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/ByteString.h>
#include <AK/FlyString.h>
#include <AK/HashTable.h>
#include <AK/Optional.h>
#include <AK/Singleton.h>
#include <AK/StringUtils.h>
#include <AK/StringView.h>

namespace AK {

struct FlyStringImplTraits : public Traits<StringImpl const*> {
    static unsigned hash(StringImpl const* s) { return s->hash(); }
    static bool equals(StringImpl const* a, StringImpl const* b)
    {
        return *a == *b;
    }
};

static Singleton<HashTable<StringImpl const*, FlyStringImplTraits>> s_table;

static HashTable<StringImpl const*, FlyStringImplTraits>& fly_impls()
{
    return *s_table;
}

void FlyString::did_destroy_impl(Badge<StringImpl>, StringImpl& impl)
{
    fly_impls().remove(&impl);
}

FlyString::FlyString(ByteString const& string)
    : m_impl(string.impl())
{
    if (string.impl()->is_fly())
        return;

    auto it = fly_impls().find(string.impl());
    if (it == fly_impls().end()) {
        fly_impls().set(string.impl());
        string.impl()->set_fly({}, true);
        m_impl = string.impl();
    } else {
        VERIFY((*it)->is_fly());
        m_impl = **it;
    }
}

FlyString::FlyString(StringView string)
    : m_impl(StringImpl::the_empty_stringimpl())
{
    if (string.is_null())
        return;
    auto it = fly_impls().find(string.hash(), [&](auto& candidate) {
        return string == *candidate;
    });
    if (it == fly_impls().end()) {
        auto new_string = string.to_byte_string();
        fly_impls().set(new_string.impl());
        new_string.impl()->set_fly({}, true);
        m_impl = new_string.impl();
    } else {
        VERIFY((*it)->is_fly());
        m_impl = **it;
    }
}

bool FlyString::equals_ignoring_ascii_case(StringView other) const
{
    return StringUtils::equals_ignoring_ascii_case(view(), other);
}

bool FlyString::starts_with(StringView str, CaseSensitivity case_sensitivity) const
{
    return StringUtils::starts_with(view(), str, case_sensitivity);
}

bool FlyString::ends_with(StringView str, CaseSensitivity case_sensitivity) const
{
    return StringUtils::ends_with(view(), str, case_sensitivity);
}

FlyString FlyString::to_lowercase() const
{
    return ByteString(*m_impl).to_lowercase();
}

bool FlyString::operator==(ByteString const& other) const
{
    return m_impl == other.impl() || view() == other.view();
}

bool FlyString::operator==(StringView string) const
{
    return view() == string;
}

bool FlyString::operator==(char const* string) const
{
    return view() == string;
}

}
