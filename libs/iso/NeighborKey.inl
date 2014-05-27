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

////////////////////////
// OctNodeNeighborKey //
////////////////////////
template<class NodeData,class Real>
Neighbors<NodeData,Real>::Neighbors(void)
{
    clear();
}

template<class NodeData,class Real>
void
Neighbors<NodeData,Real>::clear(void)
{
    for(int i=0;i<3;i++){for(int j=0;j<3;j++){for(int k=0;k<3;k++){neighbors[i][j][k]=NULL;}}}
}

template<class NodeData,class Real>
NeighborKey<NodeData,Real>::NeighborKey(void)
{
    neighbors=NULL;
}

template<class NodeData,class Real>
NeighborKey<NodeData,Real>::~NeighborKey(void)
{
    if(neighbors){delete[] neighbors;}
    neighbors=NULL;
}

template<class NodeData,class Real>
void
NeighborKey<NodeData,Real>::set(const int& d)
{
    if(neighbors){delete[] neighbors;}
    neighbors=NULL;
    if(d<0){return;}
    _depth=d;
    neighbors=new Neighbors<NodeData,Real>[d+1];
}

template<class NodeData,class Real>
Neighbors<NodeData,Real>&
NeighborKey<NodeData,Real>::setNeighbors(OctNode<NodeData,Real>* node)
{
    OctNode<NodeData,Real>* temp=node;
    depth=0;
    while(temp->parent)
    {
        depth++;
        temp=temp->parent;
    }
    if(node!=neighbors[depth].neighbors[1][1][1])
        for(int i=depth;i<=_depth;i++)
            neighbors[i].clear();

    return _setNeighbors(node,depth);
}

template<class NodeData,class Real>
Neighbors<NodeData,Real>&
NeighborKey<NodeData,Real>::_setNeighbors(OctNode<NodeData,Real>* node,const int& d)
{
    if(node!=neighbors[d].neighbors[1][1][1]){
        neighbors[d].clear();

        if(!node->parent)
        {
            neighbors[d].nIndex=OctNode<NodeData,Real>::NodeIndex();
            neighbors[d].neighbors[1][1][1]=node;
        }
        else
        {
            Neighbors<NodeData,Real>& temp=_setNeighbors(node->parent,d-1);
            int i,j,k,x1,y1,z1,x2,y2,z2;
            int idx=int(node-node->parent->children);
            neighbors[d].nIndex=neighbors[d-1].nIndex.child(idx);
            Cube::FactorCornerIndex(  idx   ,x1,y1,z1);
            Cube::FactorCornerIndex((~idx)&7,x2,y2,z2);
            for(i=0;i<2;i++){
                for(j=0;j<2;j++){
                    for(k=0;k<2;k++){
                        neighbors[d].neighbors[x2+i][y2+j][z2+k]=&node->parent->children[Cube::CornerIndex(i,j,k)];
                    }
                }
            }

            // Set the neighbors from across the faces
            i=x1<<1;
            if(temp.neighbors[i][1][1]){
                if(!temp.neighbors[i][1][1]->children){temp.neighbors[i][1][1]->initChildren();}
                for(j=0;j<2;j++){for(k=0;k<2;k++){neighbors[d].neighbors[i][y2+j][z2+k]=&temp.neighbors[i][1][1]->children[Cube::CornerIndex(x2,j,k)];}}
            }
            j=y1<<1;
            if(temp.neighbors[1][j][1]){
                if(!temp.neighbors[1][j][1]->children){temp.neighbors[1][j][1]->initChildren();}
                for(i=0;i<2;i++){for(k=0;k<2;k++){neighbors[d].neighbors[x2+i][j][z2+k]=&temp.neighbors[1][j][1]->children[Cube::CornerIndex(i,y2,k)];}}
            }
            k=z1<<1;
            if(temp.neighbors[1][1][k]){
                if(!temp.neighbors[1][1][k]->children){temp.neighbors[1][1][k]->initChildren();}
                for(i=0;i<2;i++){for(j=0;j<2;j++){neighbors[d].neighbors[x2+i][y2+j][k]=&temp.neighbors[1][1][k]->children[Cube::CornerIndex(i,j,z2)];}}
            }

            // Set the neighbors from across the edges
            i=x1<<1;	j=y1<<1;
            if(temp.neighbors[i][j][1]){
                if(!temp.neighbors[i][j][1]->children){temp.neighbors[i][j][1]->initChildren();}
                for(k=0;k<2;k++){neighbors[d].neighbors[i][j][z2+k]=&temp.neighbors[i][j][1]->children[Cube::CornerIndex(x2,y2,k)];}
            }
            i=x1<<1;	k=z1<<1;
            if(temp.neighbors[i][1][k]){
                if(!temp.neighbors[i][1][k]->children){temp.neighbors[i][1][k]->initChildren();}
                for(j=0;j<2;j++){neighbors[d].neighbors[i][y2+j][k]=&temp.neighbors[i][1][k]->children[Cube::CornerIndex(x2,j,z2)];}
            }
            j=y1<<1;	k=z1<<1;
            if(temp.neighbors[1][j][k]){
                if(!temp.neighbors[1][j][k]->children){temp.neighbors[1][j][k]->initChildren();}
                for(i=0;i<2;i++){neighbors[d].neighbors[x2+i][j][k]=&temp.neighbors[1][j][k]->children[Cube::CornerIndex(i,y2,z2)];}
            }

            // Set the neighbor from across the corner
            i=x1<<1;	j=y1<<1;	k=z1<<1;
            if(temp.neighbors[i][j][k]){
                if(!temp.neighbors[i][j][k]->children){temp.neighbors[i][j][k]->initChildren();}
                neighbors[d].neighbors[i][j][k]=&temp.neighbors[i][j][k]->children[Cube::CornerIndex(x2,y2,z2)];
            }
        }
    }
    return neighbors[d];
}
template<class NodeData,class Real>
Neighbors<NodeData,Real>&
NeighborKey<NodeData,Real>::getNeighbors(OctNode<NodeData,Real>* node)
{
    depth=0;
    OctNode<NodeData,Real>* temp=node;
    while(temp->parent)
    {
        depth++;
        temp=temp->parent;
    }
    if(node!=neighbors[depth].neighbors[1][1][1])
        for(int i=depth;i<=_depth;i++)
            neighbors[i].clear();

    return _getNeighbors(node,depth);
}

