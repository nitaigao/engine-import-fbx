#include <iostream>

#include "fbx_json_serializer.h"
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

int main (int argc, const char * argv[]) {
  directory_iterator end_itr;
  for (directory_iterator itr("/Users/nk/Desktop/fbx"); itr != end_itr; ++itr) {
    if (extension(itr->path()) == ".fbx") {
      create_directory("/Users/nk/Desktop/compiled");
      path output_name = std::string("/Users/nk/Desktop/compiled/") + basename(itr->path()) + std::string(".json");
    
      FBXJSONSerializer serializer;
      serializer.serialize(itr->path().string().c_str(), output_name.string().c_str());
    }
  }
}