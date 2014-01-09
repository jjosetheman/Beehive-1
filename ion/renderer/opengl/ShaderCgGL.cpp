///////////////////////////////////////////////////
// File:		ShaderCgGL.cpp
// Date:		11th December 2013
// Authors:		Matt Phillips
// Description:	Cg shader implementation
///////////////////////////////////////////////////

#pragma once

#include "core/debug/Debug.h"
#include "io/file.h"
#include "renderer/opengl/ShaderCgGL.h"
#include "renderer/opengl/TextureOpenGL.h"
#include "renderer/opengl/RendererOpenGL.h"

namespace ion
{
	namespace render
	{
		CGcontext ShaderManagerCgGL::sCgContext = 0;
		int ShaderManagerCgGL::sContextRefCount = 0;

		ShaderManager* ShaderManager::Create()
		{
			return new ShaderManagerCgGL();
		}

		ShaderManagerCgGL::ShaderManagerCgGL()
		{
			if(!sContextRefCount)
			{
				sCgContext = cgCreateContext();

				//Case sensitive semantics
				cgSetSemanticCasePolicy(CG_UNCHANGED_CASE_POLICY);

				//Set managed texture params (automatically binds/unbinds textures)
				cgGLSetManageTextureParameters(sCgContext, true);
			}

			sContextRefCount++;
		}

		ShaderManagerCgGL::~ShaderManagerCgGL()
		{
			sContextRefCount--;

			if(!sContextRefCount)
			{
				cgDestroyContext(sCgContext);
			}
		}

		bool ShaderManagerCgGL::CheckCgError()
		{
			CGerror error = cgGetError();

			if(error)
			{
				std::string errorStr = cgGetErrorString(error);

				if(error == CG_COMPILER_ERROR)
				{
					errorStr += " : ";
					errorStr += cgGetLastListing(sCgContext);
				}

				debug::Error(errorStr.c_str());
			}

			return error == CG_NO_ERROR;
		}

		Shader* Shader::Create()
		{
			return new ShaderCgGL();
		}

		void Shader::RegisterSerialiseType(io::Archive& archive)
		{
			archive.RegisterPointerTypeStrict<Shader, ShaderCgGL>();
		}

		ShaderCgGL::ShaderCgGL()
		{
			mCgProgram = 0;
			mProgramLoaded = false;
		}

		ShaderCgGL::~ShaderCgGL()
		{
			if(mCgProgram)
			{
				cgDestroyProgram(mCgProgram);
			}
		}

		bool ShaderCgGL::Load()
		{
			bool result = false;

			//Open file
			io::File file(mProgramFilename, io::File::OpenRead);

			if(file.IsOpen())
			{
				//Load file contents
				u64 fileSize = file.GetSize();
				char* programText = new char[(int)fileSize + 1];
				file.Read(programText, fileSize);
				programText[fileSize] = 0;

				//Done with file
				file.Close();

				//Set OpenGL context for current thread
				RendererOpenGL::SetThreadGLContext();

				if(mProgramType == Vertex)
				{
					mCgProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
				}
				else if (mProgramType == Fragment)
				{
					mCgProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
				}

				cgGLSetOptimalOptions(mCgProfile);

				//Compile program
				mCgProgram = cgCreateProgram(ShaderManagerCgGL::sCgContext, CG_SOURCE, programText, mCgProfile, mEntryPoint.c_str(), NULL);

				//Done with file text
				delete programText;
			}

			return ShaderManagerCgGL::CheckCgError();
		}

		void ShaderCgGL::Bind()
		{
			if(!mProgramLoaded)
			{
				//Load Cg program
				cgGLLoadProgram(mCgProgram);
				ShaderManagerCgGL::CheckCgError();
				mProgramLoaded = true;
			}

			cgGLEnableProfile(mCgProfile);
			cgGLBindProgram(mCgProgram);
			ShaderManagerCgGL::CheckCgError();
		}

		void ShaderCgGL::Unbind()
		{
			cgGLUnbindProgram(mCgProfile);
			cgGLDisableProfile(mCgProfile);
			ShaderManagerCgGL::CheckCgError();
		}

		Shader::ShaderParamDelegate* ShaderCgGL::CreateShaderParamDelegate(const std::string& paramName)
		{
			ShaderParamDelegateCg* paramDelegate = NULL;

			CGparameter param = cgGetNamedParameter(mCgProgram, paramName.c_str());
			if(param)
			{
				paramDelegate = new ShaderParamDelegateCg(param);
			}

			return paramDelegate;
		}

		ShaderCgGL::ShaderParamDelegateCg::ShaderParamDelegateCg(CGparameter cgParam)
		{
			mCgParam = cgParam;
		}

		void ShaderCgGL::ShaderParamDelegateCg::Set(const int& value)
		{
			cgGLSetParameter1f(mCgParam, (float)value);
			ShaderManagerCgGL::CheckCgError();
		}

		void ShaderCgGL::ShaderParamDelegateCg::Set(const float& value)
		{
			cgGLSetParameter1f(mCgParam, value);
			ShaderManagerCgGL::CheckCgError();
		}

		void ShaderCgGL::ShaderParamDelegateCg::Set(const Vector2& value)
		{
			cgGLSetParameter2fv(mCgParam, (float*)&value);
			ShaderManagerCgGL::CheckCgError();
		}

		void ShaderCgGL::ShaderParamDelegateCg::Set(const Vector3& value)
		{
			cgGLSetParameter3fv(mCgParam, (float*)&value);
			ShaderManagerCgGL::CheckCgError();
		}

		void ShaderCgGL::ShaderParamDelegateCg::Set(const Colour& value)
		{
			cgGLSetParameter4fv(mCgParam, (float*)&value);
			ShaderManagerCgGL::CheckCgError();
		}

		void ShaderCgGL::ShaderParamDelegateCg::Set(const Matrix4& value)
		{
			cgSetMatrixParameterfc(mCgParam, (float*)&value);
			ShaderManagerCgGL::CheckCgError();
		}

		void ShaderCgGL::ShaderParamDelegateCg::Set(const Texture& value)
		{
			cgGLSetTextureParameter(mCgParam, ((TextureOpenGL&)value).GetTextureId());
			ShaderManagerCgGL::CheckCgError();
		}
	}
}