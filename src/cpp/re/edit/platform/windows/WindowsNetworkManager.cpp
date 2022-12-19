/*
 * Copyright (c) 2022 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#include "WindowsNetworkManager.h"
#include <Windows.h>
#include <winhttp.h>

// hack to remove the definition of min as a "define"
#undef min

#include "../../Utils.h"
#include "../../Errors.h"

namespace re::edit {

//------------------------------------------------------------------------
// WindowsNetworkManager::HttpGet
//------------------------------------------------------------------------
std::optional<std::string> WindowsNetworkManager::HttpGet(std::wstring const &iUrl,
                                                          std::map<std::wstring, std::wstring> const &iHeaders) const
{
  URL_COMPONENTS urlComp;
  // Initialize the URL_COMPONENTS structure.
  ZeroMemory(&urlComp, sizeof(urlComp));
  urlComp.dwStructSize = sizeof(urlComp);

  // Set required component lengths to non-zero
  // so that they are cracked.
  urlComp.dwSchemeLength    = (DWORD)-1;
  urlComp.dwHostNameLength  = (DWORD)-1;
  urlComp.dwUrlPathLength   = (DWORD)-1;
  urlComp.dwExtraInfoLength = (DWORD)-1;

  if(!WinHttpCrackUrl(iUrl.c_str(), 0, 0, &urlComp))
  {
    RE_EDIT_LOG_DEBUG("Error %lu in WinHttpCrackUrl.\n", GetLastError());
    return std::nullopt;
  }

  std::wstring hostName{urlComp.lpszHostName, urlComp.dwHostNameLength};
  hostName.append(L"\0");

  std::wstring path{urlComp.lpszUrlPath, urlComp.dwUrlPathLength};
  path.append(L"\0");

  // Use WinHttpOpen to obtain a session handle.
  auto hSession = WinHttpOpen(L"re-edit / HttpGet",
                              WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                              WINHTTP_NO_PROXY_NAME,
                              WINHTTP_NO_PROXY_BYPASS, 0);

  if(!hSession)
  {
    RE_EDIT_LOG_DEBUG("Error %lu in WinHttpOpen.\n", GetLastError());
    return std::nullopt;
  }

  auto closeHSession = Utils::defer([hSession]() { if(hSession) WinHttpCloseHandle(hSession); });

  auto hConnect = WinHttpConnect(hSession, hostName.c_str(),
                                 INTERNET_DEFAULT_HTTPS_PORT, 0);
  if(!hConnect)
  {
    RE_EDIT_LOG_DEBUG("Error %lu in WinHttpConnect.\n", GetLastError());
    return std::nullopt;
  }

  auto closeHConnect = Utils::defer([hConnect]() { if(hConnect) WinHttpCloseHandle(hConnect); });

  // Create an HTTP request handle.
  auto hRequest = WinHttpOpenRequest(hConnect, L"GET", urlComp.lpszUrlPath,
                                     path.c_str(), WINHTTP_NO_REFERER,
                                     WINHTTP_DEFAULT_ACCEPT_TYPES,
                                     WINHTTP_FLAG_SECURE);

  if(!hRequest)
  {
    RE_EDIT_LOG_DEBUG("Error %lu in WinHttpOpenRequest.\n", GetLastError());
    return std::nullopt;
  }

  auto closeHRequest = Utils::defer([hRequest]() { if(hRequest) WinHttpCloseHandle(hRequest); });

  // Send a request.
  for(auto &[header, value]: iHeaders)
  {
    auto h = header;
    h.append(L": ").append(value);
    if(!WinHttpAddRequestHeaders(hRequest,
                                 h.c_str(),
                                 (ULONG) -1L,
                                 WINHTTP_ADDREQ_FLAG_ADD))
      return std::nullopt;
  }

  if(!WinHttpSendRequest(hRequest,
                         WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                         WINHTTP_NO_REQUEST_DATA, 0,
                         0, 0))
    return std::nullopt;


  // End the request.
  if(!WinHttpReceiveResponse( hRequest, NULL ))
    return std::nullopt;

  std::string res{};

  while(true)
  {
    constexpr DWORD BUFFER_SIZE = 1024;

    char buffer[1024]{};
    DWORD sizeAvailable = 0;
    if(!WinHttpQueryDataAvailable(hRequest, &sizeAvailable))
    {
      RE_EDIT_LOG_DEBUG("Error %lu in WinHttpQueryDataAvailable.\n", GetLastError());
      return std::nullopt;
    }

    if(sizeAvailable == 0)
    {
      res.append("\0");
      return res;
    }

    DWORD sizeDownloaded = 0;

    if(!WinHttpReadData(hRequest, (LPVOID) buffer, std::min(sizeAvailable, BUFFER_SIZE), &sizeDownloaded))
    {
      RE_EDIT_LOG_DEBUG("Error %lu in WinHttpReadData.\n", GetLastError());
      return std::nullopt;
    }

    if(sizeDownloaded > 0)
    {
      res.append(buffer, sizeDownloaded);
    }
  }
}

}