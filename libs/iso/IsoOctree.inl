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

#include <stdio.h>
#include <stdlib.h>

#include "fssr/iso_octree.h"


///////////////
// IsoOctree //
///////////////

template<class NodeData, class Real, class VertexData>
int
IsoOctree<NodeData,Real,VertexData>::getRootPosition(
    const OctNode<NodeData,Real>* node,
    const typename OctNode<NodeData,Real>::NodeIndex& nIdx,
    const int& eIndex,
    const Real& isoValue,
    VertexType& position,
    VertexData& vertex_data)
{
    int c0,c1;
    Cube::EdgeCorners(eIndex,c0,c1);

    if(!MarchingCubes::HasEdgeRoots(node->nodeData.mcIndex,eIndex))
        return 0;

    /* Interpolate vertex position/attributes according to corner values. */

    Real w;
    VertexType p1,p2;
    int x,y,z;

    OctNode<NodeData,Real>::CenterAndWidth(nIdx,p1,w);
    OctNode<NodeData,Real>::CenterAndWidth(nIdx,p2,w);

    Cube::FactorCornerIndex(c0,x,y,z);
    p1[0]+=-w/2+w*x;
    p1[1]+=-w/2+w*y;
    p1[2]+=-w/2+w*z;
    Cube::FactorCornerIndex(c1,x,y,z);
    p2[0]+=-w/2+w*x;
    p2[1]+=-w/2+w*y;
    p2[2]+=-w/2+w*z;

    // SIMON: Modified code
    VertexData v1 = cornerValues[OctNode<NodeData,Real>::CornerIndex(nIdx,c0,maxDepth)];
    VertexData v2 = cornerValues[OctNode<NodeData,Real>::CornerIndex(nIdx,c1,maxDepth)];
    Real t = (v1.value - isoValue) / (v1.value - v2.value);
    position = p1 * (Real(1.0) - t) + p2 * t;
    vertex_data = fssr::interpolate(v1, 1.0f - t, v2, t);

    return 1;
}

/*
 * Assign a unique key to a root along an edge given an octree node and an
 * edge index within that node. The key is an edge index which is unique
 * for every edge on an octree level. The edge index is:
 *
 *     offset  index1  index0  orientation
 *     19 bit  20 bit  20 bit    5 bit
 */
template<class NodeData,class Real,class VertexData>
long long
IsoOctree<NodeData,Real,VertexData>::getRootKey(
    const typename OctNode<NodeData,Real>::NodeIndex& nIdx,
    const int& edgeIndex)
{
    int o,i1,i2;
    Cube::FactorEdgeIndex(edgeIndex,o,i1,i2);
    int offset = BinaryNode<Real>::Index(nIdx.depth, nIdx.offset[o]);

    int eIndex[2];
    switch(o)
    {
    case 0:
        eIndex[0]=BinaryNode<Real>::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[1],i1);
        eIndex[1]=BinaryNode<Real>::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[2],i2);
        break;
    case 1:
        eIndex[0]=BinaryNode<Real>::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[0],i1);
        eIndex[1]=BinaryNode<Real>::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[2],i2);
        break;
    case 2:
        eIndex[0]=BinaryNode<Real>::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[0],i1);
        eIndex[1]=BinaryNode<Real>::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[1],i2);
        break;
    default: // SIMON
        eIndex[0] = 0;
        eIndex[1] = 0;
        fprintf(stderr, "Error setting eIndex in %s:%d\n", __FILE__, __LINE__);
    }
    return (long long)(o) | (long long)(eIndex[0])<<5 | (long long)(eIndex[1])<<25 | (long long)(offset)<<45;
}

