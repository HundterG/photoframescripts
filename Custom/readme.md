# HG Attachment Downloader

This is a custom script program that was made to download attachments for my custom picture frame.

tylerbhundt.altervista.org/pictureframe.html (Full blog post talking about the picture frames development)

This script may take a bit to get setup but it is lightweight so it can run well on embedded systems.

WARNING! This script permanitly deletes all emails once it is done! You have been warned.

# Compiling

1. First, you will need to download the Google C++ API and the Gmail addon.
* https://github.com/google/google-api-cpp-client (Google C++ API)
* https://developers.google.com/resources/api-libraries/download/gmail/v1/cpp (Gmail addon)

2. Setup the folder to build the project.
* Extract google-api-cpp-client-master.zip and google-gmail-v1-rev110-cpp-0.1.4.zip
* Copy the gmail folder into "google-api-cpp-client-master/service_apis"

3. Custom setup. Unfortunately, the google API as it is right now is incomplete so some custom changes will be needed to bypass or replace missing functions.
* Inside SDKChanges is 4 files to put in the SDK.
* Copy gmail_service.cc gmail_service.h and string_view.hpp and paste them in "google-api-cpp-client-master/service_apis/gmail/google/gmail_api"
* Copy client_service.h and paste it in "google-api-cpp-client-master/src/googleapis/client/service"

* Copy the contents of PictureFrameScript and paste them in "google-api-cpp-client-master/src/samples"
* Open HGPhoto and on line 81, replace USER with the email address you will be using.
* Optionally on line 307, replace magic with the key word you want the subject line to have.

* In "google-api-cpp-client-master/src/samples", open CMakeLists.txt and make the following changes.
Add "INCLUDE_DIRECTORIES(${GOOGLEAPIS_SERVICE_REPOSITORY_DIR}/gmail)" with the other INCLUDE_DIRECTORIES.
Add the following to the end of the file:
add_executable(HGPhoto HGPhoto.cc)
target_link_libraries(HGPhoto google_gmail_api)
target_link_libraries(HGPhoto googleapis_jsoncpp)
target_link_libraries(HGPhoto googleapis_curl_http)
target_link_libraries(HGPhoto googleapis_http)
target_link_libraries(HGPhoto googleapis_oauth2)
target_link_libraries(HGPhoto googleapis_utils)
if (HAVE_OPENSSL)
target_link_libraries(HGPhoto googleapis_openssl_codec)
endif()
target_link_libraries(HGPhoto ${GFLAGS_LIBRARY})
target_link_libraries(HGPhoto pthread)

4. Building
* Make sure you have Python 2.7 and g++ installed.
* Open a command window in "google-api-cpp-client-master" and run the following command.
python2 prepare_dependencies.py

* On Raspbian, you may run into problems compiling OpenSSL. If this happens, open prepare_dependencies.py and remove the openssl entry from the build. Then delete the "external_dependencies" folder.
* Install OpenSSL manually using the package manager on your OS.
* Run the script again.

* When that is done, run these commands to build the programs.
mkdir build && cd build
../external_dependencies/install/bin/cmake ..
make

* If it compiles cleanly, the program will be in "google-api-cpp-client-master/build/bin"

5. Setting up Google
* Goto "https://developers.google.com/identity/protocols/oauth2" and follow the instructions to obtain a client secret file for the program.

6. Finalize
* The last step is to install Zenity using the package manager on your OS.
* Once everything is done, you should be able to use the program with the folowing command.
"./HGPhoto secret_file login_prompt"
Secret_file is the file that you downloaded from the Google Dev Portal
Login_prompt is a bool for if the login prompt should display if it fails to login automatically.
When the program is done, all attachments will be in the users Pictures folder.

# BigBro

The bigbro folder contains a program for running the script at specific intervals, displaying errors, and starting a picture slideshow.
