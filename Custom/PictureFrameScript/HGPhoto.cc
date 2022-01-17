/*
 * \copyright Copyright 2013 Google Inc. All Rights Reserved.
 * \license @{
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @}
 */

//
// When the program starts up you will be asked to authorize by copying
// a URL into a browser and copying the response back. From there the
// program executes a shell that takes commands. Type 'help' for a list
// of current commands and 'quit' to exit.

#include <iostream>
#include <fstream>
#include <regex>
using std::cout;
using std::endl;
using std::ostream;  // NOLINT
#include <memory>
#include "googleapis/client/auth/file_credential_store.h"
#include "googleapis/client/auth/oauth2_authorization.h"
#include "googleapis/client/data/data_reader.h"
#if HAVE_OPENSSL
#include "googleapis/client/data/openssl_codec.h"
#endif
#include "googleapis/client/transport/curl_http_transport.h"
#include "googleapis/client/transport/http_authorization.h"
#include "googleapis/client/transport/http_transport.h"
#include "googleapis/client/transport/http_request_batch.h"
#include "googleapis/client/util/status.h"
#include "googleapis/strings/strcat.h"

#include "google/gmail_api/gmail_api.h"  // NOLINT

#include "json.hpp"
using json = nlohmann::json;
#include "b64/base64.c"

std::string savePath;

namespace googleapis {

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

using client::ClientServiceRequest;
using client::DateTime;
using client::FileCredentialStoreFactory;
using client::HttpRequestBatch;
using client::HttpResponse;
using client::HttpTransport;
using client::HttpTransportLayerConfig;
using client::JsonCppArray;
using client::OAuth2Credential;
using client::OAuth2AuthorizationFlow;
using client::OAuth2RequestOptions;
#if HAVE_OPENSSL
using client::OpenSslCodecFactory;
#endif
using client::StatusCanceled;
using client::StatusInvalidArgument;
using client::StatusOk;

const char kSampleStepPrefix[] = "SAMPLE:  ";

#define HGEMAIL "USER@gmail.com"

bool promptLogin = true;

std::string RunCommand(std::string const &cmd)
{
#if defined(_WIN32)
    std::shared_ptr<FILE> pipe(_popen(cmd.c_str(), "r"), _pclose);
#elif defined(__GNUG__)
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
#else
#error "Must specify a run protocol for this arcetecture."
#endif
    if(!pipe)
        return "";
    std::array<char, 128> buffer;
    std::stringstream ret;
    while (!feof(pipe.get()))
    {
      int c = fgetc(pipe.get());
      if(c < 0 || 255 < c)
        break;
      ret.put(char(c));
	}
    return ret.str();
}

static googleapis::util::Status PromptShellForAuthorizationCode(
    OAuth2AuthorizationFlow* flow,
    const OAuth2RequestOptions& options,
    string* authorization_code) {
      if(!promptLogin)
        return StatusCanceled("Canceled");
  string url = flow->GenerateAuthorizationCodeRequestUrlWithOptions(options);
  string urlclean;
  system(("xdg-open \"" + url + "\"").c_str());

  authorization_code->clear();
  *authorization_code = RunCommand("zenity --entry --text=\"Enter authorization code\"");
  if (authorization_code->empty()) {
    return StatusCanceled("Canceled");
  } else {
    return StatusOk();
  }
}

static googleapis::util::Status ValidateUserName(const string& name) {
  if (name.find("/") != string::npos) {
    return StatusInvalidArgument("UserNames cannot contain '/'");
  } else if (name == "." || name == "..") {
    return StatusInvalidArgument(
        StrCat("'", name, "' is not a valid UserName"));
  }
  return StatusOk();
}

void DisplayError(ClientServiceRequest* request) {
  const HttpResponse& response = *request->http_response();
  std::cout << "====  ERROR  ====" << std::endl;
  std::cout << "Status: " << response.status().error_message() << std::endl;
  if (response.transport_status().ok()) {
    std::cout << "HTTP Status Code = " << response.http_code() << std::endl;
    std::cout << std::endl
              << response.body_reader()->RemainderToString() << std::endl;
  }
  std::cout << std::endl;
}

class HGGMail {
 public:
  static googleapis::util::Status Startup(int argc, char* argv[]);
  void Run();