template<class NodeData,class Real>
Neighbors<NodeData,Real>&
NeighborKey<NodeData,Real>::_getNeighbors(OctNode<NodeData,Real>* node,const int& d)
{
    if(node!=neighbors[d].neighbors[1][1][1]){
        neighbors[d].clear();

        if(!node->parent)
        {
            neighbors[d].neighbors[1][1][1]=node;
            neighbors[d].nIndex = typename OctNode<NodeData,Real>::NodeIndex();
        }
        else
        {
            Neighbors<NodeData,Real>& temp=_getNeighbors(node->parent,d-1);
            int i,j,k,x1,y1,z1,x2,y2,z2;
            int idx=int(node-node->parent->children);
            neighbors[d].nIndex=neighbors[d-1].nIndex.child(idx);
            Cube::FactorCornerIndex(  idx   ,x1,y1,z1);
            Cube::FactorCornerIndex((~idx)&7,x2,y2,z2);
            for(i=0;i<2;i++){
                for(j=0;j<2;j++){
                    for(k=0;k<2;k++){
                        neighbors[d].neighbors[x2+i][y2+j][z2+k]=&node->parent->children[Cube::CornerIndex(i,j,k)];
                    }
                }
            }

            // Set the neighbors from across the faces
            i=x1<<1;
            if(temp.neighbors[i][1][1] && temp.neighbors[i][1][1]->children){
                for(j=0;j<2;j++){for(k=0;k<2;k++){neighbors[d].neighbors[i][y2+j][z2+k]=&temp.neighbors[i][1][1]->children[Cube::CornerIndex(x2,j,k)];}}
            }
            j=y1<<1;
            if(temp.neighbors[1][j][1] && temp.neighbors[1][j][1]->children){
                for(i=0;i<2;i++){for(k=0;k<2;k++){neighbors[d].neighbors[x2+i][j][z2+k]=&temp.neighbors[1][j][1]->children[Cube::CornerIndex(i,y2,k)];}}
            }
            k=z1<<1;
            if(temp.neighbors[1][1][k] && temp.neighbors[1][1][k]->children){
                for(i=0;i<2;i++){for(j=0;j<2;j++){neighbors[d].neighbors[x2+i][y2+j][k]=&temp.neighbors[1][1][k]->children[Cube::CornerIndex(i,j,z2)];}}
            }

            // Set the neighbors from across the edges
            i=x1<<1;	j=y1<<1;
            if(temp.neighbors[i][j][1] && temp.neighbors[i][j][1]->children){
                for(k=0;k<2;k++){neighbors[d].neighbors[i][j][z2+k]=&temp.neighbors[i][j][1]->children[Cube::CornerIndex(x2,y2,k)];}
            }
            i=x1<<1;	k=z1<<1;
            if(temp.neighbors[i][1][k] && temp.neighbors[i][1][k]->children){
                for(j=0;j<2;j++){neighbors[d].neighbors[i][y2+j][k]=&temp.neighbors[i][1][k]->children[Cube::CornerIndex(x2,j,z2)];}
            }
            j=y1<<1;	k=z1<<1;
            if(temp.neighbors[1][j][k] && temp.neighbors[1][j][k]->children){
                for(i=0;i<2;i++){neighbors[d].neighbors[x2+i][j][k]=&temp.neighbors[1][j][k]->children[Cube::CornerIndex(i,y2,z2)];}
            }

            // Set the neighbor from across the corner
            i=x1<<1;	j=y1<<1;	k=z1<<1;
            if(temp.neighbors[i][j][k] && temp.neighbors[i][j][k]->children){
                neighbors[d].neighbors[i][j][k]=&temp.neighbors[i][j][k]->children[Cube::CornerIndex(x2,y2,z2)];
            }
        }
    }
    return neighbors[d];
}

