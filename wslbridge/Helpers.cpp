/* 
 * This file is part of wslbridge2 project
 * Licensed under the GNU General Public License version 3
 * Copyright (C) 2019 Biswapriyo Nath
 */

#include <windows.h>
#include <sys/cygwin.h>
#include <sys/ioctl.h>

#include <algorithm>

#include "SocketIo.hpp"

struct TermSize terminalSize(void)
{
    winsize ws = {};
    if (isatty(STDIN_FILENO) &&
        ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0)
    {
        return TermSize { ws.ws_col, ws.ws_row };
    }
    else
    {
        return TermSize { 80, 24 };
    }
}

std::wstring mbsToWcs(const std::string &s)
{
    const size_t len = mbstowcs(nullptr, s.c_str(), 0);
    if (len == static_cast<size_t>(-1))
        fatal("error: mbsToWcs: invalid string\n");
    
    std::wstring ret;
    ret.resize(len);
    const size_t len2 = mbstowcs(&ret[0], s.c_str(), len);
    assert(len == len2);
    return ret;
}

std::string wcsToMbs(const std::wstring &s, bool emptyOnError=false)
{
    const size_t len = wcstombs(nullptr, s.c_str(), 0);
    if (len == static_cast<size_t>(-1))
    {
        if (emptyOnError)
            return {};

        fatal("error: wcsToMbs: invalid string\n");
    }
    std::string ret;
    ret.resize(len);
    const size_t len2 = wcstombs(&ret[0], s.c_str(), len);
    assert(len == len2);
    return ret;
}

std::wstring dirname(const std::wstring &path)
{
    std::wstring::size_type pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
        return L"";
    else
        return path.substr(0, pos);
}

HMODULE getCurrentModule(void)
{
    HMODULE module;
    if (!GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(getCurrentModule),
                &module))
    {
        fatal("error: GetModuleHandleEx failed\n");
    }
    return module;
}

std::wstring getModuleFileName(HMODULE module)
{
    const int bufsize = 4096;
    wchar_t path[bufsize];
    int size = GetModuleFileNameW(module, path, bufsize);
    assert(size != 0 && size != bufsize);
    return std::wstring(path);
}

bool pathExists(const std::wstring &path)
{
    return GetFileAttributesW(path.c_str()) != 0xFFFFFFFF;
}

wchar_t lowerDrive(wchar_t ch)
{
    if (ch >= L'a' && ch <= L'z')
        return ch;
    else if (ch >= L'A' && ch <= 'Z')
        return ch - L'A' + L'a';
    else
        return L'\0';
}

std::wstring findSystemProgram(const wchar_t *name)
{
    std::array<wchar_t, MAX_PATH> windir;
    windir[0] = L'\0';
    if (GetWindowsDirectoryW(windir.data(), windir.size()) == 0)
        fatal("error: GetWindowsDirectory call failed\n");

    const wchar_t *const kPart32 = L"\\System32\\";
    const auto path = [&](const wchar_t *part) -> std::wstring {
        return std::wstring(windir.data()) + part + name;
    };

    const auto ret = path(kPart32);
    if (pathExists(ret))
    {
        return ret;
    }
    else
    {
        fatal("error: '%s' does not exist\n"
              "note: Ubuntu-on-Windows must be installed\n",
              wcsToMbs(ret).c_str());
    }
}

std::pair<std::wstring, std::wstring>
normalizePath(const std::wstring &path)
{
    const auto getFinalPathName = [&](HANDLE h) -> std::wstring {
        std::wstring ret;
        ret.resize(MAX_PATH + 1);
        while (true)
        {
            const auto sz = GetFinalPathNameByHandleW(h, &ret[0], ret.size(), 0);
            if (sz == 0)
            {
                fatal("error: GetFinalPathNameByHandle failed on '%s'\n",
                    wcsToMbs(path).c_str());
            }
            else if (sz < ret.size())
            {
                ret.resize(sz);
                return ret;
            }
            else
            {
                assert(sz > ret.size());
                ret.resize(sz);
            }
        }
    };
    const HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        fatal("error: could not open '%s'\n", wcsToMbs(path).c_str());
    }

    auto npath = getFinalPathName(hFile);
    std::array<wchar_t, MAX_PATH + 1> fsname;
    fsname.back() = L'\0';
    if (!GetVolumeInformationByHandleW(
            hFile, nullptr, 0, nullptr, nullptr, nullptr,
            &fsname[0], fsname.size()))
    {
        fsname[0] = L'\0';
    }
    CloseHandle(hFile);

    /*
     * Example of GetFinalPathNameByHandle result:
     *   \\?\C:\cygwin64\bin\wslbridge-backend
     *   0123456
     *   \\?\UNC\server\share\file
     *   01234567
     */
    if (npath.size() >= 7 &&
            npath.substr(0, 4) == L"\\\\?\\" &&
            lowerDrive(npath[4]) &&
            npath.substr(5, 2) == L":\\") {
        /* Strip off the atypical \\?\ prefix. */
        npath = npath.substr(4);
    } else if (npath.substr(0, 8) == L"\\\\?\\UNC\\") {
        /* Strip off the \\\\?\\UNC\\ prefix and replace it with \\. */
        npath = L"\\\\" + npath.substr(8);
    }
    return std::make_pair(std::move(npath), fsname.data());
}

std::wstring findBackendProgram(const std::string &customBackendPath, const wchar_t *const backendName)
{
    std::wstring ret;
    if (!customBackendPath.empty())
    {
        char *winPath = static_cast<char*>(
            cygwin_create_path(CCP_POSIX_TO_WIN_A, customBackendPath.c_str()));
        if (winPath == nullptr)
            fatalPerror(("error: bad path: '" + customBackendPath + "'").c_str());

        ret = mbsToWcs(winPath);
        free(winPath);
    }
    else
    {
        std::wstring progDir = dirname(getModuleFileName(getCurrentModule()));
        ret = progDir + L"\\" + backendName;
    }

    if (!pathExists(ret))
    {
        fatal("error: '%s' backend program is missing\n",
              wcsToMbs(ret).c_str());
    }

    return ret;
}

void appendWslArg(std::wstring &out, const std::wstring &arg)
{
    if (!out.empty())
    {
        out.push_back(L' ');
    }

    const auto isCharSafe = [](wchar_t ch) -> bool {
        switch (ch)
        {
            case L'%':
            case L'+':
            case L',':
            case L'-':
            case L'.':
            case L'/':
            case L':':
            case L'=':
            case L'@':
            case L'_':
            case L'{':
            case L'}':
                return true;
            default:
                return (ch >= L'0' && ch <= L'9') ||
                       (ch >= L'a' && ch <= L'z') ||
                       (ch >= L'A' && ch <= L'Z');
        }
    };
    if (arg.empty())
    {
        out.append(L"''");
        return;
    }

    if (std::all_of(arg.begin(), arg.end(), isCharSafe))
    {
        out.append(arg);
        return;
    }
    bool inQuote = false;
    const auto enterQuote = [&](bool newInQuote) {
        if (inQuote != newInQuote) {
            out.push_back(L'\'');
            inQuote = newInQuote;
        }
    };

    enterQuote(true);
    for (auto ch : arg)
    {
        if (ch == L'\'')
        {
            enterQuote(false);
            out.append(L"\\'");
            enterQuote(true);
        }
        else if (isCharSafe(ch))
        {
            out.push_back(ch);
        }
        else
        {
            out.push_back(ch);
        }
    }
    enterQuote(false);
}
