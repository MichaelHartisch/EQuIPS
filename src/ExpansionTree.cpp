#include "ExpansionTree.h"
static bool PRINT=false;

//==================Node==================
Node::Node(int block){
    //Only use this to create the root!
    Depth=0;
    this->Parent=-1;
    this->Idx=0;
    this->IsLeaf=true;
    this->IsRoot= true;
    this->PointerToGame=0;
    this->Block=block;
    this->PosInMove=0;
    this->PosInNodes=0;
    this->PosInLeaves=0;
}

void Node::SetPointerToGame(int & p){
    PointerToGame=p;
}
void Node::Print(bool print_anyway){
    if (!PRINT && !print_anyway) return;
    cerr << "Node ("<<PosInNodes <<" " <<Idx;
    if(IsRoot) cerr << ",R";
    cerr << ") ";
    if(IsLeaf) cerr << " is leaf; ";
    cerr << "has " << Successors.size() << " successors and position in Move is " << PosInMove <<endl;
    cerr << "Has parent "<< Parent << endl; 
    if (PointerToGame!=-1) cerr << " It corresponds to game " << PointerToGame <<endl;
}

Node::Node(int & parent, int pos, int p2g, std::vector <Node> & nodes){
    this->Parent= nodes[parent].PosInNodes;
    this->Depth=nodes[parent].Depth+1;
    this->Idx=nodes[parent].Successors.size();
    this->IsLeaf=true;
    this->IsRoot= false;
    this->PointerToGame=p2g;
    this->Block=nodes[parent].Block+2;
    this->PosInMove=pos;
    this->PosInNodes=nodes.size();
}

//==================ExpansionTree==================


ExpansionTree::ExpansionTree(int block){
    this->Nodes.push_back(Node(block));
    this->Leaves.push_back(0);
    this->NodesByBlock.push_back(Leaves);
    this->MaxDepth=0;
    this->NumPosPerDepth.push_back(1);
}

ExpansionTree::ExpansionTree(const ExpansionTree& et){
    this->Nodes=et.Nodes;
    this->Leaves=et.Leaves;
    this->NodesByBlock=et.NodesByBlock;
    this->MaxDepth=et.MaxDepth;
    this->NumPosPerDepth=et.NumPosPerDepth;
    this->PathAbove=et.PathAbove;
}

void ExpansionTree::SetPathAbove(deque <int> tracker) {
    PathAbove=tracker;
}

int ExpansionTree::Expand(int & n, int p){
    if(NumPosPerDepth.size() <= Nodes[n].Depth+1){
        assert(NumPosPerDepth.size() == Nodes[n].Depth+1);
        NumPosPerDepth.push_back(0);
        std::vector <int> NewVec;
        NodesByBlock.push_back(NewVec);
        assert(NodesByBlock.size()-1 == Nodes[n].Depth+1);
    }
    Nodes.push_back(Node(n, NumPosPerDepth[Nodes[n].Depth+1],p,Nodes));
    NumPosPerDepth[Nodes[n].Depth+1]++;
    assert(Nodes[n].Depth+1==Nodes.back().Depth);
    assert(Nodes.size()==Nodes.back().PosInNodes+1);

    const Node* NewNode= &Nodes.back();
    Nodes[n].Successors.push_back(Nodes.back().PosInNodes);
    if(Nodes[n].IsLeaf){
        Nodes[n].IsLeaf=false;
        Nodes.back().PosInLeaves=Nodes[n].PosInLeaves;
        Leaves[Nodes[n].PosInLeaves]=Nodes.back().PosInNodes;
    }
    else{
        Nodes.back().PosInLeaves=Leaves.size();
        Leaves.push_back(Nodes.back().PosInNodes);
    }
    NodesByBlock[Nodes.back().Depth].push_back(Nodes.back().PosInNodes);
    if (this->MaxDepth < NewNode->Depth)
        this->MaxDepth=NewNode->Depth;
    return Nodes.back().PosInNodes;
}

