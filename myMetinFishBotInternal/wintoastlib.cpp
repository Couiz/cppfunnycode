#include "wintoastlib.h"
#include <fstream>
#pragma comment(lib,"shlwapi")
#pragma comment(lib,"user32")

using namespace WinToastLib;

std::ofstream efile;
namespace DllImporter {

    // Function load a function from library
    template <typename Function>
    HRESULT loadFunctionFromLibrary(HINSTANCE library, LPCSTR name, Function &func) {
        if (!library)
            return false;

        func = reinterpret_cast<Function>(GetProcAddress(library, name));
        return (func != nullptr) ? S_OK : E_FAIL;
    }

    typedef HRESULT(FAR STDAPICALLTYPE *f_SetCurrentProcessExplicitAppUserModelID)(__in PCWSTR AppID);
    typedef HRESULT(FAR STDAPICALLTYPE *f_PropVariantToString)(_In_ REFPROPVARIANT propvar, _Out_writes_(cch) PWSTR psz, _In_ UINT cch);
    typedef HRESULT(FAR STDAPICALLTYPE *f_RoGetActivationFactory)(_In_ HSTRING activatableClassId, _In_ REFIID iid, _COM_Outptr_ void ** factory);
    typedef HRESULT(FAR STDAPICALLTYPE *f_WindowsCreateStringReference)(_In_reads_opt_(length + 1) PCWSTR sourceString, UINT32 length, _Out_ HSTRING_HEADER * hstringHeader, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING * string);
    typedef HRESULT(FAR STDAPICALLTYPE *f_WindowsDeleteString)(_In_opt_ HSTRING string);

    f_SetCurrentProcessExplicitAppUserModelID    SetCurrentProcessExplicitAppUserModelID;
    f_PropVariantToString                        PropVariantToString;
    f_RoGetActivationFactory                     RoGetActivationFactory;
    f_WindowsCreateStringReference               WindowsCreateStringReference;
    f_WindowsDeleteString                        WindowsDeleteString;


    template<class T>
    _Check_return_ __inline HRESULT _1_GetActivationFactory(_In_ HSTRING activatableClassId, _COM_Outptr_ T** factory) {
        return RoGetActivationFactory(activatableClassId, IID_INS_ARGS(factory));
    }

    template<typename T>
    inline HRESULT Wrap_GetActivationFactory(_In_ HSTRING activatableClassId, _Inout_ Details::ComPtrRef<T> factory) throw() {
        return _1_GetActivationFactory(activatableClassId, factory.ReleaseAndGetAddressOf());
    }

    inline HRESULT initialize() {
        HINSTANCE LibShell32 = LoadLibrary(L"SHELL32.DLL");
        HRESULT hr = loadFunctionFromLibrary(LibShell32, "SetCurrentProcessExplicitAppUserModelID", SetCurrentProcessExplicitAppUserModelID);
        if (SUCCEEDED(hr)) {
            HINSTANCE LibPropSys = LoadLibrary(L"PROPSYS.DLL");
            hr = loadFunctionFromLibrary(LibPropSys, "PropVariantToString", PropVariantToString);
            if (SUCCEEDED(hr)) {
                HINSTANCE LibComBase = LoadLibrary(L"COMBASE.DLL");
                return SUCCEEDED(loadFunctionFromLibrary(LibComBase, "RoGetActivationFactory", RoGetActivationFactory))
                        && SUCCEEDED(loadFunctionFromLibrary(LibComBase, "WindowsCreateStringReference", WindowsCreateStringReference))
                        && SUCCEEDED(loadFunctionFromLibrary(LibComBase, "WindowsDeleteString", WindowsDeleteString));
            }
        }
        return hr;
    }
}

namespace Util {
    inline HRESULT defaultExecutablePath(_In_ WCHAR* path, _In_ DWORD nSize = MAX_PATH) {
        DWORD written = GetModuleFileNameEx(GetCurrentProcess(), nullptr, path, nSize);
        std::wcout << "Default executable path: " << path << std::endl;
        return (written > 0) ? S_OK : E_FAIL;
    }


