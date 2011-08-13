#include "fbx_json_serializer.h"
#include <iostream>
#include <fstream>

#include <fbxsdk.h>

#include "json/elements.h"
#include "json/writer.h"
using namespace json;

void FBXJSONSerializer::serialize(const std::string& input_filename, const std::string& output_filename) {
  KFbxSdkManager* manager = KFbxSdkManager::Create();

  KFbxIOSettings* ios = KFbxIOSettings::Create(manager, IOSROOT);
  manager->SetIOSettings(ios);

  KFbxImporter* importer = KFbxImporter::Create(manager, "");

  if(!importer->Initialize(input_filename.c_str(), -1, manager->GetIOSettings())) {
    std::cerr << "failed to import %s" << input_filename << std::endl;
    std::cerr << importer->GetLastErrorString() << std::endl;
    return;
  }

  KFbxScene* scene = KFbxScene::Create(manager, "scene");
  importer->Import(scene);
  importer->Destroy();

  KFbxNode* root_node = scene->GetRootNode();
  KFbxGeometryConverter converter(root_node->GetFbxSdkManager());
    Object json_root;
    
    Array meshes;  
    this->recurse_over_model(root_node, meshes);

    json_root["meshes"] = meshes;

    std::ofstream stream(output_filename.c_str());
    Writer::Write(json_root, stream);
}