template<class NodeData,class Real,class VertexData>
int
IsoOctree<NodeData,Real,VertexData>::getRootIndex(
    const OctNode<NodeData,Real>* node,
    const typename OctNode<NodeData,Real>::NodeIndex& nIdx,
    const int& edgeIndex,
    RootInfo& ri)
{
    /*
     * Find the finest edge coincident with the given one, that contains the
     * given root (see "Defining Consistent Isovertices" in the paper). If
     * there is an uneven number of zero-crossings along an edge, this
     * algorithm finds the "lone" root that has no twin vertex
     * (cf. getRootPair). This is needed to assign a unique index to each
     * iso-vertex so that neighboring nodes share iso-vertices on coincident
     * edges.
     */


    // The assumption is that the super-edge has one root along it.
    if(!(MarchingCubes::HasEdgeRoots(node->nodeData.mcIndex, edgeIndex)))
        return 0;

    // Special treatment if at max level.
    if(nIdx.depth==maxDepth)
    {
        ri.node=node;
        ri.edgeIndex=edgeIndex;
        ri.nIdx=nIdx;
        ri.key=getRootKey(nIdx,edgeIndex);
        return 1;
    }

    /* Initialize finest node and finest edge with current values. */
    const OctNode<NodeData,Real>* finest = node;
    typename OctNode<NodeData,Real>::NodeIndex finestNIdx = nIdx;
    int finestIndex = edgeIndex;

    /* Query faces 1 and 2 of this node adjacent to the edge. */
    int f1, f2;
    Cube::FacesAdjacentToEdge(edgeIndex,f1,f2);

    /* Check for finer edges in adjacent nodes. */
    if(nIdx.depth < maxDepth)
    {
        if(!node->children)
        {
            /* Check node neighboring to face 1. */
            const OctNode<NodeData,Real>* temp = node->faceNeighbor(f1);
            if(temp && temp->children)
            {
                finest = temp;
                finestIndex = Cube::FaceReflectEdgeIndex(edgeIndex,f1);
                int dir,off;
                Cube::FactorFaceIndex(f1,dir,off);
                if(off)
                    finestNIdx.offset[dir]++;
                else
                    finestNIdx.offset[dir]--;
            }
            else
            {
                /* Check node neighboring to face 2. */
                const OctNode<NodeData,Real>* temp = node->faceNeighbor(f2);
                if(temp && temp->children)
                {
                    finest=temp;
                    finestIndex=Cube::FaceReflectEdgeIndex(edgeIndex,f2);
                    int dir,off;
                    Cube::FactorFaceIndex(f2,dir,off);
                    if(off)
                        finestNIdx.offset[dir]++;
                    else
                        finestNIdx.offset[dir]--;
                }
                else
                {
                    /* Check node opposite to this node across the edge. */
                    const OctNode<NodeData,Real>* temp = node->edgeNeighbor(edgeIndex);
                    if(temp && temp->children)
                    {
                        finest=temp;
                        finestIndex=Cube::EdgeReflectEdgeIndex(edgeIndex);
                        int o,i1,i2;
                        Cube::FactorEdgeIndex(edgeIndex,o,i1,i2);
                        switch(o)
                        {
                        case 0:
                            if(i1)	finestNIdx.offset[1]++;
                            else	finestNIdx.offset[1]--;
                            if(i2)	finestNIdx.offset[2]++;
                            else	finestNIdx.offset[2]--;
                            break;
                        case 1:
                            if(i1)	finestNIdx.offset[0]++;
                            else	finestNIdx.offset[0]--;
                            if(i2)	finestNIdx.offset[2]++;
                            else	finestNIdx.offset[2]--;
                            break;
                        case 2:
                            if(i1)	finestNIdx.offset[0]++;
                            else	finestNIdx.offset[0]--;
                            if(i2)	finestNIdx.offset[1]++;
                            else	finestNIdx.offset[1]--;
                            break;
                        }
                    }
                }
            }
        }
    }

    int c1,c2;
    Cube::EdgeCorners(finestIndex,c1,c2);
    if(finest->children)
    {
        if (getRootIndex(&finest->children[c1],finestNIdx.child(c1),finestIndex,ri))
            return 1;
        else if	(getRootIndex(&finest->children[c2],finestNIdx.child(c2),finestIndex,ri))
            return 1;
        else
        {
            fprintf(stderr,"Failed to find root index in %s:%d\n", __FILE__, __LINE__);
            return 0;
        }
    }
    else
    {
        ri.nIdx=finestNIdx;
        ri.node=finest;
        ri.edgeIndex=finestIndex;
        ri.key=getRootKey(finestNIdx, finestIndex);
        return 1;
    }
}

