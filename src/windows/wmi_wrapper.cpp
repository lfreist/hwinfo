// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <oleauto.h>

#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "internal/windows_error.h"
#include "internal/windows_strings.h"
#include "internal/wmi_wrapper.h"

namespace hwinfo::internal::wmi {

namespace {

// Owning BSTR.
class Bstr {
 public:
  explicit Bstr(std::wstring_view s) : _str(SysAllocStringLen(s.data(), static_cast<UINT>(s.size()))) {}
  Bstr(const Bstr&) = delete;
  Bstr& operator=(const Bstr&) = delete;
  ~Bstr() { SysFreeString(_str); }

  [[nodiscard]] BSTR get() const noexcept { return _str; }

 private:
  BSTR _str;
};

// Owning VARIANT.
class Variant {
 public:
  Variant() { VariantInit(&_var); }
  Variant(const Variant&) = delete;
  Variant& operator=(const Variant&) = delete;
  ~Variant() { VariantClear(&_var); }

  VARIANT* get() noexcept { return &_var; }
  const VARIANT& operator*() const noexcept { return _var; }

 private:
  VARIANT _var;
};

std::string to_utf8(BSTR s) { return s == nullptr ? std::string{} : internal::to_utf8({s, SysStringLen(s)}); }

std::vector<std::string> string_array(SAFEARRAY* array) {
  std::vector<std::string> out;
  LONG lower = 0;
  LONG upper = -1;
  if (array == nullptr || SafeArrayGetDim(array) != 1 || FAILED(SafeArrayGetLBound(array, 1, &lower)) ||
      FAILED(SafeArrayGetUBound(array, 1, &upper))) {
    return out;
  }
  BSTR* data = nullptr;
  if (FAILED(SafeArrayAccessData(array, reinterpret_cast<void**>(&data)))) {
    return out;
  }
  for (LONG i = 0; i <= upper - lower; ++i) {
    out.push_back(to_utf8(data[i]));
  }
  SafeArrayUnaccessData(array);
  return out;
}

// WMI transfers unsigned 16 and 32 bit integers as VT_I4: the CIM type tells how to interpret them.
std::optional<Value> to_value(const VARIANT& var, CIMTYPE cim_type) {
  const bool is_unsigned = cim_type == CIM_UINT8 || cim_type == CIM_UINT16 || cim_type == CIM_UINT32;
  switch (var.vt) {
    case VT_BSTR:
      return to_utf8(var.bstrVal);
    case VT_BOOL:
      return var.boolVal != VARIANT_FALSE;
    case VT_I1:
      return std::int64_t{var.cVal};
    case VT_I2:
      return is_unsigned ? Value{std::uint64_t{static_cast<USHORT>(var.iVal)}} : Value{std::int64_t{var.iVal}};
    case VT_I4:
    case VT_INT:
      return is_unsigned ? Value{std::uint64_t{static_cast<ULONG>(var.lVal)}} : Value{std::int64_t{var.lVal}};
    case VT_I8:
      return std::int64_t{var.llVal};
    case VT_UI1:
      return std::uint64_t{var.bVal};
    case VT_UI2:
      return std::uint64_t{var.uiVal};
    case VT_UI4:
    case VT_UINT:
      return std::uint64_t{var.ulVal};
    case VT_UI8:
      return std::uint64_t{var.ullVal};
    case VT_R4:
      return double{var.fltVal};
    case VT_R8:
      return var.dblVal;
    case VT_ARRAY | VT_BSTR:
      return string_array(var.parray);
    default:
      return std::nullopt;  // VT_NULL, VT_EMPTY and types not used by hwinfo
  }
}

// Lets the (out of process) WMI service impersonate the caller. Required on every proxy obtained from WMI.
HRESULT set_proxy_blanket(IUnknown* proxy) {
  return CoSetProxyBlanket(proxy, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
}

}  // namespace

result<Connection> Connection::connect(std::wstring_view wmi_namespace) {
  // Note: CoInitializeSecurity is deliberately not called. It configures process wide security and may only be called
  // once per process, which is the responsibility of the application. The proxy blankets suffice for local queries.
  Connection connection;
  const std::string context = std::format("WMI: {}", internal::to_utf8(wmi_namespace));
  if (!connection._com.usable()) {
    return std::unexpected(hresult_error(connection._com.status(), context));
  }
  HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                                connection._locator.put_void());
  if (FAILED(hr)) {
    return std::unexpected(hresult_error(hr, context));
  }
  const Bstr resource(wmi_namespace);
  hr = connection._locator->ConnectServer(resource.get(), nullptr, nullptr, nullptr, 0, nullptr, nullptr,
                                          connection._services.put());
  if (FAILED(hr)) {
    return std::unexpected(hresult_error(hr, context));
  }
  hr = set_proxy_blanket(connection._services.get());
  if (FAILED(hr)) {
    return std::unexpected(hresult_error(hr, context));
  }
  return connection;
}

result<std::vector<Row>> Connection::query(std::string_view wmi_class,
                                           std::initializer_list<std::string_view> properties,
                                           std::string_view filter) const {
  const std::string context = std::format("WMI: {}", wmi_class);
  std::string wql = std::format("SELECT {} FROM {}", join(properties, ", "), wmi_class);
  if (!filter.empty()) {
    wql += std::format(" WHERE {}", filter);
  }

  const Bstr language(L"WQL");
  const Bstr query(to_wide(wql));
  ComPtr<IEnumWbemClassObject> enumerator;
  HRESULT hr = _services->ExecQuery(language.get(), query.get(), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                    nullptr, enumerator.put());
  if (FAILED(hr)) {
    return std::unexpected(hresult_error(hr, context));
  }
  set_proxy_blanket(enumerator.get());

  std::vector<std::wstring> names;
  names.reserve(properties.size());
  for (const auto property : properties) {
    names.push_back(to_wide(property));
  }

  std::vector<Row> rows;
  while (true) {
    ComPtr<IWbemClassObject> object;
    ULONG returned = 0;
    hr = enumerator->Next(static_cast<long>(WBEM_INFINITE), 1, object.put(), &returned);
    if (FAILED(hr)) {
      return std::unexpected(hresult_error(hr, context));
    }
    if (returned == 0) {
      break;
    }
    Row row;
    for (std::size_t i = 0; i < names.size(); ++i) {
      Variant var;
      CIMTYPE cim_type = CIM_EMPTY;
      if (FAILED(object->Get(names[i].c_str(), 0, var.get(), &cim_type, nullptr))) {
        continue;
      }
      if (auto value = to_value(*var, cim_type)) {
        row.set(std::string(properties.begin()[i]), std::move(*value));
      }
    }
    rows.push_back(std::move(row));
  }
  return rows;
}

}  // namespace hwinfo::internal::wmi

#endif  // HWINFO_WINDOWS
