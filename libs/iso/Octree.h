/*
Copyright (c) 2006, Michael Kazhdan and Matthew Bolitho
All rights reserved.

Contains modifications by Simon Fuhrmann, 2013.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer. Redistributions in binary form must reproduce
the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution.

Neither the name of the Johns Hopkins University nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/

#ifndef OCT_NODE_INCLUDED
#define OCT_NODE_INCLUDED

#include <stdio.h>

#include "math/vector.h"
#include "BinaryNode.h"
#include "MarchingCubes.h"

/* Stores a (by construction) regular octree. Each node has either zero or eight child nodes. */
template<class NodeData, class Real=float>
class OctNode
{
public:  // Types
    typedef math::Vector<Real, 3> VertexType;

    class NodeIndex
    {
    public:
        NodeIndex(void);
        int depth,offset[3];
        NodeIndex child(const int& cIndex) const;
        NodeIndex parent(void) const;
        NodeIndex& operator += (const int& cIndex);
        NodeIndex& operator -- (void);
    };

public:  // Variables
    /*
     * Children are stored as an array of eight OctNodes. This allows each node to find its own
     * position relative to the parent node in constant time with
     *   index = (this - this->parent->children);
     */

    OctNode* parent;
    OctNode* children;
    NodeData nodeData;

public:  // Methods
    OctNode(void);
    ~OctNode(void);
    int initChildren(void);
    void deleteChildren(void);

    static inline void CenterAndWidth(const NodeIndex& nIndex,VertexType& center,Real& width);

    OctNode* nextLeaf(OctNode* currentLeaf);
    OctNode* nextNode(OctNode* currentNode);
    OctNode* nextBranch(OctNode* current);
    OctNode* nextLeaf(OctNode* currentLeaf,NodeIndex &nIndex);
    OctNode* nextNode(OctNode* currentNode,NodeIndex &nIndex);
    OctNode* nextBranch(OctNode* current,NodeIndex &nIndex);

    void setFullDepth(const int& maxDepth);

    const OctNode* faceNeighbor(const int& faceIndex) const;
    const OctNode* edgeNeighbor(const int& edgeIndex) const;

    static long long CornerIndex(const NodeIndex &nIndex,const int& cIndex,const int& maxDepth);
    static long long CornerIndex(const NodeIndex &nIndex,const int& cIndex,const int& maxDepth,int index[3]);

private:
    const OctNode* __faceNeighbor(const int& dir,const int& off) const;
    const OctNode* __edgeNeighbor(const int& o,const int i[2],const int idx[2]) const;
};


#include "Octree.inl"

#endif // OCT_NODE