/*
 * This method computes for every leaf with corresponding index the "roots",
 * i.e. isovertices on edges of the cube, and generates a vertex position
 * and vertex data for every root.
 */
template<class NodeData,class Real,class VertexData>
void IsoOctree<NodeData,Real,VertexData>::getRoots(
    const OctNode<NodeData,Real>* node,
    const typename OctNode<NodeData,Real>::NodeIndex& nIdx,
    const Real& isoValue,
    stdext::hash_map<long long,int>& roots,
    std::vector<VertexType>& vertices,
    std::vector<VertexData>& vdata)
{
    /* Exit if the leaf does not have a root (mcIndex == 0 or 255). */
    if(!MarchingCubes::HasRoots(node->nodeData.mcIndex))
        return;

    /* Iterate over all edges of the cube. */
    for(int eIndex=0; eIndex<Cube::EDGES; eIndex++)
    {
        /* Skip edge if both endpoints are on the same side of isovalue. */
        if(!(MarchingCubes::HasEdgeRoots(node->nodeData.mcIndex, eIndex)))
            continue;

        /* Compute root index (finest edge with isovertex on it). */
        RootInfo ri; // Node, node index, edge index and edge key
        if(getRootIndex(node, nIdx, eIndex, ri))
        {
            /* Check if root index is already registered. */
            if(roots.find(ri.key)==roots.end()){
                VertexType position;
                VertexData vertex_data;
                /* Get position by interpolation and obtain vertex data. */
                getRootPosition(ri.node,ri.nIdx,ri.edgeIndex,isoValue,position, vertex_data);
                vertices.push_back(position);
                vdata.push_back(vertex_data);
                /* Register root index and map to vertex ID. */
                roots[ri.key]=int(vertices.size())-1;
            }
        }
        else
            fprintf(stderr,"Failed to get root index in %s:%d\n", __FILE__, __LINE__);
    }
}

template<class NodeData,class Real,class VertexData>
int
IsoOctree<NodeData,Real,VertexData>::getRootPair(
    const RootInfo& ri,
    const int& /*maxDepth*/,
    RootInfo& pair)
{
    /*
     * Get the twin isovertex to a given one, i.e., find another root
     * in neighboring leafs of the edge tree without crossing the iso-surface.
     * (see figure 3)
     */

    const OctNode<NodeData,Real>* node=ri.node;
    typename OctNode<NodeData,Real>::NodeIndex nIdx=ri.nIdx;
    int c1,c2,c;
    Cube::EdgeCorners(ri.edgeIndex,c1,c2);
    while(node->parent)
    {
        c=int(node-node->parent->children);
        if(c!=c1 && c!=c2)
            return 0;

        /* Check edge "value". */
        if(!MarchingCubes::HasEdgeRoots(node->parent->nodeData.mcIndex,ri.edgeIndex))
        {
            if(c==c1)
                return getRootIndex(&node->parent->children[c2],nIdx.parent().child(c2),ri.edgeIndex,pair);
            else
                return getRootIndex(&node->parent->children[c1],nIdx.parent().child(c2),ri.edgeIndex,pair);
        }

        /* Go to parent edge. */
        node=node->parent;
        --nIdx;
    }
    return 0;
}

