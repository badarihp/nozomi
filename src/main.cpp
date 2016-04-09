#include "src/Route.h"
#include "src/HTTPResponse.h"
using namespace std;
using namespace sakura;

int main() {
  std::function<HTTPResponse(int, double, int)> f = 
    [](int i, double d, int ii) -> HTTPResponse { 
      i = i+1;
      throw std::runtime_error("error"); 
    };
  vector<RouteParamType> types = parseParameters(f);
  for(auto i: types){ 
    std::cout << i << std::endl;
  }
  return 0;
}
