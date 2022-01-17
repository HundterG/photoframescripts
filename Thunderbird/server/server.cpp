#include "microhttpd/microhttpd.h"
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <fstream>
#include <list>
#include <mutex>

// https://www.gnu.org/software/libmicrohttpd/manual/libmicrohttpd.html#microhttpd_002dcb

// set to 1 to allow the debug page from a web browser
#define DEBUG_PAGE 1

#if DEBUG_PAGE
char const *debugPage = "<html><head><title>Test Upload</title></head><body><input type=\"file\" id=\"input\"><script>function Update(){var file = this.files[0];var xhr = new XMLHttpRequest();xhr.open(\"POST\", \"http://127.0.0.1:8080/\" + file.name, true);xhr.setRequestHeader(\"Content-Type\", file.type);xhr.onreadystatechange = function(){if(this.readyState === XMLHttpRequest.DONE && this.status === 200){}};xhr.send(file);}document.getElementById(\"input\").addEventListener(\"change\", Update, false);</script><br></body></html>";
#endif

std::string savePath;
int getDummy;

std::mutex fileLock;
std::list<std::ofstream> openFiles;

void ServerPanic(void*, char const*, unsigned int, char const*)
{
	// if something bad happens, just get out of here. big brother will fix it
	throw;
}

int AcceptFunc(void *, const sockaddr *addr, socklen_t)
{
	// only accept from localhost
	if(addr->sa_family != AF_INET) return MHD_NO;
	const sockaddr_in *addr_in = reinterpret_cast<const sockaddr_in *>(addr);
	char const *name = reinterpret_cast<char const*>(&addr_in->sin_addr.s_addr);
	if(name[0] != 127 || name[1] != 0 || name[2] != 0 || name[3] != 1) return MHD_NO;
	return MHD_YES;
}

int GET(MHD_Connection *con, char const *url, void **contexPtr)
{
	if(&getDummy != *contexPtr)
	{
		// this is first call, only headers are valid
		*contexPtr = &getDummy;
		return MHD_YES;
	}
#if DEBUG_PAGE
	if(strcmp("/", url) == 0)
	{
		MHD_Response *response = MHD_create_response_from_buffer(std::strlen(debugPage), const_cast<char*>(debugPage), MHD_RESPMEM_PERSISTENT);
		if(!response) return MHD_NO;
		int ret = MHD_queue_response(con, MHD_HTTP_OK, response);
		MHD_destroy_response(response);
		return ret;
	}
	if(strcmp(url, "/favicon.ico") == 0)
	{
		char const *body = "error";
		MHD_Response *response = MHD_create_response_from_buffer(5, const_cast<char*>(body), MHD_RESPMEM_PERSISTENT);
		if(!response) return MHD_NO;
		int ret = MHD_queue_response(con, MHD_HTTP_BAD_REQUEST, response);
		MHD_destroy_response(response);
	}
#endif
	return MHD_NO;
}

int POST(MHD_Connection *con, char const *url, char const *upData, size_t *upSize, void **contexPtr)
{
	if(*contexPtr == nullptr)
	{
		// setup file
		std::string fileName = savePath + std::to_string(std::rand()) + "_" + (url+1);
		std::lock_guard<std::mutex> g(fileLock);
		openFiles.emplace_front(fileName.c_str());
		std::ofstream &file = openFiles.front();
		if(!file.is_open())
		{
			openFiles.pop_front();
			return MHD_NO;
		}
		*contexPtr = &file;
		return MHD_YES;
	}
	else if(*upSize != 0)
	{
		// write data
		std::ofstream &file = *reinterpret_cast<std::ofstream*>(*contexPtr);
		file.write(upData, *upSize);
		*upSize = 0;
		return MHD_YES;
	}
	else
	{
		// assume file is done
		char const *body = "good";
		MHD_Response *response = MHD_create_response_from_buffer(4, const_cast<char*>(body), MHD_RESPMEM_PERSISTENT);
		if(!response) return MHD_NO;
		int ret = MHD_queue_response(con, MHD_HTTP_OK, response);
		MHD_destroy_response(response);
		return ret;
	}
}

int OPTIONS(MHD_Connection *con)
{
	char const *body = "good";
	MHD_Response *response = MHD_create_response_from_buffer(4, const_cast<char*>(body), MHD_RESPMEM_PERSISTENT);
	if(!response) return MHD_NO;
	MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
	MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
	MHD_add_response_header(response, "Access-Control-Allow-Headers", "content-type");
	int ret = MHD_queue_response(con, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	return ret;
}

int ServerFunc(void*, MHD_Connection *con, char const *url, char const *method, char const*, char const *upData, size_t *upSize, void **contexPtr)
{
	if(strcmp(method, "POST") == 0)
		return POST(con, url, upData, upSize, contexPtr);
	else if(strcmp(method, "GET") == 0)
		return GET(con, url, contexPtr);
	else if(strcmp(method, "OPTIONS") == 0)
		return OPTIONS(con);
	else
		return MHD_NO;
}

void CompleteFunc(void*, MHD_Connection*, void **con_cls, MHD_RequestTerminationCode)
{
	if(!(*con_cls == nullptr && *con_cls == &getDummy))
	{
		// close file
		std::lock_guard<std::mutex> g(fileLock);
		openFiles.remove_if([con_cls](std::ofstream &file)
		{
			return &file == *con_cls;
		});
	}
}

int main(void)
{
	savePath.assign(getenv("HOME"));
	savePath.append("/Pictures/");

	MHD_set_panic_func(ServerPanic, nullptr);
	MHD_Daemon *server = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, 8080, AcceptFunc, nullptr, ServerFunc, nullptr, MHD_OPTION_PER_IP_CONNECTION_LIMIT, 5, MHD_OPTION_NOTIFY_COMPLETED, CompleteFunc, nullptr, MHD_OPTION_END);
	if(!server)
		throw;
	for(;;)
		sleep(10);
	return 1;
}
