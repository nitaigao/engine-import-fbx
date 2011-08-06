#ifndef fbx_fbx_json_serializer_h
#define fbx_fbx_json_serializer_h

#include <fbxsdk.h>
#include "json/elements.h"
using namespace json;

class FBXJSONSerializer {
  
public:
  
  void serialize(const char* input_filename, const char* output_filename);
  
private:
  
  void recurse_over_model(KFbxNode* node, Array& meshes_array);
  
};

#endif