    inline HRESULT defaultShellLinksDirectory(_In_ WCHAR* path, _In_ DWORD nSize = MAX_PATH) {
        DWORD written = GetEnvironmentVariable(L"APPDATA", path, nSize);
        HRESULT hr = written > 0 ? S_OK : E_INVALIDARG;
        if (SUCCEEDED(hr)) {
            errno_t result = wcscat_s(path, nSize, DEFAULT_SHELL_LINKS_PATH);
            hr = (result == 0) ? S_OK : E_INVALIDARG;
            std::wcout << "Default shell link path: " << path << std::endl;
        }
        return hr;
    }

    inline HRESULT defaultShellLinkPath(const std::wstring& appname, _In_ WCHAR* path, _In_ DWORD nSize = MAX_PATH) {
        HRESULT hr = defaultShellLinksDirectory(path, nSize);
        if (SUCCEEDED(hr)) {
            const std::wstring appLink(appname + DEFAULT_LINK_FORMAT);
            errno_t result = wcscat_s(path, nSize, appLink.c_str());
            hr = (result == 0) ? S_OK : E_INVALIDARG;
            std::wcout << "Default shell link file path: " << path << std::endl;
        }
        return hr;
    }


    inline HRESULT setNodeStringValue(const std::wstring& string, IXmlNode *node, IXmlDocument *xml) {
        ComPtr<IXmlText> textNode;
        HRESULT hr = xml->CreateTextNode( WinToastStringWrapper(string).Get(), &textNode);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> stringNode;
            hr = textNode.As(&stringNode);
            if (SUCCEEDED(hr)) {
                ComPtr<IXmlNode> appendedChild;
                hr = node->AppendChild(stringNode.Get(), &appendedChild);
            }
        }
        return hr;
    }

    inline HRESULT setEventHandlers(_In_ IToastNotification* notification, _In_ WinToastHandler* eventHandler) {
        EventRegistrationToken activatedToken, dismissedToken, failedToken;
        HRESULT hr = notification->add_Activated(
                    Callback < Implements < RuntimeClassFlags<ClassicCom>,
                    ITypedEventHandler<ToastNotification*, IInspectable* >> >(
                    [eventHandler](IToastNotification*, IInspectable*)
                {
                    eventHandler->toastActivated();
                    return S_OK;
                }).Get(), &activatedToken);

        if (SUCCEEDED(hr)) {
            hr = notification->add_Dismissed(Callback < Implements < RuntimeClassFlags<ClassicCom>,
                     ITypedEventHandler<ToastNotification*, ToastDismissedEventArgs* >> >(
                     [eventHandler](IToastNotification*, IToastDismissedEventArgs* e)
                 {
                     ToastDismissalReason reason;
                     if (SUCCEEDED(e->get_Reason(&reason)))
                     {
                         eventHandler->toastDismissed(static_cast<WinToastHandler::WinToastDismissalReason>(reason));
                     }
                     return S_OK;
                 }).Get(), &dismissedToken);
            if (SUCCEEDED(hr)) {
                hr = notification->add_Failed(Callback < Implements < RuntimeClassFlags<ClassicCom>,
                    ITypedEventHandler<ToastNotification*, ToastFailedEventArgs* >> >(
                    [eventHandler](IToastNotification*, IToastFailedEventArgs*)
                {
                    eventHandler->toastFailed();
                    return S_OK;
                }).Get(), &failedToken);
            }
        }
        return hr;
    }
}

WinToast* WinToast::_instance = nullptr;
WinToast* WinToast::instance() {
    if (_instance == nullptr) {
        _instance = new WinToast();
    }
    return _instance;
}

WinToast::WinToast() : _isInitialized(false)
{
    DllImporter::initialize();
}

void WinToast::setAppName(_In_ const std::wstring& appName) {
    _appName = appName;
}

std::wstring WinToast::appName() const {
    return _appName;
}

