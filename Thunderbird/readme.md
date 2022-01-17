# HG Picture Downloader

This is a small script for downloading attachments to a folder when new mail comes in.

tylerbhundt.altervista.org/pictureframe.html (Full blog post talking about the picture frames development)

This script comes in 2 parts, the plugin to be used with Thunderbird and a standalone server for handling the file saving.

The plugin part of the script requires Thunderbird version 91 in order to run.

# Compiling

1. The standalone server must be compiled.
* Open a command window in "Thunderbird/server" and run these command.
"gcc microhttpd/*.c -c"
"g++ *.o server.cpp -pthread -oserver.out"
* While the server is running, no output will be produced.
* You can test if the server is running by going to "http://localhost:8080/" in a web browser and uploading a file manually.

2. Thunderbird
* Follow these instructions to activate the debug plugin panel and load this plugin.
https://developer.thunderbird.net/add-ons/mailextensions/hello-world-add-on

3. Running
Make sure that the standalone server is running when the plugin is active otherwise attachments will not be saved to disk.
