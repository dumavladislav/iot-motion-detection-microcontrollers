
#include <sstream>

using namespace std;

class DeviceUtilities
{
private:
    /* data */
public:
    DeviceUtilities(/* args */);
    ~DeviceUtilities();

    vector<string> static splitString(string str, char delimiter);
};

DeviceUtilities::DeviceUtilities(/* args */)
{
}

DeviceUtilities::~DeviceUtilities()
{
}

vector<string> DeviceUtilities::splitString(string str, char delimiter)
{
  vector<string> internal;
  stringstream ss(str); // Turn the string into a stream.
  string tok;

  while (getline(ss, tok, delimiter))
  {
    internal.push_back(tok);
  }

  return internal;
}
