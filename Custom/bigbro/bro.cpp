#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include <cstdio>
#include <sstream>

std::atomic_bool running = ATOMIC_VAR_INIT(false);

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

void MailThread(void)
{
	bool lastGood = true;
	std::this_thread::sleep_for( std::chrono::minutes(1) );
	for(;;)
	{
		bool thisGood = false;
		running = true;
		std::this_thread::sleep_for( std::chrono::seconds(5) );
		std::string result = RunCommand("HGPhoto HGPDsecret.json false");
		running = false;
		if(result.size() != 0)
		{
			if(result[0] == 'G')
			{
				std::this_thread::sleep_for( std::chrono::minutes(30) );
				thisGood = true;
			}
		}

		if(thisGood == false && lastGood == false)
		{
			system("zenity --error --text=\"Error connecting to gmail. If error persists, you may need to log back in.\"");
			return;
		}
		else if(thisGood == false)
		{
			std::this_thread::sleep_for( std::chrono::minutes(10) );
		}

		lastGood = thisGood;
	}
}

void SlideShowWatcher(void)
{
	time_t time = std::time(nullptr);
	int count = 0;
	while(count < 3)
	{
		system("feh -x -F -r -Y -Z -z --auto-rotate -A slideshow -D 7 -R 60 /home/pi/Pictures");
		if(60 < std::time(nullptr) - time)
		{
			count = 0;
			time = std::time(nullptr);
		}
		++count;
	}
}

int main(void)
{
	std::this_thread::sleep_for( std::chrono::seconds(2) );
	std::thread t3(SlideShowWatcher);
	std::thread t4(MailThread);
	t4.detach();
	t3.join();
	while(running) { std::this_thread::sleep_for( std::chrono::seconds(1) ); }
	return 1;
}
