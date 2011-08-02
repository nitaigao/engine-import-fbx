#include <iostream>

#include "fbx_json_serializer.h"

int main (int argc, const char * argv[]) {
  const char* input_filename = "/Users/nk/Desktop/untitled.fbx";
  const char* output_filename = "/Users/nk/Desktop/untitled.json";
  FBXJSONSerializer serializer;
  serializer.serialize(input_filename, output_filename);
}