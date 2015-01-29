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

////////////
// Square //
////////////
int Square::CornerIndex(const int& x,const int& y){return (y<<1)|x;}
void Square::FactorCornerIndex(const int& idx,int& x,int& y)
{
    x=(idx>>0)%2;
    y=(idx>>1)%2;
}

void Square::FactorEdgeIndex(const int& idx, int& orientation, int& i)
{
    switch(idx){
        case 0: case 2:
            orientation=0;
            i=idx/2;
            return;
        case 1: case 3:
            orientation=1;
            i=((idx/2)+1)%2;
            return;
        default:
            orientation = 0;
            i = 0;
            return;
    }
}

void Square::EdgeCorners(const int& idx,int& c1,int& c2)
{
    int orientation = 0,i = 0;
    FactorEdgeIndex(idx,orientation,i);
    switch(orientation){
        case 0:
            c1=CornerIndex(0,i);
            c2=CornerIndex(1,i);
            break;
        case 1:
            c1=CornerIndex(i,0);
            c2=CornerIndex(i,1);
            break;
    }
}

void Square::OrientedEdgeCorners(const int& idx,int& c1,int& c2)
{
    int orientation = 0,i = 0;
    FactorEdgeIndex(idx,orientation,i);
    switch(orientation){
        case 0:
            c1=CornerIndex((i  )&1,i);
            c2=CornerIndex((i+1)&1,i);
            break;
        case 1:
            c1=CornerIndex(i,(i+1)&1);
            c2=CornerIndex(i,(i  )&1);
            break;
    }
}

//////////
// Cube //
//////////
int Cube::CornerIndex(const int& x,const int& y,const int& z){return (z<<2)|(y<<1)|x;}
void Cube::FactorCornerIndex(const int& idx,int& x,int& y,int& z)
{
    x=(idx>>0)%2;
    y=(idx>>1)%2;
    z=(idx>>2)%2;
}

int
Cube::EdgeIndex (const int& orientation,const int& i,const int& j)
{
    return (i | (j<<1))|(orientation<<2);
}

void Cube::FactorEdgeIndex(const int& idx,int& orientation,int& i,int &j)
{
    orientation=idx>>2;
    i=idx&1;
    j=(idx&2)>>1;
}

int Cube::FaceIndex(const int& x,const int& y,const int& z)
{
    if		(x<0)	{return  0;}
    else if	(x>0)	{return  1;}
    else if	(y<0)	{return  2;}
    else if	(y>0)	{return  3;}
    else if	(z<0)	{return  4;}
    else if	(z>0)	{return  5;}
    else			{return -1;}
}

int Cube::FaceIndex(const int& dir,const int& offSet)
{
    return (dir<<1)|offSet;
}

void Cube::FactorFaceIndex(const int& idx,int& dir,int& offSet)
{
    dir  = idx>>1;
    offSet=idx &1;
}

int Cube::FaceAdjacentToEdges(const int& eIndex1,const int& eIndex2)
{
    int f1,f2,g1,g2;
    FacesAdjacentToEdge(eIndex1,f1,f2);
    FacesAdjacentToEdge(eIndex2,g1,g2);
    if(f1==g1 || f1==g2){return f1;}
    if(f2==g1 || f2==g2){return f2;}
    return -1;
}

void Cube::FacesAdjacentToEdge(const int& eIndex,int& f1Index,int& f2Index)
{
    int orientation,i1,i2;
    FactorEdgeIndex(eIndex,orientation,i1,i2);
    i1<<=1;
    i2<<=1;
    i1--;
    i2--;
    switch(orientation){
        case 0:
            f1Index=FaceIndex( 0,i1, 0);
            f2Index=FaceIndex( 0, 0,i2);
            break;
        case 1:
            f1Index=FaceIndex(i1, 0, 0);
            f2Index=FaceIndex( 0, 0,i2);
            break;
        case 2:
            f1Index=FaceIndex(i1, 0, 0);
            f2Index=FaceIndex( 0,i2, 0);
            break;
    }
}