void FBXJSONSerializer::recurse_over_model(fbxsdk_2012_1::KFbxNode *fbx_node, json::Array& meshes_array) {
  KFbxMesh* old_mesh = fbx_node->GetMesh();
  if (old_mesh) {
    KFbxGeometryConverter converter(fbx_node->GetFbxSdkManager());
    KFbxMesh* mesh = converter.TriangulateMesh(old_mesh);
    mesh->ComputeBBox();
    mesh->ComputeVertexNormals();
    
    Object json_mesh;
    {
      KFbxLayerElementUV* uvs = mesh->GetLayer(0)->GetUVs();
      KFbxLayerElementNormal* normals = mesh->GetLayer(0, KFbxLayerElement::eNORMAL)->GetNormals();
      Array json_mesh_normals;        
      Array json_mesh_uvs;
      Array json_mesh_vertices;
      int polygon_count = mesh->GetPolygonCount();
      for (int poly_index = 0; poly_index < polygon_count; poly_index++) {
        for (int vertex_index = 0; vertex_index < mesh->GetPolygonSize(poly_index); vertex_index++) {
          int vertex_position = mesh->GetPolygonVertex(poly_index, vertex_index);
          
          KFbxVector4 vertex = mesh->GetControlPoints()[vertex_position];
          Number vertex_x = vertex.GetAt(0); json_mesh_vertices.Insert(vertex_x);
          Number vertex_y = vertex.GetAt(1); json_mesh_vertices.Insert(vertex_y);
          Number vertex_z = vertex.GetAt(2); json_mesh_vertices.Insert(vertex_z);

          KFbxVector4 normal = normals->GetDirectArray()[vertex_position];

          Number normal_x = normal.GetAt(0); json_mesh_normals.Insert(normal_x);
          Number normal_y = normal.GetAt(1); json_mesh_normals.Insert(normal_y);
          Number normal_z = normal.GetAt(2); json_mesh_normals.Insert(normal_z); 

          if (uvs) {            
            int mesh_index = mesh->GetTextureUVIndex(poly_index, vertex_index);
            KFbxVector2 uv = uvs->GetDirectArray().GetAt(mesh_index);  
            Number uv_x = 1.0f-uv[0]; json_mesh_uvs.Insert(uv_x); // these are flipped
            Number uv_y = 1.0f-uv[1]; json_mesh_uvs.Insert(uv_y);
          }
        }
      }
      json_mesh["vertices"] = json_mesh_vertices;
      json_mesh["normals"] = json_mesh_normals;
      json_mesh["uvs"] = json_mesh_uvs;

    }
    {
      fbxDouble3 scale = fbx_node->LclScaling.Get();
      Object json_mesh_scale;
      json_mesh_scale["x"] = Number(scale[0]);
      json_mesh_scale["y"] = Number(scale[1]);
      json_mesh_scale["z"] = Number(scale[2]);
      json_mesh["scale"] = json_mesh_scale;
    }
    {
      fbxDouble3 translation = fbx_node->LclTranslation.Get();
      Object json_mesh_translation;
      json_mesh_translation["x"] = Number(translation[0]);
      json_mesh_translation["y"] = Number(translation[1]);
      json_mesh_translation["z"] = Number(translation[2]);
      json_mesh["translation"] = json_mesh_translation;
    }
    {
      fbxDouble3 rotation = fbx_node->LclRotation.Get();
      Object json_mesh_rotation;
      json_mesh_rotation["x"] = Number(rotation[0]);
      json_mesh_rotation["y"] = Number(rotation[1]);
      json_mesh_rotation["z"] = Number(rotation[2]);
      json_mesh["rotation"] = json_mesh_rotation;
    }
    {
      int material_count = fbx_node->GetMaterialCount(); 
      Array json_mesh_materials;
      
      for (int material_index = 0; material_index < material_count; material_index++) {
        KFbxSurfaceMaterial* surface_material = fbx_node->GetMaterial(material_index);
        
        Object json_material;
        
        Array json_textures;
        int textureIndex = 0;
        FOR_EACH_TEXTURE(textureIndex) {
          KFbxProperty property = surface_material->FindProperty(KFbxLayerElement::TEXTURE_CHANNEL_NAMES[textureIndex]);
          int layered_texture_count = property.GetSrcObjectCount(KFbxTexture::ClassId);
          for (int layered_texture_index = 0; layered_texture_index < layered_texture_count; ++layered_texture_index) {
            KFbxTexture* texture = KFbxCast <KFbxTexture> (property.GetSrcObject(KFbxTexture::ClassId, layered_texture_index));
            if(texture) {
              KFbxFileTexture *file_texture = KFbxCast<KFbxFileTexture>(texture);
              if (file_texture) {
                Object json_texture;
                json_texture["filename"] = String(file_texture->GetFileName());
                json_textures.Insert(json_texture);
              }               
            }
          }
        }
        json_material["textures"] = json_textures;
        
        KFbxSurfaceLambert* lambert_material = KFbxCast<KFbxSurfaceLambert>(surface_material);
        
        if (lambert_material) {
          Object diffuse;
          double diffuse_r = lambert_material->Diffuse.Get()[0];
          diffuse["r"] = Number(diffuse_r);
          double diffuse_g = lambert_material->Diffuse.Get()[1];
          diffuse["g"] = Number(diffuse_g);
          double diffuse_b = lambert_material->Diffuse.Get()[2];
          diffuse["b"] = Number(diffuse_b);
          json_material["diffuse"] = diffuse;

          Object ambient;
          double ambient_r = lambert_material->Ambient.Get()[0];
          ambient["r"] = Number(ambient_r);
          double ambient_g = lambert_material->Ambient.Get()[1];
          ambient["g"] = Number(ambient_g);
          double ambient_b = lambert_material->Ambient.Get()[2];
          ambient["b"] = Number(ambient_b);
          json_material["ambient"] = ambient;
          
          KFbxProperty specular_property = lambert_material->FindProperty("SpecularColor");
          fbxDouble3 specular_data;
          specular_property.Get(&specular_data, eDOUBLE3);
          Object specular;
          float specular_r = specular_data[0];
          specular["r"] = Number(specular_r);
          float specular_g = specular_data[1];
          specular["g"] = Number(specular_g);
          float specular_b = specular_data[2];
          specular["b"] = Number(specular_b);
          json_material["specular"] = specular;            
        }
        
        json_mesh_materials.Insert(json_material);
      }
      json_mesh["materials"] = json_mesh_materials;
    }
    json_mesh["uv_stride"] = Number(2);
    json_mesh["vertex_stride"] = Number(3);
    json_mesh["normal_stride"] = Number(3);
    meshes_array.Insert(json_mesh);
  }
  
  for(int j = 0; j < fbx_node->GetChildCount(); j++) {
    recurse_over_model(fbx_node->GetChild(j), meshes_array);
  }
}