#pragma once

#include <glm.hpp>
#include <gtx/string_cast.hpp>
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "animation.h"
#include "bone.h"

class Animator
{
public:
	Animator(Animation* animation)
	{
		m_CurrentTime = 0.0;
		m_CurrentAnimation = animation;
        m_RootGlobalTransform = AssimpGLMHelpers::ConvertMatrixToGLMFormat(m_CurrentAnimation->GetGlobalTransformation());

		m_FinalBoneMatrices.reserve(100);

		for (int i = 0; i < 100; i++)
			m_FinalBoneMatrices.push_back(glm::mat4(1.0f));

        LOGCATE("Animator init, m_RootGlobalTransform[0]: %s", glm::to_string(m_RootGlobalTransform).c_str());

	}

	void UpdateAnimation(float dt)
	{
	    DEBUG_LOGCATE();
		m_DeltaTime = dt;
		if (m_CurrentAnimation)
		{
			m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
			m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
			CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
		}
	}

	void PlayAnimation(Animation* pAnimation)
	{
		m_CurrentAnimation = pAnimation;
		m_CurrentTime = 0.0f;
	}

	/**
	 * 计算某个骨骼 影响顶点的换算矩阵
	 * @param node 存骨骼名字，矩阵
	 * @param parentTransform 父节点的换算矩阵
	 */
	void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
	{

		std::string nodeName = node->name;
		glm::mat4 nodeTransform = node->transformation;
        LOGCATE("CalculateBoneTransform nodeName %s", nodeName.c_str());

		Bone* Bone = m_CurrentAnimation->FindBone(nodeName);

		if (Bone)
		{
            LOGCATE("CalculateBoneTransform Bone->Update %.4f", m_CurrentTime);
            //Bone对象根据时间，计算一个矩阵
			Bone->Update(m_CurrentTime);
			//得到矩阵
			nodeTransform = Bone->GetLocalTransform();
		}

		LOGCATE("CalculateBoneTransform, nodeTransform %s", glm::to_string(nodeTransform).c_str() );
		//当前骨骼的换算矩阵，会被父节点的矩阵影响，所以要相乘
		glm::mat4 globalTransformation = parentTransform * nodeTransform;

        LOGCATE("CalculateBoneTransform, parentTransform %s", glm::to_string(parentTransform).c_str() );
        LOGCATE("CalculateBoneTransform, globalTransformation %s", glm::to_string(globalTransformation).c_str() );


        std::map<std::string,BoneInfo> boneInfoMap = m_CurrentAnimation->GetBoneIDMap();

		if (boneInfoMap.find(nodeName) != boneInfoMap.end())
		{
			int index = boneInfoMap[nodeName].id;
			glm::mat4 offset = boneInfoMap[nodeName].offset;
			//某个骨骼影响顶点的换算矩阵，该矩阵将传递给vertex shader
			//需要再乘于根节点的m_GlobalTransform，根节点影响所有子节点的换算
			m_FinalBoneMatrices[index] = m_RootGlobalTransform * globalTransformation * offset;
			LOGCATE("m_RootGlobalTransform %s, m_FinalBoneMatrices[%d]: %s, offset %s", glm::to_string(m_RootGlobalTransform).c_str(),
							index, glm::to_string(m_FinalBoneMatrices[index]).c_str(), glm::to_string(offset).c_str());
		}

		//递归，计算子节点的矩阵
		for (int i = 0; i < node->childrenCount; i++)
			CalculateBoneTransform(&node->children[i], globalTransformation);
	}

	std::vector<glm::mat4> GetFinalBoneMatrices()
	{
		return m_FinalBoneMatrices;
	}

private:
	std::vector<glm::mat4> m_FinalBoneMatrices;
	Animation* m_CurrentAnimation;
	float m_CurrentTime;
	float m_DeltaTime;
	glm::mat4 m_RootGlobalTransform;

};