// Copyright 2011 The Native Client SDK Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "geturl_handler.h"

#include <ppapi/c/pp_errors.h>
#include <ppapi/c/ppb_instance.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/url_response_info.h>
#include <ppapi/cpp/var.h>
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <QtCore>
#include "qnetworkreplypepperimpl_p.h"


namespace {
// HTTP status values.
const int32_t kHTTPStatusOK = 200;
}  // namespace

GetURLHandler* GetURLHandler::Create(pp::Instance* instance,
                                     const std::string& url,
                                     QNetworkReplyPepperImplPrivate *d) {
  return new GetURLHandler(instance, url, d);
}

GetURLHandler::GetURLHandler(pp::Instance* instance,
                             const std::string& url,
                             QNetworkReplyPepperImplPrivate *d)
    : url_(url),
      url_request_(instance),
      url_loader_(instance),
      cc_factory_(this),
      d(d) {
  url_request_.SetURL(url);
  url_request_.SetMethod("GET");
}

GetURLHandler::~GetURLHandler() {
}

bool GetURLHandler::Start() {
  //url_callback_ = url_callback;
  pp::CompletionCallback cc = cc_factory_.NewCallback(&GetURLHandler::OnOpen);
  int32_t result = url_loader_.Open(url_request_, cc);
  // A |result| value of PP_OK_COMPLETIONPENDING means the Open() call is not
  // able to complete synchronously.  In this case, it is not necessarily an
  // error, it just means that Open() will do its operation asynchronously and
  // call |cc| when done.  In the event that Open() is able to complete
  // synchronously (either because an error occurred, or it just worked), |cc|
  // needs to be run here directly.
  if (PP_OK_COMPLETIONPENDING != result)
    cc.Run(result);

  return result == PP_OK || result == PP_OK_COMPLETIONPENDING;
}

void GetURLHandler::OnOpen(int32_t result) {
if (result != PP_OK) {
    DownloadFinished(result);
  } else {
    // Process the response, validating the headers to confirm successful
    // loading.
    pp::URLResponseInfo url_response(url_loader_.GetResponseInfo());
    if (url_response.is_null()) {
      DownloadFinished(PP_ERROR_FILENOTFOUND);
      return;
    }
    int32_t status_code = url_response.GetStatusCode();
    if (status_code != kHTTPStatusOK) {
      DownloadFinished(PP_ERROR_FILENOTFOUND);
      return;
    }
    ReadBody();
  }
}

void GetURLHandler::OnRead(int32_t result) {
    if (result > 0) {
    // In this case, |result| is the number of bytes read.  Copy these into
    // the local buffer and continue.
    int32_t num_bytes = result < kBufferSize ? result : sizeof(buffer_);
    d->data.append(buffer_, num_bytes);

    ReadBody();
  } else {
    // Either the end of the file was reached (|result| == PP_OK) or there
    // was an error.  Execute the callback in either case.
    DownloadFinished(result);
  }
}

void GetURLHandler::ReadBody() {
  // Reads the response body (asynchronous) into this->buffer_.
  // OnRead() will be called when bytes are received or when an error occurs.
  // Look at <ppapi/c/dev/ppb_url_loader> for more details.
  pp::CompletionCallback cc = cc_factory_.NewCallback(&GetURLHandler::OnRead);
  int32_t res = url_loader_.ReadResponseBody(buffer_,
                                             sizeof(buffer_),
                                             cc);
  if (PP_OK_COMPLETIONPENDING != res)
    cc.Run(res);
}

void GetURLHandler::DownloadFinished(int32_t state)
{
    //qDebug() << "GetURLHandler::DownloadFinished" << d->url << state;
    d->state = state;
    QMutexLocker lock(&d->mutex);
    d->dataReady.wakeAll();
}