std::wstring WinToast::appUserModelId() const {
    return _aumi;
}
void WinToast::setAppUserModelId(_In_ const std::wstring& aumi) {
    _aumi = aumi;
    std::wcout << L"Default App User Model Id: " << _aumi.c_str() << std::endl;
}

bool WinToast::isCompatible() {
        return !((DllImporter::SetCurrentProcessExplicitAppUserModelID == nullptr)
            || (DllImporter::PropVariantToString == nullptr)
            || (DllImporter::RoGetActivationFactory == nullptr)
            || (DllImporter::WindowsCreateStringReference == nullptr)
                 || (DllImporter::WindowsDeleteString == nullptr));
}

std::wstring WinToast::configureAUMI(_In_ const std::wstring &company,
                                               _In_ const std::wstring &name,
                                               _In_ const std::wstring &surname,
                                               _In_ const std::wstring &versionInfo)
{
    std::wstring aumi = company;
    aumi += L"." + name;
    aumi += L"." + surname;
    aumi += L"." + versionInfo;

    if (aumi.length() > SCHAR_MAX) {
        std::wcout << "Error: max size allowed for AUMI: 128 characters." << std::endl;
    }
    return aumi;
}

bool WinToast::initialize() {
    if (_aumi.empty() || _appName.empty()) {
        std::wcout << L"Error: App User Model Id or Appname is empty!" << std::endl;
        return _isInitialized = false;
    }

    if (!isCompatible()) {
        std::wcout << L"Your OS is not compatible with this library! =(" << std::endl;
        return _isInitialized = false;
    }

    if (FAILED(DllImporter::SetCurrentProcessExplicitAppUserModelID(_aumi.c_str()))) {
        std::wcout << L"Error while attaching the AUMI to the current proccess =(" << std::endl;
        return _isInitialized = false;
    }


    HRESULT hr = validateShellLink();
    if (FAILED(hr)) {
        hr = createShellLink();
    }

    if (SUCCEEDED(hr)) {
        hr = DllImporter::Wrap_GetActivationFactory(WinToastStringWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), &_notificationManager);
        if (SUCCEEDED(hr)) {
            hr = notificationManager()->CreateToastNotifierWithId(WinToastStringWrapper(_aumi).Get(), &_notifier);
            if (SUCCEEDED(hr)) {
                hr = DllImporter::Wrap_GetActivationFactory(WinToastStringWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), &_notificationFactory);
            }
        }
    }

    return _isInitialized = SUCCEEDED(hr);
}

HRESULT	WinToast::validateShellLink() {

    WCHAR	_path[MAX_PATH];
    Util::defaultShellLinkPath(_appName, _path);
    // Check if the file exist
    DWORD attr = GetFileAttributes(_path);
    if (attr >= 0xFFFFFFF) {
        std::wcout << "Error, shell link not found. Try to create a new one in: " << _path << std::endl;
        return E_FAIL;
    }

    // Let's load the file as shell link to validate.
    // - Create a shell link
    // - Create a persistant file
    // - Load the path as data for the persistant file
    // - Read the property AUMI and validate with the current
    // - Review if AUMI is equal.
    ComPtr<IShellLink> shellLink;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
    if (SUCCEEDED(hr)) {
        ComPtr<IPersistFile> persistFile;
        hr = shellLink.As(&persistFile);
        if (SUCCEEDED(hr)) {
            hr = persistFile->Load(_path, STGM_READWRITE);
            if (SUCCEEDED(hr)) {
                ComPtr<IPropertyStore> propertyStore;
                hr = shellLink.As(&propertyStore);
                if (SUCCEEDED(hr)) {
                    PROPVARIANT appIdPropVar;
                    hr = propertyStore->GetValue(PKEY_AppUserModel_ID, &appIdPropVar);
                    if (SUCCEEDED(hr)) {
                        WCHAR AUMI[MAX_PATH];
                        hr = DllImporter::PropVariantToString(appIdPropVar, AUMI, MAX_PATH);
                        if (SUCCEEDED(hr)) {
                            hr = (_aumi == AUMI) ? S_OK : E_FAIL;
                        } else { // AUMI Changed for the same app, let's update the current value! =)
                            PropVariantClear(&appIdPropVar);
                            hr = InitPropVariantFromString(_aumi.c_str(), &appIdPropVar);
                            if (SUCCEEDED(hr)) {
                                hr = propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar);
                                if (SUCCEEDED(hr)) {
                                    hr = propertyStore->Commit();
                                    if (SUCCEEDED(hr) && SUCCEEDED(persistFile->IsDirty())) {
                                        hr = persistFile->Save(_path, TRUE);
                                    }
                                }
                            }
                        }
                        PropVariantClear(&appIdPropVar);
                    }
                }
            }
        }
    }
    return hr;
}



