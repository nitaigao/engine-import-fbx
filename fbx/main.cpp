#include <iostream>

#include "fbx_json_serializer.h"
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

int main (int argc, const char * argv[]) {
  std::string input_file = argv[1];
  std::string output_file = argv[2];
  
  std::clog << input_file << " to " << output_file << std::endl;
    
  FBXJSONSerializer serializer;
  serializer.serialize(input_file, output_file);
}