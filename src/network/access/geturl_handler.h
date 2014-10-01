// Copyright 2011 The Native Client SDK Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef GETURL_HANDLER_H_
#define GETURL_HANDLER_H_

#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/url_loader.h>
#include <ppapi/cpp/url_request_info.h>
#include <ppapi/utility/completion_callback_factory.h>
#include <string>
#include <vector>
#include "qnetworkreplypepperimpl_p.h"

// GetURLHandler is used to download data from |url|. When download is
// finished or when an error occurs, it calls JavaScript function
// reportResult(url, result, success) (defined in geturl.html) and
// self-destroys.
//
// EXAMPLE USAGE:
// GetURLHandler* handler* = GetURLHandler::Create(url);
// if (!handler->Start())
//   delete handler;
//
//
class GetURLHandler {
 public:
  // Helper class to run a method as a callback when the URL download is
  // complete.  This executes a method with the signature:
  //   void Method(const std::vector<uint8_t>& data, int32_t error);
  // |data| is a pointer to the newly-downloaded data buffer.
  // |data_byte_count| is the size in bytes of the dta buffer.
  // |error| indicates any errors that occured in the download; it has value
  // PP_OK on success.
  //
  // The callback is called from the GetURLHandler instance as soon as the URL's
  // data is all downloaded.  When the callback returns, |data| can be
  // deallocated, as can the calling GetURLHandler object.  If you want to
  // retain |data|, you must make a copy of it.
  class URLCallbackExecutor {
   public:
    virtual ~URLCallbackExecutor() {}
    virtual void Execute(const std::vector<uint8_t>& data, int32_t error) = 0;
  };

  template <class T>
  class URLCallback : public URLCallbackExecutor {
   public:
    typedef void (T::*Method)(const std::vector<uint8_t>& data, int32_t error);

    URLCallback(T* instance, Method method)
        : instance_(instance), method_(method) {}
    virtual ~URLCallback() {}
    virtual void Execute(const std::vector<uint8_t>& data,
                         int32_t error) {
      // Use "this->" to force C++ to look inside our templatized base class;
      // see Effective C++, 3rd Ed, item 43, p210 for details.
      return ((this->instance_)->*(this->method_))(data, error);
    }

   private:
    T* instance_;
    Method method_;
  };

  // Creates instance of GetURLHandler on the heap.
  // GetURLHandler objects shall be created only on the heap (they
  // self-destroy when all data is in).
  static GetURLHandler* Create(pp::Instance* instance, const std::string& url, QNetworkReplyPepperImplPrivate *d);
  // Initiates page (URL) download.
  // Returns false in case of internal error, and self-destroys.
  bool Start();

 private:
  static const int kBufferSize = 4096;

  GetURLHandler(pp::Instance* instance, const std::string& url, QNetworkReplyPepperImplPrivate *d);
  ~GetURLHandler();

  // Callback for pp::URLLoader::Open().
  // Called by pp::URLLoader when response headers are received or when an
  // error occurs (in response to the call of pp::URLLoader::Open()).
  // Look at <ppapi/c/ppb_url_loader.h> and
  // <ppapi/cpp/url_loader.h> for more information about pp::URLLoader.
  void OnOpen(int32_t result);

  // Callback fo the pp::URLLoader::ReadResponseBody().
  // |result| contains the number of bytes read or an error code.
  // Appends data from this->buffer_ to this->url_response_body_.
  void OnRead(int32_t result);

  // Reads the response body (asynchronously) into this->buffer_.
  // OnRead() will be called when bytes are received or when an error occurs.
  void ReadBody();

  void DownloadFinished(int32_t state);

  std::string url_;  // URL to be downloaded.
  pp::URLRequestInfo url_request_;
  pp::URLLoader url_loader_;  // URLLoader provides an API to download URLs.
  char buffer_[kBufferSize];  // buffer for pp::URLLoader::ReadResponseBody().
  std::vector<uint8_t> url_response_body_;  // Contains downloaded data.
  pp::CompletionCallbackFactory<GetURLHandler> cc_factory_;
  QNetworkReplyPepperImplPrivate *d;

  GetURLHandler(const GetURLHandler&);
  void operator=(const GetURLHandler&);
};

#endif  // EXAMPLES_GRAPHICS_PHOTO_GETURL_HANDLER_H_

