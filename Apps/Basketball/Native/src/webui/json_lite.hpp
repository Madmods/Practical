// json_lite.hpp — just enough JSON to talk to the WebView2 page (parse + write, wide strings).
#pragma once
#include <cwchar>
#include <cmath>
#include <stdexcept>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <map>
#include <string>
#include <vector>

namespace json {

struct Value {
    enum Type { Null, Bool, Number, String, Array, Object } type = Null;
    bool b = false;
    double n = 0;
    std::wstring s;
    std::vector<Value> a;
    std::map<std::wstring, Value> o;

    const Value& operator[](const std::wstring& key) const {
        static const Value none;
        auto it = o.find(key);
        return it == o.end() ? none : it->second;
    }
    std::wstring str(const std::wstring& fallback = L"") const { return type == String ? s : fallback; }
    double num(double fallback = 0) const { return type == Number ? n : fallback; }
    bool boolean(bool fallback = false) const { return type == Bool ? b : fallback; }
};

// The value interface is retained; parsing uses Windows' established JSON library.
inline Value convert(const winrt::Windows::Data::Json::IJsonValue& j, unsigned depth=0) {
    using winrt::Windows::Data::Json::JsonValueType;
    if(depth>32) throw std::runtime_error("JSON depth"); Value v;
    switch(j.ValueType()) {
    case JsonValueType::Null: break;
    case JsonValueType::Boolean: v.type=Value::Bool;v.b=j.GetBoolean();break;
    case JsonValueType::Number: v.type=Value::Number;v.n=j.GetNumber();if(!std::isfinite(v.n))throw std::runtime_error("JSON number");break;
    case JsonValueType::String: v.type=Value::String;v.s=j.GetString();break;
    case JsonValueType::Array: v.type=Value::Array;for(auto x:j.GetArray())v.a.push_back(convert(x,depth+1));break;
    case JsonValueType::Object: v.type=Value::Object;for(auto x:j.GetObject())v.o[std::wstring(x.Key())]=convert(x.Value(),depth+1);break;
    } return v;
}
inline Value parse(const std::wstring& text) {
    if(text.size()>65536)return {};
    unsigned depth=0;bool quoted=false,escape=false;
    for(auto c:text) {
        if(quoted){if(escape)escape=false;else if(c==L'\\')escape=true;else if(c==L'"')quoted=false;}
        else if(c==L'"')quoted=true;
        else if(c==L'['||c==L'{'){if(++depth>32)return {};}
        else if(c==L']'||c==L'}'){if(!depth)return {};--depth;}
    }
    if(depth||quoted)return {};
    try {winrt::Windows::Data::Json::JsonValue value{nullptr};
        if(!winrt::Windows::Data::Json::JsonValue::TryParse(text,value))return {};
        return convert(value);
    }catch(...){return {};}
}

// Quote a string as a JSON string literal.
inline std::wstring quote(const std::wstring& s) {
    std::wstring out = L"\"";
    for (wchar_t c : s) {
        switch (c) {
            case L'"': out += L"\\\""; break;
            case L'\\': out += L"\\\\"; break;
            case L'\n': out += L"\\n"; break;
            case L'\r': out += L"\\r"; break;
            case L'\t': out += L"\\t"; break;
            default:
                if (c < 0x20) { wchar_t buf[8]; swprintf(buf, 8, L"\\u%04x", static_cast<unsigned>(c)); out += buf; }
                else out += c;
        }
    }
    return out + L"\"";
}

// Serialise a parsed value back to JSON text (used to hand nested data to your code as-is).
inline std::wstring dump(const Value& v) {
    switch (v.type) {
        case Value::Null: return L"null";
        case Value::Bool: return v.b ? L"true" : L"false";
        case Value::Number: { if(!std::isfinite(v.n))return L"null"; wchar_t buf[64]; swprintf(buf, 64, L"%.15g", v.n); return buf; }
        case Value::String: return quote(v.s);
        case Value::Array: {
            std::wstring out = L"[";
            for (size_t i = 0; i < v.a.size(); ++i) { if (i) out += L","; out += dump(v.a[i]); }
            return out + L"]";
        }
        case Value::Object: {
            std::wstring out = L"{"; bool first = true;
            for (const auto& kv : v.o) { if (!first) out += L","; first = false; out += quote(kv.first) + L":" + dump(kv.second); }
            return out + L"}";
        }
    }
    return L"null";
}

}  // namespace json
