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
		m_GlobalTransform = AssimpGLMHelpers::ConvertMatrixToGLMFormat(m_CurrentAnimation->GetGlobalTransformation());

		m_FinalBoneMatrices.reserve(100);

		for (int i = 0; i < 100; i++)
			m_FinalBoneMatrices.push_back(glm::mat4(1.0f));

        LOGCATE("Animator init, m_GlobalTransform[0]: %s", glm::to_string(m_GlobalTransform).c_str());

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

	void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
	{

		std::string nodeName = node->name;
		glm::mat4 nodeTransform = node->transformation;
        LOGCATE("CalculateBoneTransform nodeName %s", nodeName.c_str());

		Bone* Bone = m_CurrentAnimation->FindBone(nodeName);

		if (Bone)
		{
            LOGCATE("CalculateBoneTransform Bone->Update %.4f", m_CurrentTime);
			Bone->Update(m_CurrentTime);
			nodeTransform = Bone->GetLocalTransform();
		}

		LOGCATE("CalculateBoneTransform, nodeTransform %s", glm::to_string(nodeTransform).c_str() );

		glm::mat4 globalTransformation = parentTransform * nodeTransform;

        LOGCATE("CalculateBoneTransform, parentTransform %s", glm::to_string(parentTransform).c_str() );
        LOGCATE("CalculateBoneTransform, globalTransformation %s", glm::to_string(globalTransformation).c_str() );


        std::map<std::string,BoneInfo> boneInfoMap = m_CurrentAnimation->GetBoneIDMap();

		if (boneInfoMap.find(nodeName) != boneInfoMap.end())
		{
			int index = boneInfoMap[nodeName].id;
			glm::mat4 offset = boneInfoMap[nodeName].offset;

			m_FinalBoneMatrices[index] = m_GlobalTransform * globalTransformation * offset;
			LOGCATE("m_FinalBoneMatrices[%d]: %s, offset %s", index, glm::to_string(m_FinalBoneMatrices[index]).c_str(), glm::to_string(offset).c_str());
		}

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
	glm::mat4 m_GlobalTransform;

};