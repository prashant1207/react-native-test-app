//
// Copyright (c) Microsoft Corporation
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//

#include "pch.h"

#include "ReactInstance.h"

#include <filesystem>

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Web.Http.Headers.h>

#include "AutolinkedNativeModules.g.h"
#include "ReactPackageProvider.h"

using ReactTestApp::ReactInstance;
using winrt::ReactTestApp::implementation::ReactPackageProvider;
using winrt::Windows::Foundation::IAsyncOperation;
using winrt::Windows::Foundation::PropertyValue;
using winrt::Windows::Foundation::Uri;
using winrt::Windows::Storage::ApplicationData;
using winrt::Windows::Web::Http::HttpClient;

namespace
{
    winrt::hstring const kBreakOnFirstLine = L"breakOnFirstLine";
    winrt::hstring const kUseDirectDebugger = L"useDirectDebugger";
    winrt::hstring const kUseFastRefresh = L"useFastRefresh";
    winrt::hstring const kUseWebDebugger = L"useWebDebugger";

    bool RetrieveLocalSetting(winrt::hstring const &key, bool defaultValue)
    {
        auto localSettings = ApplicationData::Current().LocalSettings();
        auto values = localSettings.Values();
        return winrt::unbox_value_or<bool>(values.Lookup(key), defaultValue);
    }

    void StoreLocalSetting(winrt::hstring const &key, bool value)
    {
        auto localSettings = ApplicationData::Current().LocalSettings();
        auto values = localSettings.Values();
        values.Insert(key, PropertyValue::CreateBoolean(value));
    }
}  // namespace

ReactInstance::ReactInstance()
{
    reactNativeHost_.PackageProviders().Append(winrt::make<ReactPackageProvider>());
    winrt::Microsoft::ReactNative::RegisterAutolinkedNativeModulePackages(
        reactNativeHost_.PackageProviders());
}

void ReactInstance::LoadJSBundleFrom(JSBundleSource source)
{
    auto instanceSettings = reactNativeHost_.InstanceSettings();
    switch (source) {
        case JSBundleSource::DevServer:
            instanceSettings.JavaScriptMainModuleName(L"index");
            instanceSettings.JavaScriptBundleFile(L"");
            break;
        case JSBundleSource::Embedded:
            instanceSettings.JavaScriptBundleFile(winrt::to_hstring(GetBundleName()));
            break;
    }

    Reload();
}

void ReactInstance::Reload()
{
    auto instanceSettings = reactNativeHost_.InstanceSettings();

    instanceSettings.UseWebDebugger(UseWebDebugger());
    instanceSettings.UseDirectDebugger(UseDirectDebugger());

    auto useFastRefresh = UseFastRefresh();
    instanceSettings.UseFastRefresh(useFastRefresh);
    instanceSettings.UseLiveReload(useFastRefresh);

    // instanceSettings.EnableDeveloperMenu(false);

    reactNativeHost_.ReloadInstance();
}

bool ReactInstance::BreakOnFirstLine() const
{
    return RetrieveLocalSetting(kBreakOnFirstLine, false);
}

void ReactInstance::BreakOnFirstLine(bool breakOnFirstLine)
{
    StoreLocalSetting(kBreakOnFirstLine, breakOnFirstLine);
    Reload();
}

bool ReactInstance::UseDirectDebugger() const
{
    return RetrieveLocalSetting(kUseDirectDebugger, false);
}

void ReactInstance::UseDirectDebugger(bool useDirectDebugger)
{
    if (useDirectDebugger) {
        // Remote debugging is incompatible with direct debugging
        StoreLocalSetting(kUseWebDebugger, false);
    }
    StoreLocalSetting(kUseDirectDebugger, useDirectDebugger);
    Reload();
}

bool ReactInstance::UseFastRefresh() const
{
    return RetrieveLocalSetting(kUseFastRefresh, true);
}

void ReactInstance::UseFastRefresh(bool useFastRefresh)
{
    StoreLocalSetting(kUseFastRefresh, useFastRefresh);
    Reload();
}

bool ReactInstance::UseWebDebugger() const
{
    return RetrieveLocalSetting(kUseWebDebugger, false);
}

void ReactInstance::UseWebDebugger(bool useWebDebugger)
{
    if (useWebDebugger) {
        // Remote debugging is incompatible with direct debugging
        StoreLocalSetting(kUseDirectDebugger, false);
    }
    StoreLocalSetting(kUseWebDebugger, useWebDebugger);
    Reload();
}

std::string ReactTestApp::GetBundleName()
{
    auto entryFileNames = {"index.windows",
                           "main.windows",
                           "index.native",
                           "main.native",
                           "index"
                           "main"};

    for (std::string main : entryFileNames) {
        std::string path = "Bundle\\" + main + ".bundle";
        if (std::filesystem::exists(path)) {
            return main;
        }
    }

    return "";
}

IAsyncOperation<bool> ReactTestApp::IsDevServerRunning()
{
    Uri uri(L"http://localhost:8081/status");
    HttpClient httpClient;
    try {
        auto r = co_await httpClient.GetAsync(uri);
        co_return r.IsSuccessStatusCode();
    } catch (winrt::hresult_error &) {
        co_return false;
    }
}