template<class NodeData,class Real,class VertexData>
void
IsoOctree<NodeData,Real,VertexData>::getIsoFaceEdges(
    const OctNode<NodeData,Real>* node,
    const typename OctNode<NodeData,Real>::NodeIndex& nIdx,
    const int& faceIndex,
    std::vector<std::pair<RootInfo,RootInfo> >& edges,
    const int& flip)
{
    /* Get all iso-edges that lie fully within a cube face (node face). */
    int c1,c2,c3,c4;
    if(node->children)
    {
        /* Recursively get face edges of the faces in the four finer nodes. */
        Cube::FaceCorners(faceIndex,c1,c2,c3,c4);
        getIsoFaceEdges(&node->children[c1],nIdx.child(c1),faceIndex,edges,flip);
        getIsoFaceEdges(&node->children[c2],nIdx.child(c2),faceIndex,edges,flip);
        getIsoFaceEdges(&node->children[c3],nIdx.child(c3),faceIndex,edges,flip);
        getIsoFaceEdges(&node->children[c4],nIdx.child(c4),faceIndex,edges,flip);
    }
    else
    {
        /* Get MC index and MC case table to connect isovertices. */
        int idx = node->nodeData.mcIndex;
        RootInfo ri1,ri2;
        const std::vector<std::vector<int> >& table = MarchingCubes::caseTable(idx);
        for(size_t i=0; i<table.size(); i++)
        {
            size_t pSize=table[i].size();
            for(size_t j=0;j<pSize;j++)
            {
                /* If the two edges form a face... */
                if(faceIndex==Cube::FaceAdjacentToEdges(table[i][j],table[i][(j+1)%pSize]))
                {
                    /* connect the two roots. */
                    if(getRootIndex(node,nIdx,table[i][j],ri1) && getRootIndex(node,nIdx,table[i][(j+1)%pSize],ri2))
                    {
                        if(flip)
                            edges.push_back(std::pair<RootInfo,RootInfo>(ri2,ri1));
                        else
                            edges.push_back(std::pair<RootInfo,RootInfo>(ri1,ri2));
                    }
                    else
                        fprintf(stderr,"Bad Edge 1: %lld %lld\n",ri1.key,ri2.key);
                }
            }
        }
    }
}

template<class NodeData,class Real,class VertexData>
void
IsoOctree<NodeData,Real,VertexData>::getIsoPolygons(
    OctNode<NodeData,Real>* node,
    const typename OctNode<NodeData,Real>::NodeIndex& nIdx,
    stdext::hash_map<long long,int>& roots,
    std::vector<std::vector<int> >& polygons,
    NeighborKey<NodeData,Real>& nKey)
{
    /* Prepare neighborhood cache data structure. */
    nKey.getNeighbors(node);

    /* Iterate over node faces, find finest face and getIsoFaceEdges. */
    int x[3] = { 1, 1, 1 };
    std::vector<std::pair<RootInfo,RootInfo> > riEdges;
    for(int i=0;i<3;i++) // iterate over axis directions
    {
        for(int j=0;j<2;j++) // iterate over axis orientations
        {
            x[i]=j<<1;
            /* Use this node's faces if face-neighboring nodes have no children. */
            if(!nKey.neighbors[nIdx.depth].neighbors[x[0]][x[1]][x[2]]
                || !nKey.neighbors[nIdx.depth].neighbors[x[0]][x[1]][x[2]]->children)
                getIsoFaceEdges(node,nIdx,Cube::FaceIndex(i,j),riEdges,0);
            else
            {
                /* If the coincident face in the neighboring node provides a
                 * finer subdivision, use it. */
                typename OctNode<NodeData,Real>::NodeIndex idx=nIdx;
                if(j)	idx.offset[i]++;
                else	idx.offset[i]--;
                getIsoFaceEdges(nKey.neighbors[idx.depth].neighbors[x[0]][x[1]][x[2]],
                    idx,Cube::FaceIndex(i,j^1),riEdges,1);
            }
        }
        x[i]=1;
    }

    /*
     * establish invariant: vertexCount == outgoingEdges - incomingEdges
     * check vertexCount to find "open isovertices" (cf. figure 3).
     */
    typedef stdext::hash_map<long long,std::pair<RootInfo,int> > EdgeRootMap; // EdgeKey -> (RootInfo,Counter)
    EdgeRootMap vertexCount;
    std::vector<std::pair<long long,long long> > edges; // EdgeKey -> EdgeKey
    for(size_t i=0;i<riEdges.size();i++)
    {
        typename EdgeRootMap::iterator iter;

        edges.push_back(std::pair<long long,long long>(riEdges[i].first.key,riEdges[i].second.key));
        iter=vertexCount.find(riEdges[i].first.key);
        if(iter==vertexCount.end())
        {
            vertexCount[riEdges[i].first.key].first=riEdges[i].first;
            vertexCount[riEdges[i].first.key].second=0;
        }
        iter=vertexCount.find(riEdges[i].second.key);
        if(iter==vertexCount.end())
        {
            vertexCount[riEdges[i].second.key].first=riEdges[i].second;
            vertexCount[riEdges[i].second.key].second=0;
        }
        vertexCount[riEdges[i].first.key ].second++;
        vertexCount[riEdges[i].second.key].second--;
    }

    /*
     * "edges" now contains all edges between roots on adjacent cube edges,
     * now add more edges to guarantee closed loops (proof in paper).
     */

    for(int i=0;i<int(edges.size());i++)
    {
        typename EdgeRootMap::iterator iter;

        iter=vertexCount.find(edges[i].first);
        if(iter==vertexCount.end())
            printf("Could not find vertex: %lld\n",edges[i].first);
        else if(vertexCount[edges[i].first].second)
        {
            /* Start vertex is "unbalanced", add an incoming edge to twin vertex. */

            RootInfo ri;
            if(!getRootPair(vertexCount[edges[i].first].first,maxDepth,ri))
                fprintf(stderr,"Failed to get root pair 1: %lld %d\n",edges[i].first,vertexCount[edges[i].first].second);
            iter=vertexCount.find(ri.key);
            if(iter==vertexCount.end())
                printf("Vertex pair not in list\n");
            else
            {
                edges.push_back(std::pair<long long,long long>(ri.key,edges[i].first));
                vertexCount[ri.key].second++;
                vertexCount[edges[i].first].second--;
            }
        }

        iter=vertexCount.find(edges[i].second);
        if(iter==vertexCount.end())
            printf("Could not find vertex: %lld\n",edges[i].second);
        else if(vertexCount[edges[i].second].second)
        {
            /* End vertex is "unbalanced", add an outgoing edge to twin vertex. */

            RootInfo ri;
            if(!getRootPair(vertexCount[edges[i].second].first,maxDepth,ri))
                fprintf(stderr,"Failed to get root pair 2: %lld %d\n",edges[i].second,vertexCount[edges[i].second].second);
            iter=vertexCount.find(ri.key);
            if(iter==vertexCount.end())
                printf("Vertex pair not in list\n");
            else
            {
                edges.push_back(std::pair<long long,long long>(edges[i].second,ri.key));
                vertexCount[edges[i].second].second++;
                vertexCount[ri.key].second--;
            }
        }
    }
    getEdgeLoops(edges,roots,polygons);
}

