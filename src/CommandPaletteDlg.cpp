﻿// This file is part of BowPad.
//
// Copyright (C) 2013, 2015-2017, 2020 - Stefan Kueng
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// See <http://www.gnu.org/licenses/> for a copy of the full license text
//
#include "stdafx.h"
#include "CommandPaletteDlg.h"
#include "CommandHandler.h"
#include "KeyboardShortcutHandler.h"
#include "resource.h"
#include "AppUtils.h"
#include "CmdScripts.h"
#include "StringUtils.h"
#include "Theme.h"
#include <string>
#include <algorithm>
#include <memory>
#include <Commdlg.h>

extern HINSTANCE hRes;

CCommandPaletteDlg::CCommandPaletteDlg(HWND hParent)
    : m_hParent(hParent)
    , m_hFilter(nullptr)
    , m_hResults(nullptr)
{
}

CCommandPaletteDlg::~CCommandPaletteDlg()
{
}

void CCommandPaletteDlg::ClearFilterText()
{
    SetWindowText(m_hFilter, L"");
    FillResults();
}

LRESULT CCommandPaletteDlg::DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            CTheme::Instance().RegisterThemeChangeCallback(
                [this]() {
                    CTheme::Instance().SetThemeForDialog(*this, CTheme::Instance().IsDarkTheme());
                });
            CTheme::Instance().SetThemeForDialog(*this, CTheme::Instance().IsDarkTheme());
            InitDialog(hwndDlg, IDI_BOWPAD, false);
            CTheme::Instance().SetThemeForDialog(*this, CTheme::Instance().IsDarkTheme());
            // initialize the controls
            m_hFilter  = GetDlgItem(*this, IDC_FILTER);
            m_hResults = GetDlgItem(*this, IDC_RESULTS);

            ResString sFilterCue(hRes, IDS_COMMANDPALETTE_FILTERCUE);
            SendMessage(m_hFilter, EM_SETCUEBANNER, 1, (LPARAM)sFilterCue.c_str());
            SetWindowSubclass(m_hFilter, EditSubClassProc, 0, reinterpret_cast<DWORD_PTR>(this));

            const auto& resourceData = CKeyboardShortcutHandler::Instance().GetResourceData();
            const auto& commands     = CCommandHandler::Instance().GetCommands();
            for (const auto& cmd : commands)
            {
                auto whereAt = std::find_if(resourceData.begin(), resourceData.end(),
                                            [&](const auto& item) { return ((UINT)item.second == cmd.first); });
                if (whereAt != resourceData.end())
                {
                    CmdPalData data;
                    data.cmdId = cmd.first;

                    auto sID = whereAt->first + L"_TooltipDescription_RESID";

                    auto ttIDit = resourceData.find(sID);
                    if (ttIDit != resourceData.end())
                    {
                        auto sRes        = LoadResourceWString(hRes, ttIDit->second);
                        data.description = sRes;
                    }

                    sID = whereAt->first + L"_LabelTitle_RESID";

                    auto ltIDit = resourceData.find(sID);
                    if (ltIDit != resourceData.end())
                    {
                        auto sRes    = LoadResourceWString(hRes, ltIDit->second);
                        data.command = sRes;
                        SearchReplace(data.command, L"&", L"");
                    }

                    data.shortcut = CKeyboardShortcutHandler::Instance().GetShortCutStringForCommand((WORD)cmd.first);
                    if (!data.command.empty())
                        m_allResults.push_back(data);
                }
                else
                {
                    auto pScript = dynamic_cast<CCmdScript*>(cmd.second.get());
                    if (pScript)
                    {
                        CmdPalData data;
                        data.cmdId   = cmd.first;
                        data.command = CCommandHandler::Instance().GetPluginMap().at(cmd.first);
                        SearchReplace(data.command, L"&", L"");
                        data.shortcut = CKeyboardShortcutHandler::Instance().GetShortCutStringForCommand((WORD)cmd.first);
                        if (!data.command.empty())
                            m_allResults.push_back(data);
                    }
                }
            }
            const auto& noDelCommands = CCommandHandler::Instance().GetNoDeleteCommands();
            for (const auto& cmd : noDelCommands)
            {
                auto whereAt = std::find_if(resourceData.begin(), resourceData.end(),
                                            [&](const auto& item) { return ((UINT)item.second == cmd.first); });
                if (whereAt != resourceData.end())
                {
                    CmdPalData data;
                    data.cmdId = cmd.first;

                    auto sID = whereAt->first + L"_TooltipDescription_RESID";

                    auto ttIDit = resourceData.find(sID);
                    if (ttIDit != resourceData.end())
                    {
                        auto sRes        = LoadResourceWString(hRes, ttIDit->second);
                        data.description = sRes;
                    }

                    sID = whereAt->first + L"_LabelTitle_RESID";

                    auto ltIDit = resourceData.find(sID);
                    if (ltIDit != resourceData.end())
                    {
                        auto sRes    = LoadResourceWString(hRes, ltIDit->second);
                        data.command = sRes;
                        SearchReplace(data.command, L"&", L"");
                    }

                    data.shortcut = CKeyboardShortcutHandler::Instance().GetShortCutStringForCommand((WORD)cmd.first);
                    if (!data.command.empty())
                        m_allResults.push_back(data);
                }
            }
            std::sort(m_allResults.begin(), m_allResults.end(),
                      [](const CmdPalData& a, const CmdPalData& b) -> bool {
                          return StrCmpLogicalW(a.command.c_str(), b.command.c_str()) < 0;
                      });
            m_resizer.UseSizeGrip(false);
            m_resizer.Init(hwndDlg);
            m_resizer.UseSizeGrip(!CTheme::Instance().IsDarkTheme());
            m_resizer.AddControl(hwndDlg, IDC_FILTER, RESIZER_TOPLEFTRIGHT);
            m_resizer.AddControl(hwndDlg, IDC_RESULTS, RESIZER_TOPLEFTBOTTOMRIGHT);
            InvalidateRect(*this, nullptr, true);
            InitResultsList();
            FillResults();
        }
            return TRUE;
        case WM_COMMAND:
            return DoCommand(LOWORD(wParam), HIWORD(wParam));
        case WM_NOTIFY:
            switch (wParam)
            {
                case IDC_RESULTS:
                    return DoListNotify((LPNMITEMACTIVATE)lParam);
                    break;
                default:
                    return FALSE;
            }
            break;
        case WM_TIMER:
        {
            FillResults();
            KillTimer(*this, 101);
        }
        break;
        case WM_ACTIVATE:
            if (wParam == WA_INACTIVE)
            {
                ShowWindow(*this, SW_HIDE);
            }
            else
            {
                SetFocus(m_hFilter);
            }
            break;
        case WM_SIZE:
        {
            int newWidth  = LOWORD(lParam);
            int newHeight = HIWORD(lParam);
            m_resizer.DoResize(newWidth, newHeight);
        }
        break;
    }
    return FALSE;
}