HRESULT	WinToast::createShellLink() {
    WCHAR   exePath[MAX_PATH];
    WCHAR	slPath[MAX_PATH];
    Util::defaultShellLinkPath(_appName, slPath);
    Util::defaultExecutablePath(exePath);
    ComPtr<IShellLink> shellLink;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
    if (SUCCEEDED(hr)) {
        hr = shellLink->SetPath(exePath);
        if (SUCCEEDED(hr)) {
            hr = shellLink->SetArguments(L"");
            if (SUCCEEDED(hr)) {
                hr = shellLink->SetWorkingDirectory(exePath);
                if (SUCCEEDED(hr)) {
                    ComPtr<IPropertyStore> propertyStore;
                    hr = shellLink.As(&propertyStore);
                    if (SUCCEEDED(hr)) {
                        PROPVARIANT appIdPropVar;
                        hr = InitPropVariantFromString(_aumi.c_str(), &appIdPropVar);
                        if (SUCCEEDED(hr)) {
                            hr = propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar);
                            if (SUCCEEDED(hr)) {
                                hr = propertyStore->Commit();
                                if (SUCCEEDED(hr)) {
                                    ComPtr<IPersistFile> persistFile;
                                    hr = shellLink.As(&persistFile);
                                    if (SUCCEEDED(hr)) {
                                        hr = persistFile->Save(slPath, TRUE);
                                    }
                                }
                            }
                        }
                        PropVariantClear(&appIdPropVar);
                    }
                }
            }
        }
    }
    CoTaskMemFree(exePath);
    CoTaskMemFree(slPath);
    return hr;
}



bool WinToast::showToast(_In_ const WinToastTemplate& toast, _In_ WinToastHandler* handler)  {
    if (!isInitialized()) {
		efile.open("F:\\testerror.txt", std::ios_base::app);
		efile << "Error when launching the toast. WinToast is not initialized =(" << std::endl;
		efile.close();
        return _isInitialized;
    }

    HRESULT hr = _notificationManager->GetTemplateContent(ToastTemplateType(toast.type()), &_xmlDocument);
    if (SUCCEEDED(hr)) {
        const int fieldsCount = toast.textFieldsCount();
        for (int i = 0; i < fieldsCount && SUCCEEDED(hr); i++) {
            hr = setTextField(toast.textField(WinToastTemplate::TextField(i)), i);
        }
        if (SUCCEEDED(hr)) {
            hr = toast.hasImage() ? setImageField(toast.imagePath()) : hr;
            if (SUCCEEDED(hr)) {
                hr = _notificationFactory->CreateToastNotification(xmlDocument(), &_notification);
                if (SUCCEEDED(hr)) {
                    hr = Util::setEventHandlers(notification(), handler);
                    if (SUCCEEDED(hr)) {
                        hr = _notifier->Show(notification());
                    }
                }
            }
        }
    }

    return SUCCEEDED(hr);
}


