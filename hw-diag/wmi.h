#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

class WmiQuery {
public:
    WmiQuery();
    ~WmiQuery();
    bool Init();
    void Cleanup();

    std::vector<std::vector<std::wstring>> Query(const std::wstring& wql);

    // Get raw VARIANT for a field from the first matching row.
    // Caller must VariantClear() the returned VARIANT.
    VARIANT QueryField(const std::wstring& wql, const std::wstring& field);

private:
    IWbemServices* pSvc = nullptr;
    IWbemLocator* pLoc = nullptr;
    bool initialized = false;
};
