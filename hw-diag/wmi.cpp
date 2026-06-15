#include "wmi.h"
#include <stdexcept>

WmiQuery::WmiQuery() {}

WmiQuery::~WmiQuery() { Cleanup(); }

bool WmiQuery::Init() {
    HRESULT hr;
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return false;

    hr = CoInitializeSecurity(NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL);
    if (FAILED(hr) && hr != RPC_E_TOO_LATE) return false;

    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hr)) return false;

    hr = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, 0, 0, 0, &pSvc);
    if (FAILED(hr)) return false;

    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE);
    if (FAILED(hr)) return false;

    initialized = true;
    return true;
}

void WmiQuery::Cleanup() {
    if (pSvc) { pSvc->Release(); pSvc = nullptr; }
    if (pLoc) { pLoc->Release(); pLoc = nullptr; }
    initialized = false;
}

std::wstring GetVariantStr(IWbemClassObject* pObj, const wchar_t* name) {
    VARIANT vt;
    VariantInit(&vt);
    HRESULT hr = pObj->Get(name, 0, &vt, 0, 0);
    std::wstring result;
    if (SUCCEEDED(hr)) {
        if (vt.vt == VT_BSTR && vt.bstrVal)
            result = vt.bstrVal;
        else if (vt.vt == VT_I4)
            result = std::to_wstring(vt.intVal);
        else if (vt.vt == VT_I8)
            result = std::to_wstring(vt.llVal);
        else if (vt.vt == VT_UI4)
            result = std::to_wstring(vt.ulVal);
        else if (vt.vt == VT_UI8)
            result = std::to_wstring(vt.ullVal);
        else if (vt.vt == VT_R4) {
            wchar_t buf[64]; swprintf_s(buf, L"%.2f", vt.fltVal); result = buf;
        }
        else if (vt.vt == VT_R8) {
            wchar_t buf[64]; swprintf_s(buf, L"%.2f", vt.dblVal); result = buf;
        }
    }
    VariantClear(&vt);
    return result;
}

std::vector<std::vector<std::wstring>> WmiQuery::Query(const std::wstring& wql) {
    std::vector<std::vector<std::wstring>> rows;
    if (!initialized) return rows;

    IEnumWbemClassObject* pEnum = NULL;
    HRESULT hr = pSvc->ExecQuery(
        _bstr_t(L"WQL"), _bstr_t(wql.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnum);
    if (FAILED(hr)) return rows;

    IWbemClassObject* pObj = NULL;
    ULONG returned = 0;
    while (pEnum->Next(WBEM_INFINITE, 1, &pObj, &returned) == S_OK) {
        SAFEARRAY* pNames = NULL;
        hr = pObj->GetNames(NULL, WBEM_FLAG_ALWAYS, NULL, &pNames);
        if (SUCCEEDED(hr)) {
            LONG lLower, lUpper;
            SafeArrayGetLBound(pNames, 1, &lLower);
            SafeArrayGetUBound(pNames, 1, &lUpper);
            std::vector<std::wstring> row;
            for (LONG i = lLower; i <= lUpper; i++) {
                BSTR name;
                SafeArrayGetElement(pNames, &i, &name);
                std::wstring val = GetVariantStr(pObj, name);
                row.push_back(name ? name : L"");
                row.push_back(val);
                SysFreeString(name);
            }
            SafeArrayDestroy(pNames);
            rows.push_back(row);
        }
        pObj->Release();
    }
    pEnum->Release();
    return rows;
}

VARIANT WmiQuery::QueryField(const std::wstring& wql, const std::wstring& field) {
    VARIANT vt;
    VariantInit(&vt);
    if (!initialized) return vt;

    IEnumWbemClassObject* pEnum = NULL;
    HRESULT hr = pSvc->ExecQuery(
        _bstr_t(L"WQL"), _bstr_t(wql.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnum);
    if (FAILED(hr)) return vt;

    IWbemClassObject* pObj = NULL;
    ULONG returned = 0;
    if (pEnum->Next(WBEM_INFINITE, 1, &pObj, &returned) == S_OK) {
        pObj->Get(field.c_str(), 0, &vt, 0, 0);
        pObj->Release();
    }
    pEnum->Release();
    return vt;
}
