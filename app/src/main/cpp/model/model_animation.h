#ifndef MODEL_ANIM_H
#define MODEL_ANIM_H

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../inc/assimp/Importer.hpp"
#include "../inc/assimp/scene.h"
#include "../inc/assimp/postprocess.h"

#include "mesh.h"
#include "shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include "assimp_glm_helpers.h"
#include "animdata.h"
#include <opencv2/opencv.hpp>

using namespace std;

class ModelAnim
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;
	bool hasTexture;
    glm::vec3 maxXyz, minXyz;




    // constructor, expects a filepath to a 3D model.
    ModelAnim(string const &path, bool gamma = false) : gammaCorrection(gamma), hasTexture(false)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
    	DEBUG_LOGCATE();
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    float GetMaxViewDistance()
    {
        //glm::vec3 vec3 = (abs(minXyz) + abs(maxXyz)) / 2.0f;
        //TODO chenxf
        glm::vec3 vec3 = maxXyz;
        float maxDis = fmax(vec3.x, fmax(vec3.y, vec3.z));
        LOGCATE("Model::GetMaxViewDistance maxDis=%f", maxDis);
        return maxDis;
    }

	std::map<string, BoneInfo> GetBoneInfoMap() {
		LOGCATE("GetBoneInfoMap, address %p, size=%d",&m_BoneInfoMap, m_BoneInfoMap.size());
		return m_BoneInfoMap;
    }

	int& GetBoneCount() { return m_BoneCounter; }

    bool ContainsTextures()
    {
        return hasTexture;
    }


    glm::vec3 GetAdjustModelPosVec()
    {
        glm::vec3 vec3 = (minXyz + maxXyz) / 2.0f;
        LOGCATE("Model::GetAdjustModelPosVec vec3(%f, %f, %f)", vec3.x, vec3.y, vec3.z);
        return (minXyz + maxXyz) / 2.0f;
    }

    void Destroy()
    {
        for (Mesh &mesh : meshes) {
            mesh.Destroy();
        }
    }

private:

	std::map<string, BoneInfo> m_BoneInfoMap;
	int m_BoneCounter = 0;

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
        // check for errors
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
		LOGCATE("processNode, finish processMesh");
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
		LOGCATE("processNode DONE");
    }

	void SetVertexBoneDataToDefault(Vertex& vertex)
	{
		for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
		{
			vertex.m_BoneIDs[i] = -1;
			vertex.m_Weights[i] = 0.0f;
		}
	}


	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			SetVertexBoneDataToDefault(vertex);
			vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
			vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);
			
			if (mesh->mTextureCoords[0])
			{
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		ExtractBoneWeightForVertices(vertices,mesh,scene);

		return Mesh(vertices, indices, textures);
	}

	void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
	{
    	LOGCATE("SetVertexBoneData, boneID %d, weight %f", boneID, weight);
		for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
		{
			if (vertex.m_BoneIDs[i] < 0)
			{
				vertex.m_Weights[i] = weight;
				vertex.m_BoneIDs[i] = boneID;
				break;
			}
		}
	}


	void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
	{
    	LOGCATE("ExtractBoneWeightForVertices, m_BoneCounter %d", m_BoneCounter);
		auto& boneInfoMap = m_BoneInfoMap;
		int& boneCount = m_BoneCounter;

		for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			int boneID = -1;
			std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
			LOGCATE("bone name %s", boneName.c_str());
			if (boneInfoMap.find(boneName) == boneInfoMap.end())
			{
				BoneInfo newBoneInfo;
				newBoneInfo.id = boneCount;
				newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
				boneInfoMap[boneName] = newBoneInfo;
				boneID = boneCount;
				boneCount++;
			}
			else
			{
				boneID = boneInfoMap[boneName].id;
			}
			LOGCATE("boneId %s, boneCount %d", boneName.c_str(), boneCount);

			assert(boneID != -1);
			auto weights = mesh->mBones[boneIndex]->mWeights;
			int numWeights = mesh->mBones[boneIndex]->mNumWeights;

			LOGCATE("weights %d, numWeights %d", weights, numWeights);

			for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;
				assert(vertexId <= vertices.size());
				SetVertexBoneData(vertices[vertexId], boneID, weight);
			}
		}
	}


	unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false)
	{
		string filename = string(path);
		filename = directory + '/' + filename;

		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char *data = nullptr;

		// load the texture using OpenCV
		LOGCATE("TextureFromFile Loading texture %s", filename.c_str());
		cv::Mat textureImage = cv::imread(filename);
		if (!textureImage.empty())
		{
			hasTexture = true;
			// opencv reads textures in BGR format, change to RGB for GL
			cv::cvtColor(textureImage, textureImage, CV_BGR2RGB);
			// opencv reads image from top-left, while GL expects it from bottom-left
			// vertically flip the image
			//cv::flip(textureImage, textureImage, 0);

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureImage.cols,
						 textureImage.rows, 0, GL_RGB, GL_UNSIGNED_BYTE,
						 textureImage.data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			GO_CHECK_GL_ERROR();
		} else {
			LOGCATE("TextureFromFile Texture failed to load at path: %s", path);
		}

		return textureID;
	}

    
    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }

    void updateMaxMinXyz(glm::vec3 pos)
    {
        maxXyz.x = pos.x > maxXyz.x ? pos.x : maxXyz.x;
        maxXyz.y = pos.y > maxXyz.y ? pos.y : maxXyz.y;
        maxXyz.z = pos.z > maxXyz.z ? pos.z : maxXyz.z;

        minXyz.x = pos.x < minXyz.x ? pos.x : minXyz.x;
        minXyz.y = pos.y < minXyz.y ? pos.y : minXyz.y;
        minXyz.z = pos.z < minXyz.z ? pos.z : minXyz.z;
    }

};



#endif