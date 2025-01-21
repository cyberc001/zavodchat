#include <chrono>
#include <iostream>

int main()
{
	const auto server_ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
	std::cout << "sending timestamp " << server_ts.count() << std::endl;
	std::string req = "curl -k -XGET -w '\\n' 'https://localhost/clock?ts=" + std::to_string(server_ts.count()) + "'";
	system(req.c_str());
	return 0;
}
