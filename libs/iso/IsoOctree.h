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

#ifndef ISO_OCTREE_INCLUDED
#define ISO_OCTREE_INCLUDED

#ifdef USE_HASHMAP
#   include <hash_map>
#else
#   include <map>
#   define hash_map map
#   define stdext std
#endif

#include <vector>

#include "math/vector.h"
#include "MarchingCubes.h"
#include "Octree.h"
#include "NeighborKey.h"

template<class NodeData,class Real,class VertexData>
class IsoOctree
{
public:  // Types
    // SIMON The vertex type used.
    typedef math::Vector<Real, 3> VertexType;

public:  // Variables
    // The maximum depth of the tree. This value must be at least as large as the true depth of the tree
    // as its used for assigning unique ids. (It can, however, be larger than the depth for uniqueness to still hold.)
    int maxDepth;

    /*
     * SIMON:
     * The NodeData contains a mcIndex, a 3D position and VertexData.
     */

    // The octree itself
    OctNode<NodeData,Real> tree;

    // A hash-table of data associated to the corners of the octree nodes
    stdext::hash_map<long long, VertexData> cornerValues;

public:  // Methods
    // Extracts an iso-surface from the octree
    void getIsoSurface(
        const Real& isoValue,
        std::vector<VertexType>& vertices,
        std::vector<VertexData>& vertex_data,
        std::vector<std::vector<int> >& polygons);

private:  // Types
    class RootInfo
    {
    public:
        const OctNode<NodeData,Real>* node;
        int edgeIndex;
        long long key;
        typename OctNode<NodeData,Real>::NodeIndex nIdx;
    };

private:  // Methods
    void getRoots(const OctNode<NodeData,Real>* node,const typename OctNode<NodeData,Real>::NodeIndex& nIdx,const Real& isoValue,stdext::hash_map<long long,int>& roots,std::vector<VertexType>& vertices,std::vector<VertexData>& vertex_data);
    int getRootIndex(const OctNode<NodeData,Real>* node,const typename OctNode<NodeData,Real>::NodeIndex& nIdx,const int& edgeIndex,RootInfo& ri);
    int getRootPosition(const OctNode<NodeData,Real>* node,const typename OctNode<NodeData,Real>::NodeIndex& nIdx,const int& eIndex,const Real& isoValue, VertexType& position, VertexData& vertex_data);
    long long getRootKey(const typename OctNode<NodeData,Real>::NodeIndex& nIdx,const int& edgeIndex);

    int getRootPair(const RootInfo& root,const int& maxDepth,RootInfo& pair);

    void getIsoFaceEdges(
        const OctNode<NodeData,Real>* node,
        const typename OctNode<NodeData,Real>::NodeIndex& nIdx,
        const int& faceIndex,
        std::vector<std::pair<RootInfo,RootInfo> >& edges,
        const int& flip);

    void getIsoPolygons(OctNode<NodeData,Real>* node,const typename OctNode<NodeData,Real>::NodeIndex& nIdx,stdext::hash_map<long long,int>& roots,std::vector<std::vector<int> >& polygons,NeighborKey<NodeData,Real>& nKey);

    template<class C>
    void getEdgeLoops(std::vector<std::pair<C,C> >& edges,stdext::hash_map<C,int>& roots,std::vector<std::vector<int> >& polygons);

    // Assumes NodeData::mcIndex
    void setMCIndex(const Real& isoValue);
};

#include "IsoOctree.inl"

#endif // ISO_OCTREE_INCLUDED