template<class NodeData,class Real,class VertexData>
void
IsoOctree<NodeData,Real,VertexData>::getEdgeLoops(
    std::vector<std::pair<long long,long long> >& edges, // EdgeKey -> EdgeKey
    stdext::hash_map<long long,int>& roots, // EdgeKey -> Vertex ID
    std::vector<std::vector<int> >& polygons)
{
    /* Join all edges to form polygons, see section "Closing the Isopolylines". */
    size_t polygonSize = polygons.size();
    while(edges.size())
    {
        std::vector<std::pair<long long,long long> > front, back;

        /* Grab an edge and start a new polygon. */
        std::pair<long long,long long> e = edges[0];
        polygons.resize(polygonSize+1);
        edges[0]=edges[edges.size()-1];
        edges.pop_back();

        long long frontIdx = e.second;
        long long backIdx = e.first;

        /* Find all edges that share a point with one of the ends of the polyline (not yet a loop). */
        std::pair<long long,long long> temp;
        for(int j=int(edges.size())-1;j>=0;j--){
            if(edges[j].first==frontIdx || edges[j].second==frontIdx){
                if(edges[j].first==frontIdx)
                    {temp=edges[j];}
                else
                    {temp.first=edges[j].second;temp.second=edges[j].first;}
                frontIdx=temp.second;
                front.push_back(temp);
                edges[j]=edges[edges.size()-1];
                edges.pop_back();
                j=int(edges.size());
            }
            else if(edges[j].first==backIdx || edges[j].second==backIdx){
                if(edges[j].second==backIdx)
                    {temp=edges[j];}
                else
                    {temp.first=edges[j].second;temp.second=edges[j].first;}
                backIdx=temp.first;
                back.push_back(temp);
                edges[j]=edges[edges.size()-1];
                edges.pop_back();
                j=int(edges.size());
            }
        }

        /* Collect iso-vertices to form polygon. */
        polygons[polygonSize].resize(back.size()+front.size()+1);
        int idx=0;
        for(int j=int(back.size())-1;j>=0;j--)
            polygons[polygonSize][idx++]=roots[back[j].first];
        polygons[polygonSize][idx++]=roots[e.first];
        for(int j=0;j<int(front.size());j++)
            polygons[polygonSize][idx++]=roots[front[j].first];
        polygonSize++;
    }
}

