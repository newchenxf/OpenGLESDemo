#pragma once

#include <vector>
#include <map>
#include "../glm/glm.hpp"
#include "../inc/assimp/scene.h"
#include "bone.h"
#include <functional>
#include "animdata.h"
#include "model_animation.h"
#include "LogUtil.h"

struct AssimpNodeData
{
	glm::mat4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;
};

class Animation
{
public:
	Animation() = default;

	Animation(const std::string& animationPath, ModelAnim* model)
	{
		LOGCATE("Animation created");
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
		assert(scene && scene->mRootNode);

		auto animation = scene->mAnimations[0];//可以有多种动画，本例子只显示第一种动画

		m_Duration = animation->mDuration;
		m_TicksPerSecond = animation->mTicksPerSecond;
		LOGCATE("Animation created, m_Duration %d, m_TicksPerSecond %d", m_Duration, m_TicksPerSecond);
        m_GlobalTransformation = scene->mRootNode->mTransformation;//TODO chenxf
		m_GlobalTransformation = m_GlobalTransformation.Inverse();
		ReadHeirarchyData(m_RootNode, scene->mRootNode);
		ReadMissingBones(animation, *model);
	}

	~Animation()
	{
	}

	Bone* FindBone(const std::string& name)
	{
		LOGCATE("FindBone %s", name.c_str());
		auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
			[&](const Bone& Bone)
			{
				return Bone.GetBoneName() == name;
			}
		);
		if (iter == m_Bones.end()) return nullptr;
		else return &(*iter);
	}

	
	inline float GetTicksPerSecond() { return m_TicksPerSecond; }
	inline float GetDuration() { return m_Duration;}
	inline aiMatrix4x4& GetGlobalTransformation() { return m_GlobalTransformation;}

	inline const AssimpNodeData& GetRootNode() { return m_RootNode; }
	inline const std::map<std::string,BoneInfo>& GetBoneIDMap() 
	{
		LOGCATE("GetBoneIDMap, m_BoneInfoMap size %d", m_BoneInfoMap.size());
		return m_BoneInfoMap;
	}

private:
	void ReadMissingBones(const aiAnimation* animation, ModelAnim& model)
	{
		int size = animation->mNumChannels;
		//获得之前解析权重时所记录的骨骼map，其中key为骨骼名字
		m_BoneInfoMap = model.GetBoneInfoMap();//getting m_BoneInfoMap from Model class
		LOGCATE("ReadMissingBones, m_BoneInfoMap address %p, size %d, animation->mNumChannels %d", &m_BoneInfoMap,m_BoneInfoMap.size(), animation->mNumChannels);
		//获得骨骼计数器，用于分配id
		int& boneCount = model.GetBoneCount(); //getting the m_BoneCounter from Model class

		//reading channels(bones engaged in an animation and their keyframes)
		//读取通道列表，每个通道包括所有被该动画影响的骨骼，以及对应的关键帧
		for (int i = 0; i < size; i++)
		{
			auto channel = animation->mChannels[i];//一个channel代表某个骨骼
			std::string boneName = channel->mNodeName.data;//拿到骨骼名字

			if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
			{//如果万一map不包括这个骨骼，则记录下来
				m_BoneInfoMap[boneName].id = boneCount;
				boneCount++;
			}
			//创建一个Bone对象，添加到m_Bones数组
			m_Bones.push_back(Bone(channel->mNodeName.data,
								   m_BoneInfoMap[channel->mNodeName.data].id, channel));
		}
		LOGCATE("ReadMissingBones, m_BoneInfoMap size %d", m_BoneInfoMap.size());
//		m_BoneInfoMap = boneInfoMap;
		LOGCATE("ReadMissingBones, now m_BoneInfoMap size %d", m_BoneInfoMap.size());

	}

	void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
	{
		assert(src);

		dest.name = src->mName.data;
		dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
		dest.childrenCount = src->mNumChildren;

		for (int i = 0; i < src->mNumChildren; i++)
		{
			AssimpNodeData newData;
			ReadHeirarchyData(newData, src->mChildren[i]);
			dest.children.push_back(newData);
		}
	}
	float m_Duration;
	int m_TicksPerSecond;
	std::vector<Bone> m_Bones;
	AssimpNodeData m_RootNode;
	std::map<std::string, BoneInfo> m_BoneInfoMap;
    aiMatrix4x4 m_GlobalTransformation;
};

