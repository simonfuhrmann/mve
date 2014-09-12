/*
Copyright (c) 2007, Michael Kazhdan
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

#ifndef NEIGHBOR_KEY_INCLUDED
#define NEIGHBOR_KEY_INCLUDED

#include "Octree.h"

/* Stores the 3x3x3 neighborhood of an octree node. */
template<class NodeData, class Real>
struct Neighbors
{
    Neighbors(void);
    void clear(void);

    OctNode<NodeData,Real>* neighbors[3][3][3];
    typename OctNode<NodeData,Real>::NodeIndex nIndex;
};

/* Stores 3x3x3 neighborhoods of an octree node and each of its parents. */
template<class NodeData, class Real>
class NeighborKey
{
public:
    Neighbors<NodeData,Real>* neighbors;
    int depth;

    NeighborKey(void);
    ~NeighborKey(void);

    void set(const int& depth);
    Neighbors<NodeData,Real>& setNeighbors(OctNode<NodeData,Real>* node);
    Neighbors<NodeData,Real>& getNeighbors(OctNode<NodeData,Real>* node);

private:
    int _depth;
    Neighbors<NodeData,Real>& _setNeighbors(OctNode<NodeData,Real>* node,const int& d);
    Neighbors<NodeData,Real>& _getNeighbors(OctNode<NodeData,Real>* node,const int& d);
};

/* Uses the above NeighborKey to find all octree nodes surrounding a corner. */
template<class NodeData, class Real>
class IsoNeighborKey : public NeighborKey<NodeData,Real>
{
public:
    void GetCornerNeighbors(OctNode<NodeData,Real>* node,const int& c,OctNode<NodeData,Real>* neighbors[Cube::CORNERS]);
    OctNode<NodeData,Real>* FaceNeighbor(OctNode<NodeData,Real>* node,int dir,int off);
    OctNode<NodeData,Real>* EdgeNeighbor(OctNode<NodeData,Real>* node,int o,int i1,int i2);
    OctNode<NodeData,Real>* CornerNeighbor(OctNode<NodeData,Real>* node,int x,int y,int z);

    static void CornerIndex(const int& c,int idx[3]);
    static void EdgeIndex(const int& c,int idx[3]);
    static void FaceIndex(const int& c,int idx[3]);

private:
    void __GetCornerNeighbors(OctNode<NodeData,Real>* node,const int& depth,const int& c,OctNode<NodeData,Real>* neighbors[Cube::CORNERS]);
    OctNode<NodeData,Real>* __FaceNeighbor(OctNode<NodeData,Real>* node,const int& depth,int dir,int off);
    OctNode<NodeData,Real>* __EdgeNeighbor(OctNode<NodeData,Real>* node,const int& depth,int o,int i1,int i2);
    OctNode<NodeData,Real>* __CornerNeighbor(OctNode<NodeData,Real>* node,const int& depth,int x,int y,int z);
};


#include "NeighborKey.inl"

#endif // NEIGHBOR_KEY_INCLUDED