LRESULT CCommandPaletteDlg::DoCommand(int id, int code)
{
    switch (id)
    {
        case IDOK:
        {
            auto i = ListView_GetSelectionMark(m_hResults);
            if (i >= 0)
            {
                const auto& data = m_results[i];
                SendMessage(m_hParent, WM_COMMAND, MAKEWPARAM(data.cmdId, 1), 0);
            }
        }
        break;
        case IDCANCEL:
            ShowWindow(*this, SW_HIDE);
            break;
        case IDC_FILTER:
            if (code == EN_CHANGE)
                SetTimer(*this, 101, 50, nullptr);
            break;
    }
    return 1;
}

void CCommandPaletteDlg::InitResultsList()
{
    SetWindowTheme(m_hResults, L"Explorer", nullptr);
    ListView_SetItemCountEx(m_hResults, 0, 0);

    auto hListHeader = ListView_GetHeader(m_hResults);
    int  c           = Header_GetItemCount(hListHeader) - 1;
    while (c >= 0)
        ListView_DeleteColumn(m_hResults, c--);

    DWORD exStyle = LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT;
    ListView_SetExtendedListViewStyle(m_hResults, exStyle);

    LVCOLUMN lvc{};
    lvc.mask = LVCF_FMT;
    lvc.fmt  = LVCFMT_LEFT;
    ListView_InsertColumn(m_hResults, 0, &lvc);
    lvc.fmt = LVCFMT_RIGHT;
    ListView_InsertColumn(m_hResults, 1, &lvc);
    lvc.fmt = LVCFMT_LEFT;
    ListView_InsertColumn(m_hResults, 2, &lvc);

    ListView_SetColumnWidth(m_hResults, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(m_hResults, 1, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(m_hResults, 2, LVSCW_AUTOSIZE);
}

LRESULT CCommandPaletteDlg::DoListNotify(LPNMITEMACTIVATE lpNMItemActivate)
{
    switch (lpNMItemActivate->hdr.code)
    {
        case LVN_GETINFOTIP:
        {
            LPNMLVGETINFOTIP tip       = (LPNMLVGETINFOTIP)lpNMItemActivate;
            int              itemIndex = (size_t)tip->iItem;
            if (itemIndex < 0 || itemIndex >= (int)m_results.size())
            {
                assert(false);
                return 0;
            }
            const auto& data = m_results[itemIndex];
            if (data.description.empty())
            {
                _snwprintf_s(tip->pszText, tip->cchTextMax, _TRUNCATE, L"%s\t%s",
                             data.command.c_str(), data.shortcut.c_str());
            }
            else
            {
                wcscpy_s(tip->pszText, tip->cchTextMax, data.description.c_str());
            }
        }
        break;
        case LVN_GETDISPINFO:
            // Note: the listview is Owner Draw which means you draw it,
            // but you still need to tell the list what exactly the text is
            // that you intend to draw or things like clicking the header to
            // re-size won't work.
            return GetListItemDispInfo(reinterpret_cast<NMLVDISPINFO*>(lpNMItemActivate));

        case NM_RETURN:
        case NM_DBLCLK:
            // execute the selected command
            if (lpNMItemActivate->iItem >= 0 && lpNMItemActivate->iItem < (int)m_results.size())
            {
                const auto& data = m_results[lpNMItemActivate->iItem];
                SendMessage(m_hParent, WM_COMMAND, MAKEWPARAM(data.cmdId, 1), 0);
            }
            break;
        case NM_CUSTOMDRAW:
            return DrawListItem(reinterpret_cast<NMLVCUSTOMDRAW*>(lpNMItemActivate));
            break;
    }
    return 0;
}

LRESULT CCommandPaletteDlg::DrawListItem(NMLVCUSTOMDRAW* pLVCD)
{
    switch (pLVCD->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;
        case CDDS_ITEMPREPAINT:
            // Tell Windows to send draw notifications for each subitem.
            return CDRF_NOTIFYSUBITEMDRAW;
        case CDDS_ITEMPREPAINT | CDDS_ITEM | CDDS_SUBITEM:
        {
            pLVCD->clrText = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_WINDOWTEXT));
            if (ListView_GetItemState(m_hResults, pLVCD->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED)
                pLVCD->clrText = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_HOTLIGHT));
        }
        break;
    }
    return CDRF_DODEFAULT;
}