HRESULT WinToast::setTextField(_In_ const std::wstring& text, _In_ int pos) {
    ComPtr<IXmlNodeList> nodeList;
    HRESULT hr = _xmlDocument->GetElementsByTagName(WinToastStringWrapper(L"text").Get(), &nodeList);
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNode> node;
        hr = nodeList->Item(pos, &node);
        if (SUCCEEDED(hr)) {
            hr = Util::setNodeStringValue(text, node.Get(), xmlDocument());
        }
    }
    return hr;
}


HRESULT WinToast::setImageField(_In_ const std::wstring& path)  {
    wchar_t imagePath[MAX_PATH] = L"file:///";
    HRESULT hr = StringCchCat(imagePath, MAX_PATH, path.c_str());
    if (SUCCEEDED(hr)) {
        ComPtr<IXmlNodeList> nodeList;
        HRESULT hr = _xmlDocument->GetElementsByTagName(WinToastStringWrapper(L"image").Get(), &nodeList);
        if (SUCCEEDED(hr)) {
            ComPtr<IXmlNode> node;
            hr = nodeList->Item(0, &node);
            if (SUCCEEDED(hr))  {
                ComPtr<IXmlNamedNodeMap> attributes;
                hr = node->get_Attributes(&attributes);
                if (SUCCEEDED(hr)) {
                    ComPtr<IXmlNode> editedNode;
                    hr = attributes->GetNamedItem(WinToastStringWrapper(L"src").Get(), &editedNode);
                    if (SUCCEEDED(hr)) {
                        Util::setNodeStringValue(imagePath, editedNode.Get(), xmlDocument());
                    }
                }
            }
        }
    }
    return hr;
}

WinToastTemplate::WinToastTemplate(const WinToastTemplateType& type) :
    _type(type)
{
    initComponentsFromType();
}


WinToastTemplate::~WinToastTemplate()
{
    _textFields.clear();
}

void WinToastTemplate::setTextField(_In_ const std::wstring& txt, _In_ const WinToastTemplate::TextField& pos) {
    _textFields[pos] = txt;
}
void WinToastTemplate::setImagePath(_In_ const std::wstring& imgPath) {
    if (!_hasImage)
        return;
    _imagePath = imgPath;
}

int WinToastTemplate::TextFieldsCount[WinToastTemplateTypeCount] = { 1, 2, 2, 3, 1, 2, 2, 3};
void WinToastTemplate::initComponentsFromType() {
    _hasImage = _type < ToastTemplateType_ToastText01;
    _textFields = std::vector<std::wstring>(TextFieldsCount[_type], L"");
}


void WinToastHandler::toastActivated() const {
    std::wcout << L"The user clicked in this toast" << std::endl;

}

void WinToastHandler::toastFailed() const {
    std::wcout << L"Error showing current toast" << std::endl;
}

void WinToastHandler::toastDismissed(WinToastHandler::WinToastDismissalReason state) const {
    switch (state) {
    case UserCanceled:
        std::wcout << L"The user dismissed this toast" << std::endl;
        break;
    case ApplicationHidden:
        std::wcout <<  L"The application hid the toast using ToastNotifier.hide()" << std::endl;
        break;
    case TimedOut:
        std::wcout << L"The toast has timed out" << std::endl;
        break;
    default:
        std::wcout << L"Toast not activated" << std::endl;
        break;
    }
}


WinToastStringWrapper::WinToastStringWrapper(_In_reads_(length) PCWSTR stringRef, _In_ UINT32 length) throw() {
    HRESULT hr = DllImporter::WindowsCreateStringReference(stringRef, length, &_header, &_hstring);
    if (!SUCCEEDED(hr)) {
        RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
    }
}


WinToastStringWrapper::WinToastStringWrapper(const std::wstring &stringRef)
{
    HRESULT hr = DllImporter::WindowsCreateStringReference(stringRef.c_str(), static_cast<UINT32>(stringRef.length()), &_header, &_hstring);
    if (FAILED(hr)) {
        RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
    }
}

WinToastStringWrapper::~WinToastStringWrapper() {
    DllImporter::WindowsDeleteString(_hstring);
}

HSTRING WinToastStringWrapper::Get() const {
    return _hstring;
}