/////////////////
// IsoNeighborKey //
/////////////////
template<class NodeData,class Real>
OctNode<NodeData,Real>*
IsoNeighborKey<NodeData,Real>::__FaceNeighbor(OctNode<NodeData,Real>* node,const int& depth,int dir,int off)
{
    if(!depth)	return NULL;
    int x,y,z;
    x=y=z=1;
    switch(dir)
    {
    case 0:
        x=off<<1;
        break;
    case 1:
        y=off<<1;
        break;
    case 2:
        z=off<<1;
        break;
    }
    for(int d=depth;d>=0;d--)
        if(NeighborKey<NodeData,Real>::neighbors[d].neighbors[x][y][z])
            return NeighborKey<NodeData,Real>::neighbors[d].neighbors[x][y][z];

    return NULL;
}

template<class NodeData,class Real>
OctNode<NodeData,Real>*
IsoNeighborKey<NodeData,Real>::__EdgeNeighbor(OctNode<NodeData,Real>* node,const int& depth,int o,int i1,int i2)
{
    if(!depth)	return NULL;
    int x,y,z,cIndex,xx,yy,zz;
    x=y=z=1;

    // Check if the edge-adjacent neighbor exists at the current depth
    switch(o)
    {
    case 0:
        y=i1<<1;
        z=i2<<1;
        break;
    case 1:
        x=i1<<1;
        z=i2<<1;
        break;
    case 2:
        x=i1<<1;
        y=i2<<1;
        break;
    }
    if(NeighborKey<NodeData,Real>::neighbors[depth].neighbors[x][y][z])
        return NeighborKey<NodeData,Real>::neighbors[depth].neighbors[x][y][z];

    cIndex=int(node-node->parent->children);
    Cube::FactorCornerIndex(cIndex,xx,yy,zz);

    // Check if the node is on the corresponding edge of the parent
    switch(o)
    {
    case 0:
        if(yy==i1 && zz==i2)	return __EdgeNeighbor(node->parent,depth-1,o,i1,i2);
        break;
    case 1:
        if(xx==i1 && zz==i2)	return __EdgeNeighbor(node->parent,depth-1,o,i1,i2);
        break;
    case 2:
        if(xx==i1 && yy==i2)	return __EdgeNeighbor(node->parent,depth-1,o,i1,i2);
        break;
    }

    // Check if the node is on a face of the parent containing the edge
    switch(o)
    {
    case 0:
        if(yy==i1)	return __FaceNeighbor(node->parent,depth-1,1,yy);
        if(zz==i2)	return __FaceNeighbor(node->parent,depth-1,2,zz);
        break;
    case 1:
        if(xx==i1)	return __FaceNeighbor(node->parent,depth-1,0,xx);
        if(zz==i2)	return __FaceNeighbor(node->parent,depth-1,2,zz);
        break;
    case 2:
        if(xx==i1)	return __FaceNeighbor(node->parent,depth-1,0,xx);
        if(yy==i2)	return __FaceNeighbor(node->parent,depth-1,1,yy);
        break;
    }
    fprintf(stderr,"We shouldn't be here: Edges\n");
    return NULL;
}