 private:
  // Gets authorization to access the user's personal calendar data.
  googleapis::util::Status Authorize();

  OAuth2Credential credential_;
  static std::unique_ptr<google_gmail_api::GmailService> service_;
  static std::unique_ptr<OAuth2AuthorizationFlow> flow_;
  static std::unique_ptr<HttpTransportLayerConfig> config_;
};

// static
std::unique_ptr<google_gmail_api::GmailService> HGGMail::service_;
std::unique_ptr<OAuth2AuthorizationFlow> HGGMail::flow_;
std::unique_ptr<HttpTransportLayerConfig> HGGMail::config_;


/* static */
util::Status HGGMail::Startup(int argc, char* argv[]) {
  if ((argc < 2) || (argc > 3)) {
    string error =
        StrCat("Invalid Usage:\n",
               argv[0], " <client_secrets_file> [prompt-login]\n");
    return StatusInvalidArgument(error);
  }

  // Set up HttpTransportLayer.
  googleapis::util::Status status;
  config_.reset(new HttpTransportLayerConfig);
  client::HttpTransportFactory* factory =
      new client::CurlHttpTransportFactory(config_.get());
  config_->ResetDefaultTransportFactory(factory);
  if (argc > 2) {
    if(argv[2][0] == 'f' || argv[2][0] == 'F')
      promptLogin = false;
    //config_->mutable_default_transport_options()->set_cacerts_path(argv[2]);
    // variable saying if allow login or just error
  }

  // Set up OAuth 2.0 flow for getting credentials to access personal data.
  const string client_secrets_file = argv[1];
  flow_.reset(OAuth2AuthorizationFlow::MakeFlowFromClientSecretsPath(
      client_secrets_file, config_->NewDefaultTransportOrDie(), &status));
  if (!status.ok()) return status;

  flow_->set_default_scopes(/*CalendarService::SCOPES::CALENDAR google_gmail_api::GmailService::SCOPES::MAIL_GOOGLE_COM,
    google_gmail_api::GmailService::SCOPES::GMAIL_METADATA, 
    google_gmail_api::GmailService::SCOPES::GMAIL_MODIFY */
    //"https://mail.google.com/, https://www.googleapis.com/auth/gmail.metadata, https://www.googleapis.com/auth/gmail.modify"
    google_gmail_api::GmailService::SCOPES::MAIL_GOOGLE_COM
    );
  flow_->mutable_client_spec()->set_redirect_uri(
      OAuth2AuthorizationFlow::kOutOfBandUrl);
  flow_->set_authorization_code_callback(
      NewPermanentCallback(&PromptShellForAuthorizationCode, flow_.get()));

  string home_path;
  status = FileCredentialStoreFactory::GetSystemHomeDirectoryStorePath(
      &home_path);
  if (status.ok()) {
    FileCredentialStoreFactory store_factory(home_path);
    // Use a credential store to save the credentials between runs so that
    // we dont need to get permission again the next time we run. We are
    // going to encrypt the data in the store, but leave it to the OS to
    // protect access since we do not authenticate users in this sample.
#if HAVE_OPENSSL
    OpenSslCodecFactory* openssl_factory = new OpenSslCodecFactory;
    status = openssl_factory->SetPassphrase(
        flow_->client_spec().client_secret());
    if (!status.ok()) return status;
    store_factory.set_codec_factory(openssl_factory);
#endif

    flow_->ResetCredentialStore(
        store_factory.NewCredentialStore("CalendarSample", &status));
  }
  if (!status.ok()) return status;

  // Now we'll initialize the calendar service proxy that we'll use
  // to interact with the calendar from this sample program.
  HttpTransport* transport = config_->NewDefaultTransport(&status);
  if (!status.ok()) return status;

  service_.reset(new google_gmail_api::GmailService(transport));
  return status;
}


util::Status HGGMail::Authorize() {
  string email;
  email = HGEMAIL;
  if (!email.empty()) {
    googleapis::util::Status status = ValidateUserName(email);
    if (!status.ok()) {
      return status;
    }
  }

  OAuth2RequestOptions options;
  options.email = email;
  googleapis::util::Status status =
        flow_->RefreshCredentialWithOptions(options, &credential_);
  if (!status.ok()) {
    return status;
  }

  credential_.set_flow(flow_.get());
  return StatusOk();
}


void HGGMail::Run() {
  googleapis::util::Status status = Authorize();
  if (!status.ok()) {
    cout << "Error\n";
    return;
  }

{
  
  std::unique_ptr<google_gmail_api::UsersResource_MessagesResource_ListMethod> method( service_->get_users().get_messages().NewListMethod(&credential_, "me") );
  method->clear_include_spam_trash();
  google_gmail_api::ListMessagesResponse *mail = google_gmail_api::ListMessagesResponse::New();
  if(!method->Execute().ok())
  {
    std::cout << "Error\n";
    return;
  }

  string body;
  method->http_response()->GetBodyString(&body);
  json list = json::parse(body.c_str());
  for(auto & msg : list["messages"])
  {
    std::string id = msg["id"].get<std::string>();
    std::unique_ptr<google_gmail_api::UsersResource_MessagesResource_GetMethod> getmethod( service_->get_users().get_messages().NewGetMethod(&credential_, "me", "") );
    getmethod->uri_template_.append( id );
    if(!getmethod->Execute().ok())
    {
      std::cout << "Error\n";
      return;
    }

    getmethod->http_response()->GetBodyString(&body);
    json email = json::parse(body.c_str());
    std::string subject;
    for(auto & head : email["payload"]["headers"])
    {
      if(head["name"].get<std::string>() == "Subject")
      {
        subject = head["value"];
      }
    }

    std::regex reg("magic", std::regex_constants::icase);
    if(!std::regex_search(subject.begin(), subject.end(), reg))
    {
      continue;
    }

    for(auto & part : email["payload"]["parts"])
    {
      if(part["body"]["attachmentId"].is_string())
      {
        std::string aid = part["body"]["attachmentId"].get<std::string>();
        std::unique_ptr<google_gmail_api::UsersResource_MessagesResource_AttachmentsResource_GetMethod> attmethod( service_->get_users().get_messages().get_attachments().NewGetMethod(&credential_, "me", "", "") );
        attmethod->uri_template_ += id + "/attachments/" + aid;
        if(!attmethod->Execute().ok())
        {
          std::cout << "Error\n";
          return;
        }
        attmethod->http_response()->GetBodyString(&body);
        json attach = json::parse(body.c_str());
        size_t decodeLen = 0;
        std::string data = attach["data"].get<std::string>();
        uint8_t *decoded = b64url_decode_with_alloc((uint8_t*)data.c_str(), data.length(), &decodeLen);
        if(decoded)
        {
          std::ofstream out(savePath + std::to_string(std::rand()) + "_" + part["filename"].get<std::string>().c_str());
          out.write((char const*)decoded, data.length());
          free(decoded);
        }
      }
    }

    std::unique_ptr<google_gmail_api::UsersResource_MessagesResource_DeleteMethod> delmethod( service_->get_users().get_messages().NewDeleteMethod(&credential_, "me", "") );
    delmethod->uri_template_.append( id );
    if(!delmethod->Execute().ok())
    {
      std::cout << "Error\n";
      return;
    }
  }

  cout << "Good\n";
}
return;
}


}  // namespace googleapis

using namespace googleapis;
int main(int argc, char* argv[]) {
  savePath.assign(getenv("HOME"));
	savePath.append("/Pictures/");

  googleapis::util::Status status = HGGMail::Startup(argc, argv);
  if (!status.ok()) {
    std::cerr << status.error_message() << std::endl;
    return -1;
  }

  HGGMail sample;
  sample.Run();

  return 0;
}
