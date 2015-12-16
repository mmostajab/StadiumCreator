#include <fstream>
#include <iostream>
#include <string>

#include "stadium.h"


void main(){

  Stadium stadium;
  read_stadium_definition("stadium.def", stadium);
  write_stadium("test.stadium", stadium);

}