void Cube::EdgeCorners(const int& idx,int& c1,int& c2)
{
    int orientation,i1,i2;
    FactorEdgeIndex(idx,orientation,i1,i2);
    switch(orientation){
        case 0:
            c1=CornerIndex(0,i1,i2);
            c2=CornerIndex(1,i1,i2);
            break;
        case 1:
            c1=CornerIndex(i1,0,i2);
            c2=CornerIndex(i1,1,i2);
            break;
        case 2:
            c1=CornerIndex(i1,i2,0);
            c2=CornerIndex(i1,i2,1);
            break;
    }
}

void Cube::FaceCorners(const int& idx,int& c1,int& c2,int& c3,int& c4)
{
    int i=idx%2;
    switch(idx/2){
    case 0:
        c1=CornerIndex(i,0,0);
        c2=CornerIndex(i,1,0);
        c3=CornerIndex(i,0,1);
        c4=CornerIndex(i,1,1);
        return;
    case 1:
        c1=CornerIndex(0,i,0);
        c2=CornerIndex(1,i,0);
        c3=CornerIndex(0,i,1);
        c4=CornerIndex(1,i,1);
        return;
    case 2:
        c1=CornerIndex(0,0,i);
        c2=CornerIndex(1,0,i);
        c3=CornerIndex(0,1,i);
        c4=CornerIndex(1,1,i);
        return;
    }
}

int Cube::FaceReflectEdgeIndex(const int& idx,const int& faceIndex)
{
    int orientation=faceIndex/2;
    int o,i,j;
    FactorEdgeIndex(idx,o,i,j);
    if(o==orientation){return idx;}
    switch(orientation){
        case 0:	return EdgeIndex(o,(i+1)%2,j);
        case 1:
            switch(o){
                case 0:	return EdgeIndex(o,(i+1)%2,j);
                case 2:	return EdgeIndex(o,i,(j+1)%2);
            };
        case 2:	return EdgeIndex(o,i,(j+1)%2);
    }
    return -1;
}

int	Cube::EdgeReflectEdgeIndex(const int& edgeIndex)
{
    int o,i1,i2;
    FactorEdgeIndex(edgeIndex,o,i1,i2);
    return Cube::EdgeIndex(o,(i1+1)%2,(i2+1)%2);
}

int Cube::SquareToCubeCorner(const int& fIndex,const int& cIndex)
{
    // Assuming that the offset is 0, this returns corners in a consistent orientation
    int dir,off,i1,i2;
    FactorFaceIndex(fIndex,dir,off);
    Square::FactorCornerIndex(cIndex,i1,i2);
    switch(dir)
    {
    case 0:
        return CornerIndex(off,i1,i2);
    case 1:
        return CornerIndex(i1,off,(i2+1)&1);
    case 2:
        return CornerIndex(i1,i2,off);
    }
    return -1;
}

int Cube::SquareToCubeEdge(const int& fIndex,const int& eIndex)
{
    // Assuming that the offset is 0, this returns corners in a consistent orientation
    int dir,off,o,i;
    FactorFaceIndex(fIndex,dir,off);
    Square::FactorEdgeIndex(eIndex,o,i);
    switch(dir)
    {
    case 0:
        if(o==0)
            return EdgeIndex(1,off,i);
        else if(o==1)
            return EdgeIndex(2,off,i);
        else
            return -1;
    case 1:
        if(o==0)
            return EdgeIndex(0,off,(i+1)&1);
        else if(o==1)
            return EdgeIndex(2,i,off);
        else
            return -1;
    case 2:
        if(o==0)
            return EdgeIndex(0,i,off);
        else if(o==1)
            return EdgeIndex(1,i,off);
        else
            return -1;
    }
    return -1;
}

/////////////////////
// MarchingSquares //
/////////////////////

MarchingSquares::FaceEdges MarchingSquares::__caseTable[1<<(Square::CORNERS)];

void MarchingSquares::SetCaseTable(void)
{
    for(int idx=0; idx < (1<<Square::CORNERS); idx++)
    {
        int c1,c2;
        __caseTable[idx].count=0;
        for(int i=0;i<Square::EDGES;i++)
        {
            Square::OrientedEdgeCorners(i,c1,c2);
            if( !(idx&(1<<c1)) && (idx&(1<<c2)) )
                __caseTable[idx].edge[__caseTable[idx].count++].first=i;
        }
        __caseTable[idx].count=0;
        for(int i=0;i<Square::EDGES;i++)
        {
            Square::OrientedEdgeCorners(i,c1,c2);
            if( (idx&(1<<c1)) && !(idx&(1<<c2)) )
                __caseTable[idx].edge[__caseTable[idx].count++].second=i;
        }
    }
}

