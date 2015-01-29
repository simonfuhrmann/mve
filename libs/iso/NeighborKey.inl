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

#ifndef NEIGHBORKEY_INL
#define NEIGHBORKEY_INL

/*
 * Neighbors
 */

template<class NodeData,class Real>
Neighbors<NodeData,Real>::Neighbors(void)
{
    clear();
}

template<class NodeData,class Real>
void
Neighbors<NodeData,Real>::clear(void)
{
    for(int i=0;i<3;i++)
        for(int j=0;j<3;j++)
            for(int k=0;k<3;k++)
                neighbors[i][j][k]=NULL;
}

/*
 * NeighborKey
 */

template<class NodeData,class Real>
NeighborKey<NodeData,Real>::NeighborKey(void)
{
    neighbors=NULL;
}

template<class NodeData,class Real>
NeighborKey<NodeData,Real>::~NeighborKey(void)
{
    delete [] neighbors;
    neighbors=NULL;
}

template<class NodeData,class Real>
void
NeighborKey<NodeData,Real>::set(const int& d)
{
    delete [] neighbors;
    neighbors=NULL;

    if(d<0)
        return;

    /* Allocate depth+1 3x3 cube neighborhoods. */
    _depth=d;
    neighbors=new Neighbors<NodeData,Real>[d+1];
}

template<class NodeData,class Real>
Neighbors<NodeData,Real>&
NeighborKey<NodeData,Real>::getNeighbors(OctNode<NodeData,Real>* node)
{
    depth=0;

    /* Iterate from the current node to the octree root and count depth. */
    OctNode<NodeData,Real>* temp = node;
    while(temp->parent)
    {
        depth++;
        temp=temp->parent;
    }

    /* Set all pointers to NULL if central node is not correct. */
    if(node != neighbors[depth].neighbors[1][1][1])
        for(int i=depth;i<=_depth;i++)
            neighbors[i].clear();

    /* Return neighbors for current node at depth. */
    return _getNeighbors(node,depth);
}

template<class NodeData,class Real>
Neighbors<NodeData,Real>&
NeighborKey<NodeData,Real>::_getNeighbors(OctNode<NodeData,Real>* node,const int& d)
{
    /* If node is already set, immediately return. */
    if (node == neighbors[d].neighbors[1][1][1])
        return neighbors[d];

    neighbors[d].clear();
    if(!node->parent)
    {
        /* Node is octree root. */
        neighbors[d].neighbors[1][1][1]=node;
        neighbors[d].nIndex = typename OctNode<NodeData,Real>::NodeIndex();
    }
    else
    {
        /* Recursively get neighborhood for previous level. */
        Neighbors<NodeData,Real>& temp=_getNeighbors(node->parent,d-1);

        /* Recursively define the node index. */
        int idx=int(node-node->parent->children);
        neighbors[d].nIndex = neighbors[d-1].nIndex.child(idx);

        int x1,y1,z1,x2,y2,z2;
        Cube::FactorCornerIndex(  idx   ,x1,y1,z1);
        Cube::FactorCornerIndex((~idx)&7,x2,y2,z2);
        int i,j,k;
        for(i=0;i<2;i++){
            for(j=0;j<2;j++){
                for(k=0;k<2;k++){
                    neighbors[d].neighbors[x2+i][y2+j][z2+k] = &node->parent->children[Cube::CornerIndex(i,j,k)];
                }
            }
        }

        // Set the neighbors from across the faces
        i=x1<<1;
        if(temp.neighbors[i][1][1] && temp.neighbors[i][1][1]->children){
            for(j=0;j<2;j++){
                for(k=0;k<2;k++){
                    neighbors[d].neighbors[i][y2+j][z2+k]=&temp.neighbors[i][1][1]->children[Cube::CornerIndex(x2,j,k)];}}
        }
        j=y1<<1;
        if(temp.neighbors[1][j][1] && temp.neighbors[1][j][1]->children){
            for(i=0;i<2;i++){
                for(k=0;k<2;k++){
                    neighbors[d].neighbors[x2+i][j][z2+k]=&temp.neighbors[1][j][1]->children[Cube::CornerIndex(i,y2,k)];}}
        }
        k=z1<<1;
        if(temp.neighbors[1][1][k] && temp.neighbors[1][1][k]->children){
            for(i=0;i<2;i++){
                for(j=0;j<2;j++){
                    neighbors[d].neighbors[x2+i][y2+j][k]=&temp.neighbors[1][1][k]->children[Cube::CornerIndex(i,j,z2)];}}
        }

        // Set the neighbors from across the edges
        i=x1<<1;	j=y1<<1;
        if(temp.neighbors[i][j][1] && temp.neighbors[i][j][1]->children){
            for(k=0;k<2;k++){
                neighbors[d].neighbors[i][j][z2+k]=&temp.neighbors[i][j][1]->children[Cube::CornerIndex(x2,y2,k)];}
        }
        i=x1<<1;	k=z1<<1;
        if(temp.neighbors[i][1][k] && temp.neighbors[i][1][k]->children){
            for(j=0;j<2;j++){
                neighbors[d].neighbors[i][y2+j][k]=&temp.neighbors[i][1][k]->children[Cube::CornerIndex(x2,j,z2)];}
        }
        j=y1<<1;	k=z1<<1;
        if(temp.neighbors[1][j][k] && temp.neighbors[1][j][k]->children){
            for(i=0;i<2;i++){
                neighbors[d].neighbors[x2+i][j][k]=&temp.neighbors[1][j][k]->children[Cube::CornerIndex(i,y2,z2)];}
        }

        // Set the neighbor from across the corner
        i=x1<<1;	j=y1<<1;	k=z1<<1;
        if(temp.neighbors[i][j][k] && temp.neighbors[i][j][k]->children){
            neighbors[d].neighbors[i][j][k]=&temp.neighbors[i][j][k]->children[Cube::CornerIndex(x2,y2,z2)];
        }
    }

    return neighbors[d];
}

#endif // NEIGHBORKEY_INL