LRESULT CCommandPaletteDlg::GetListItemDispInfo(NMLVDISPINFO* pDispInfo)
{
    if ((pDispInfo->item.mask & LVIF_TEXT) != 0)
    {
        if (pDispInfo->item.pszText == nullptr)
            return 0;
        pDispInfo->item.pszText[0] = 0;
        int itemIndex              = pDispInfo->item.iItem;
        if (itemIndex >= (int)m_results.size())
            return 0;

        std::wstring sTemp;
        const auto&  item = m_results[itemIndex];
        switch (pDispInfo->item.iSubItem)
        {
            case 0:
                lstrcpyn(pDispInfo->item.pszText, item.command.c_str(), pDispInfo->item.cchTextMax);
                break;

            case 1:
                lstrcpyn(pDispInfo->item.pszText, item.shortcut.c_str(), pDispInfo->item.cchTextMax);
                break;
            case 2:
                lstrcpyn(pDispInfo->item.pszText, item.description.c_str(), pDispInfo->item.cchTextMax);
                break;
        }
    }
    return 0;
}

void CCommandPaletteDlg::FillResults()
{
    static std::wstring lastFilterText;
    auto                filterText  = GetDlgItemText(IDC_FILTER);
    std::wstring        sFilterText = filterText.get();
    if (lastFilterText.empty() || _wcsicmp(sFilterText.c_str(), lastFilterText.c_str()))
    {
        m_results.clear();
        int col1Width = 0;
        int col2Width = 0;
        for (const auto& cmd : m_allResults)
        {
            if (sFilterText.empty() || StrStrIW(cmd.command.c_str(), sFilterText.c_str()))
            {
                col1Width = max(col1Width, ListView_GetStringWidth(m_hResults, cmd.command.c_str()));
                col2Width = max(col2Width, ListView_GetStringWidth(m_hResults, cmd.shortcut.c_str()));
                m_results.push_back(cmd);
            }
        }
        lastFilterText = sFilterText;
        ListView_SetItemCountEx(m_hResults, m_results.size(), 0);
        ListView_SetColumnWidth(m_hResults, 0, col1Width + 50LL);
        ListView_SetColumnWidth(m_hResults, 1, col2Width + 50LL);
        ListView_SetColumnWidth(m_hResults, 2, LVSCW_AUTOSIZE);
    }
}

LRESULT CALLBACK CCommandPaletteDlg::EditSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    CCommandPaletteDlg* pThis = reinterpret_cast<CCommandPaletteDlg*>(dwRefData);
    switch (uMsg)
    {
        case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hWnd, EditSubClassProc, uIdSubclass);
            break;
        }
        case WM_KEYDOWN:
            if (wParam == VK_DOWN)
            {
                auto i = ListView_GetSelectionMark(pThis->m_hResults);
                ListView_SetItemState(pThis->m_hResults, i, 0, LVIS_SELECTED);
                ListView_SetSelectionMark(pThis->m_hResults, i + 1);
                ListView_SetItemState(pThis->m_hResults, i + 1, LVIS_SELECTED, LVIS_SELECTED);
                ListView_EnsureVisible(pThis->m_hResults, i + 1, false);
                return 0;
            }
            if (wParam == VK_UP)
            {
                auto i = ListView_GetSelectionMark(pThis->m_hResults);
                if (i > 0)
                {
                    ListView_SetItemState(pThis->m_hResults, i, 0, LVIS_SELECTED);
                    ListView_SetSelectionMark(pThis->m_hResults, i - 1);
                    ListView_SetItemState(pThis->m_hResults, i - 1, LVIS_SELECTED, LVIS_SELECTED);
                    ListView_EnsureVisible(pThis->m_hResults, i - 1, false);
                }
                return 0;
            }
            if (wParam == VK_RETURN)
            {
                auto i = ListView_GetSelectionMark(pThis->m_hResults);
                if (i >= 0)
                {
                    const auto& data = pThis->m_results[i];
                    SendMessage(pThis->m_hParent, WM_COMMAND, MAKEWPARAM(data.cmdId, 1), 0);
                }
            }
            break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
