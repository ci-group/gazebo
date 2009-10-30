
/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

/** @file Definition of the .b3d importer class. */

#ifndef AI_B3DIMPORTER_H_INC
#define AI_B3DIMPORTER_H_INC

#include "aiTypes.h"
#include "aiMesh.h"
#include "aiMaterial.h"

#include <string>
#include <vector>

namespace Assimp{

class B3DImporter : public BaseImporter{
public:

	virtual bool CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const;

protected:

	virtual void GetExtensionList(std::string& append);
	virtual void InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler);

private:

	int ReadByte();
	int ReadInt();
	float ReadFloat();
	aiVector2D ReadVec2();
	aiVector3D ReadVec3();
	aiQuaternion ReadQuat();
	std::string ReadString();
	std::string ReadChunk();
	void ExitChunk();
	unsigned ChunkSize();

	template<class T>
	T *to_array( const std::vector<T> &v );

	struct Vertex{
		aiVector3D vertex;
		aiVector3D normal;
		aiVector3D texcoords;
		unsigned char bones[4];
		float weights[4];
	};

	void Oops();
	void Fail( std::string str );

	void ReadTEXS();
	void ReadBRUS();

	void ReadVRTS();
	void ReadTRIS( int v0 );
	void ReadMESH();
	void ReadBONE( int id );
	void ReadKEYS( aiNodeAnim *nodeAnim );
	void ReadANIM();

	aiNode *ReadNODE( aiNode *parent );

	void ReadBB3D( aiScene *scene );

	unsigned _pos;
	unsigned _size;
	std::vector<unsigned char> _buf;
	std::vector<unsigned> _stack;
	
	std::vector<std::string> _textures;
	std::vector<aiMaterial*> _materials;

	int _vflags,_tcsets,_tcsize;
	std::vector<Vertex> _vertices;

	std::vector<aiNode*> _nodes;
	std::vector<aiMesh*> _meshes;
	std::vector<aiNodeAnim*> _nodeAnims;
	std::vector<aiAnimation*> _animations;
};

}

#endif