deque<int> ExpansionTree::RetrieveFullPath(const int & n) const{
    deque<int> IdxPath = RetrieveCurrentPath(n);
    for (int i=PathAbove.size()-1; i>=0; i--)
        IdxPath.push_front(PathAbove[i]);
    return IdxPath;
}

void ExpansionTree::ResetConsidered(){
    for (int i=0; i<Nodes.size();i++)
        Nodes[i].Considered=false;
}

void ExpansionTree::SetConsidered(const int & n){
    Node N= Nodes[n];
    //Nodes[n].Considered=true;
    assert(n==N.PosInNodes);
    Nodes[N.PosInNodes].Considered=true;

    if (PRINT) cerr << "n="<<N.PosInNodes << " set N.Considered=" << Nodes[N.PosInNodes].Considered<<endl;
    while (!N.IsRoot){
        N=Nodes[N.Parent];
        Nodes[N.PosInNodes].Considered=true;
        if (PRINT) cerr << "n="<<N.PosInNodes << " set N.Considered=" << Nodes[N.PosInNodes].Considered<<endl;
    }
    return;
}

deque<std::pair < int,bool >  > ExpansionTree::RetrieveCurrentPathNoDoubles(const int & n){
    Node N= Nodes[n];
    deque<std::pair < int,bool > > IdxPath;
    IdxPath.push_front(std::make_pair(N.Idx,N.Considered));
    if (PRINT) cerr << "RetrieveCurrentPathNoDoubles:"<<endl;
    if (PRINT) cerr << "n="<<N.PosInNodes << " N.Considered=" << N.Considered<<endl;
    while (!N.IsRoot){
        N=Nodes[N.Parent];
        IdxPath.push_front(std::make_pair(N.Idx,N.Considered));
        if (PRINT) cerr << "n="<<N.PosInNodes << " N.Considered=" << N.Considered<<endl;
    }
    return IdxPath;
}

deque<int> ExpansionTree::RetrieveCurrentPath(const int & n) const{
    Node N= Nodes[n];
    deque<int> IdxPath;
    IdxPath.push_front(N.Idx);
    while (!N.IsRoot){
        N=Nodes[N.Parent];
        IdxPath.push_front(N.Idx);
    }
    return IdxPath;
}

deque<int> ExpansionTree::RetrieveCurrentPathOfNodes(const int & n) const{
    Node N= Nodes[n];
    deque<int> NodesPath;
    NodesPath.push_front(n);
    while (!N.IsRoot){
        NodesPath.push_front(N.Parent);
        N=Nodes[N.Parent];
    }
    return NodesPath;
}

int ExpansionTree::RetrievePredecessor(const int n, int lev){
    Node N=Nodes[n];
    int c=0;
    while (c<lev){
        N=Nodes[N.Parent];
        c++;
    }
    return N.PosInNodes;
}

deque<int> ExpansionTree::RetrieveMovePositions(const int n) const{
    Node N=Nodes[n];
    deque<int> PosIdx;
    PosIdx.push_front(N.PosInMove);
    while (!N.IsRoot){
        N=Nodes[N.Parent];
        PosIdx.push_front(N.PosInMove);
    }
    return PosIdx;
}

void ExpansionTree::Print(bool printAnyway){
    if (!PRINT && !printAnyway) return;
    for (int d=0;d<NodesByBlock.size();d++){
        cerr <<"Depth "<< d << endl;
        for (int i=0; i<NodesByBlock[d].size();i++){
            assert(d==Nodes[NodesByBlock[d][i]].Depth);
            Nodes[NodesByBlock[d][i]].Print(printAnyway);
            deque<int> ThePath=RetrieveCurrentPath(NodesByBlock[d][i]);
            cerr << "Path: "; 
            for (int i=0;i < ThePath.size(); i++)
                cerr << ThePath[i] << " "; 
            cerr << endl;
        }
    }
}