template<class NodeData,class Real>
OctNode<NodeData,Real>*
IsoNeighborKey<NodeData,Real>::__CornerNeighbor(OctNode<NodeData,Real>* node,const int& depth,int x,int y,int z)
{
    if(!depth)	return NULL;
    int cIndex,xx,yy,zz;

    // Check if the edge-adjacent neighbor exists at the current depth
    if(NeighborKey<NodeData,Real>::neighbors[depth].neighbors[x<<1][y<<1][z<<1])
        return NeighborKey<NodeData,Real>::neighbors[depth].neighbors[x<<1][y<<1][z<<1];

    cIndex=int(node-node->parent->children);
    Cube::FactorCornerIndex(cIndex,xx,yy,zz);

    // Check if the node is on the corresponding corner of the parent
    if(xx==x && yy==y && zz==z)	return __CornerNeighbor(node->parent,depth-1,x,y,z);

    // Check if the node is on an edge of the parent containing the corner
    if(xx==x && yy==y)			return __EdgeNeighbor(node->parent,depth-1,2,x,y);
    if(xx==x && zz==z)			return __EdgeNeighbor(node->parent,depth-1,1,x,z);
    if(yy==y && zz==z)			return __EdgeNeighbor(node->parent,depth-1,0,y,z);

    // Check if the node is on a face of the parent containing the edge
    if(xx==x)					return __FaceNeighbor(node->parent,depth-1,0,x);
    if(yy==y)					return __FaceNeighbor(node->parent,depth-1,1,y);
    if(zz==z)					return __FaceNeighbor(node->parent,depth-1,2,z);

    fprintf(stderr,"We shouldn't be here: Corners\n");
    return NULL;
}

template<class NodeData,class Real>
void
IsoNeighborKey<NodeData,Real>::__GetCornerNeighbors(OctNode<NodeData,Real>* node,const int& d,const int& c,OctNode<NodeData,Real>* neighbors[Cube::CORNERS])
{
    int x,y,z,xx,yy,zz,ax,ay,az;
    Cube::FactorCornerIndex(c,x,y,z);
    ax=x^1;
    ay=y^1;
    az=z^1;
    xx=x<<0;
    yy=y<<1;
    zz=z<<2;
    ax<<=0;
    ay<<=1;
    az<<=2;

    // Set the current node
    neighbors[ax|ay|az]=node;

    // Set the face adjacent neighbors
    neighbors[xx|ay|az]=__FaceNeighbor(node,d,0,x);
    neighbors[ax|yy|az]=__FaceNeighbor(node,d,1,y);
    neighbors[ax|ay|zz]=__FaceNeighbor(node,d,2,z);

    // Set the edge adjacent neighbors
    neighbors[ax|yy|zz]=__EdgeNeighbor(node,d,0,y,z);
    neighbors[xx|ay|zz]=__EdgeNeighbor(node,d,1,x,z);
    neighbors[xx|yy|az]=__EdgeNeighbor(node,d,2,x,y);

    // Set the corner adjacent neighbor
    neighbors[xx|yy|zz]=__CornerNeighbor(node,d,x,y,z);
}

template<class NodeData,class Real>
void
IsoNeighborKey<NodeData,Real>::GetCornerNeighbors(OctNode<NodeData,Real>* node,const int& c,OctNode<NodeData,Real>* neighbors[Cube::CORNERS])
{
    getNeighbors(node);
    memset(neighbors,NULL,sizeof(OctNode<NodeData,Real>*)*Cube::CORNERS);
    __GetCornerNeighbors(node,NeighborKey<NodeData,Real>::depth,c,neighbors);
}

template<class NodeData,class Real>
void
IsoNeighborKey<NodeData,Real>::CornerIndex(const int& c,int idx[3])
{
    int x,y,z;
    Cube::FactorCornerIndex(c,x,y,z);
    idx[0]=x<<1;
    idx[1]=y<<1;
    idx[2]=z<<1;
}

template<class NodeData,class Real>
void
IsoNeighborKey<NodeData,Real>::EdgeIndex(const int& e,int idx[3])
{
    int o,i1,i2;
    Cube::FactorEdgeIndex(e,o,i1,i2);
    idx[0]=idx[1]=idx[2]=1;
    switch(o)
    {
    case 0:
        idx[1]=i1<<1;
        idx[2]=i2<<1;
        break;
    case 1:
        idx[0]=i1<<1;
        idx[2]=i2<<1;
        break;
    case 2:
        idx[0]=i1<<1;
        idx[1]=i2<<1;
        break;
    }
}

template<class NodeData,class Real>
void
IsoNeighborKey<NodeData,Real>::FaceIndex(const int& f,int idx[3])
{
    int dir,off;
    Cube::FactorFaceIndex(f,dir,off);
    idx[0]=idx[1]=idx[2]=1;
    idx[dir]=off<<1;
}

#endif // NEIGHBORKEY_INL