const MarchingSquares::FaceEdges& MarchingSquares::caseTable(const int& idx)
{
    return __caseTable[idx];
}

///////////////////
// MarchingCubes //
///////////////////
template<class Real>
int MarchingCubes::GetIndex(const Real v[Cube::CORNERS],const Real& iso)
{
    int idx=0;
    for(int i=0;i<Cube::CORNERS;i++)
        if(v[i]<iso)
            idx|=1<<i;
    return idx;
}

int MarchingCubes::HasRoots(const int& mcIndex)
{
    if(mcIndex==0 || mcIndex==255){return 0;}
    else{return 1;}
}

int MarchingCubes::HasEdgeRoots(const int& mcIndex,const int& edgeIndex)
{
    int c1,c2;
    Cube::EdgeCorners(edgeIndex,c1,c2);
    if( ( (mcIndex&(1<<c1)) && !(mcIndex&(1<<c2)) ) || ( !(mcIndex&(1<<c1)) && (mcIndex&(1<<c2)) ) )
        return 1;
    else
        return 0;
}

void MarchingCubes::GetEdgeLoops(std::vector<std::pair<int,int> >& edges,std::vector<std::vector<int> >& loops)
{
    int loopSize=0;
    int idx;
    std::pair<int,int> e,temp;
    loops.clear();
    while(edges.size())
    {
        e=edges[0];
        loops.resize(loopSize+1);
        edges[0]=edges[edges.size()-1];
        edges.pop_back();
        loops[loopSize].push_back(e.first);
        idx=e.second;
        for(int j=int(edges.size())-1;j>=0;j--){
            if(edges[j].first==idx || edges[j].second==idx){
                if(edges[j].first==idx)	{temp=edges[j];}
                else					{temp.first=edges[j].second;temp.second=edges[j].first;}
                idx=temp.second;
                loops[loopSize].push_back(temp.first);
                edges[j]=edges[edges.size()-1];
                edges.pop_back();
                j=int(edges.size());
            }
        }
        loopSize++;
    }
}

std::vector< std::vector<int> > MarchingCubes::__caseTable[1<<Cube::CORNERS];

const std::vector< std::vector<int> >& MarchingCubes::caseTable(const int& idx)
{
    return __caseTable[idx];
}

void MarchingCubes::SetCaseTable(void)
{
    MarchingSquares::SetCaseTable();

    /* Iterate the 256 MC cases. */
    for (int idx=0; idx < (1<<Cube::CORNERS); idx++)
    {
        std::vector<std::pair<int,int> > edges;
        /* Iterate all 8 faces of the cube. */
        for (int f=0; f<Cube::FACES; f++)
        {
            /* Compute case index for a face in the MS case table. */
            int dir, off;
            Cube::FactorFaceIndex(f,dir,off);
            int fIdx = 0;
            for (int fc=0; fc < Square::CORNERS; fc++)
                if (idx & (1<<Cube::SquareToCubeCorner(f,fc)))
                    fIdx |= 1<<fc;

            for(int i=0;i<MarchingSquares::caseTable(fIdx).count;i++)
                if(off)
                    edges.push_back(std::pair<int,int>(
                        Cube::SquareToCubeEdge(f,MarchingSquares::caseTable(fIdx).edge[i].first),
                        Cube::SquareToCubeEdge(f,MarchingSquares::caseTable(fIdx).edge[i].second)));
                else
                    edges.push_back(std::pair<int,int>(
                        Cube::SquareToCubeEdge(f,MarchingSquares::caseTable(fIdx).edge[i].second),
                        Cube::SquareToCubeEdge(f,MarchingSquares::caseTable(fIdx).edge[i].first)));
        }
        __caseTable[idx].clear();
        GetEdgeLoops(edges,__caseTable[idx]);
    }
}