template<class NodeData,class Real,class VertexData>
void
IsoOctree<NodeData,Real,VertexData>::setMCIndex(const Real& isoValue)
{
    /* Reset all MC indices to 0. */
    OctNode<NodeData,Real>* temp;
    for(temp = tree.nextNode(NULL) ; temp ; temp=tree.nextNode(temp) )
        temp->nodeData.mcIndex=0;

    /* Iterate all leafs, look up corner values and set MC index. */
    typename OctNode<NodeData,Real>::NodeIndex nIdx;
    for(temp=tree.nextLeaf(NULL,nIdx) ; temp ; temp=tree.nextLeaf(temp,nIdx) )
    {
        Real cValues[Cube::CORNERS]; // Implicit function values
        for(int i=0;i<Cube::CORNERS;i++)
        {
            if(cornerValues.find(OctNode<NodeData,Real>::CornerIndex(nIdx,i,maxDepth))==cornerValues.end())
                fprintf(stderr,"Could not find value in corner value table for %d/%d/%d/%d!\n", nIdx.depth, nIdx.offset[0], nIdx.offset[1], nIdx.offset[2]);

            // SIMON change
            VertexData data = cornerValues[OctNode<NodeData,Real>::CornerIndex(nIdx,i,maxDepth)];
            cValues[i] = data.value;
        }

        /* Assign marching cubes indices according to sign pattern of node corners. */
        temp->nodeData.mcIndex = MarchingCubes::GetIndex(cValues,isoValue);

        /* Set MC indices of inner nodes from child values. */
        if(temp->parent)
        {
            int cIndex = int(temp - temp->parent->children);
            int bitFlag = temp->nodeData.mcIndex & (1<<cIndex);
            if(bitFlag)
            {
                OctNode<NodeData,Real> *parent,*child;
                child=temp;
                parent=temp->parent;
                while(parent && (child-parent->children)==cIndex)
                {
                    parent->nodeData.mcIndex |= bitFlag;
                    child=parent;
                    parent=parent->parent;
                }
            }
        }
    }
}


template<class NodeData,class Real,class VertexData>
void
IsoOctree<NodeData,Real,VertexData>::getIsoSurface(
    const Real& isoValue,
    std::vector<VertexType>& vertices,
    std::vector<VertexData>& vertex_data,
    std::vector<std::vector<int> >& polygons)
{
    /* (1) Set marching cubes values. */
    setMCIndex(isoValue);

    /*
     * (2) Defining Consistent Isovertices (see paper)
     * - iterate leaf nodes, compute all node roots for every leaf
     * - roots are isovertices on octree edges
     * - vertices and data are added to vectors
     */
    stdext::hash_map<long long,int> roots;
    {
        OctNode<NodeData,Real>* node;
        typename OctNode<NodeData,Real>::NodeIndex nIdx;
        for(node=tree.nextLeaf(NULL,nIdx); node; node=tree.nextLeaf(node,nIdx))
            getRoots(node,nIdx,isoValue,roots,vertices,vertex_data);
    }

    /*
     * (3) Closing the Isopolylines (see paper)
     * - iterate leaf nodes, compute polygons for every leaf
     */
    {
        NeighborKey<NodeData,Real> nKey;
        nKey.set(maxDepth);
        OctNode<NodeData,Real>* node;
        typename OctNode<NodeData,Real>::NodeIndex nIdx;
        for(node=tree.nextLeaf(NULL,nIdx); node; node=tree.nextLeaf(node,nIdx))
            getIsoPolygons(node,nIdx,roots,polygons,nKey);
    }
}
