#include <iostream>
#include <string>
#include <unistd.h>

int main(int argc, char * argv[]){

  if(argc == 1)
    std::cout << "hello" << std::endl;

  if(argc == 2)
    return 69;

  for(int i = 1; i < argc; i++)
    std::cout << argv[i] << std::endl;

  std::string s;

  sleep(3);
  /*  while(s.compare("o") != 0){
    std::cin >> s;
    std::cout << "k" << std::endl;
  }
  */

  std::cout << "donezo" << std::endl;
  return 0;
}
