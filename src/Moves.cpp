#include <Moves.h>

Assignment::Assignment(){
    idx=-1;
    val=-1;
}

Assignment::Assignment(int index, int value){
    idx=index;
    val=value;
}
void Assignment::Print(bool print_anyway){
    if (!PRINT && !print_anyway) return;
    cerr << "x_"<<idx <<"="<<val<<endl;
}

PartialAssignment::PartialAssignment(int b){
    this->Block=b;
    this->IsInOrg=false;
}

PartialAssignment::PartialAssignment(){
    this->IsInOrg=false;
}

void PartialAssignment::SetLinker(int & et_node){
    LinkerNode=et_node;
}

void PartialAssignment::Add(const int & idx, const int &val){
    SubGame.push_back(Assignment(idx,val));
}


void PartialAssignment::Print(ExpansionTree & et, bool print_anyway){
    if (!PRINT && !print_anyway) return;
    for (int i=0; i< SubGame.size();i++)
        SubGame[i].Print(print_anyway);
        if(PRINT) cerr << "IsInOrg="<<IsInOrg<<endl;
        if(PRINT) cerr << "LinkerPath: ";
        if(LinkerNode >= et.Nodes.size())
        {
            if(PRINT)cerr << "ERROR: No such node; LinkerNode=" << LinkerNode << " but Nodes.size()=" << et.Nodes.size() << endl;
            return;
        }
        Node TheNode=et.Nodes[LinkerNode];

        deque < int > Path = et.RetrieveCurrentPath(LinkerNode);

    if(PRINT){
        for (int i=0; i<Path.size();i++)
            cerr << Path[i] << "_";
        cerr <<endl;
    }
}


Move::Move(Quantifier q){
    this->Q=q;
    this->MinBlock=-1;
    this->MaxBlock=-1;
    this->IsNULL=true;
}

Move::Move(MultiGame * g){
        
    for (int i=0;i<g->BlocksInMove.size();i++){
            vector < PartialAssignment > NewVec;
            Assignments.push_back(NewVec);
            Blocks.push_back(g->BlocksInMove[i]);
    }
    assert(Blocks.back() ==g->BlocksInMove.back());
    this->Q=g->Q;
    MinBlock=g->BlocksInMove[0];
    MaxBlock=g->BlocksInMove.back();
    IsNULL=true;
}

Move::Move(Quantifier q, int b){
    this->Q=q;
    this->MinBlock=b;
    this->MaxBlock=b;
    Blocks.push_back(b);
    this->IsNULL=true;
}

Move::Move(Quantifier &  q, int b_min,int b_max){
    this->Q=q;
    this->MinBlock=b_min;
    this->MaxBlock=b_max;
    for(int i=b_min; i<=b_max;i=i+2)
        Blocks.push_back(i);
    assert((b_max-b_min)%2 ==0);
    this->IsNULL=true;
}

Move::Move(Move* m){
    this->Q=m->Q;
    this->Assignments = m->Assignments;
    this->MinBlock=m->MinBlock;
    this->MaxBlock=m->MaxBlock;
    for (int i=0;i<m->Blocks.size(); i++)
        this->Blocks.push_back(m->Blocks[i]);
    this->IsNULL=m->IsNULL;
}

void Move::Add(const Move& mu, int link){
    assert(mu.MinBlock==mu.MaxBlock);
    assert(this->Q==mu.Q);
    assert(this->Blocks.size()==0 || this->Blocks.back()==mu.MinBlock-2);
    assert(mu.Assignments[0].size()==1);
    assert(this->Blocks.size()==0 || mu.Assignments[0][0].Block==Blocks.back()+2);
    assert(this->Blocks.size()==Assignments.size());

    if(Assignments.size()==0){
        Assignments.push_back(mu.Assignments[0]);
        assert(mu.MinBlock==this->MinBlock || this->MinBlock==-1);
        Assignments.back().back().LinkerNode=link;
        this->Blocks.push_back(mu.MinBlock);
        MaxBlock=mu.MinBlock;
    }
    else if (this->MaxBlock<mu.MinBlock){
        Assignments.push_back(mu.Assignments[0]);
        Assignments.back().back().LinkerNode=link;
        assert(mu.MinBlock==this->MaxBlock+2);
        this->Blocks.push_back(mu.MinBlock);
        MaxBlock=mu.MinBlock;
    }
    else{
        int findBlock=0;
        while (this->Blocks[findBlock]!=mu.MinBlock) findBlock++; 
        Assignments[findBlock].push_back(mu.Assignments[0][0]);
        Assignments[findBlock].back().LinkerNode=link;
        assert(MaxBlock==mu.MinBlock);
    }
    this->IsNULL=false;
    
    if(MinBlock==-1) MinBlock=MaxBlock;
    
}

Move Move::Copy(){
    Move CopiedMove(this);
    return CopiedMove;
}



int Move::ExtractLastBlock() const{
    assert(Assignments.back()[0].Block==this->Blocks.back());
    return Assignments.back()[0].Block;
}

int Move::ExtractLinkerNode(){
    assert(Assignments.back()[0].Block==this->Blocks.back());
    assert(Assignments.back().size()==1);
    return Assignments.back()[0].LinkerNode;
}

void Move::Transform(ExpansionTree & tree){
    for (int ell=0; ell < tree.Leaves.size();ell++){
        deque < int > Path= tree.RetrieveCurrentPath(tree.Leaves[ell]);
        if (PRINT) cerr << "Path: ";
        PrintDeque(Path, PRINT);
        deque < int > MovePos;

        MovePos = tree.RetrieveMovePositions(tree.Leaves[ell]);
        if (PRINT) cerr << "MovePos: ";
        PrintDeque(MovePos, PRINT);

        //For every subgame Phi_i we need to figure out what auxiliary variables are needed and keep them in 
        for (int b=0; b < Path.size();b++){
            this->Assignments[b][MovePos[b]].IsInOrg=true;
        }   
    }

    for (int i=Assignments.size()-1; i >= 0;i--){
        bool Removing=true;
        for (int j=Assignments[i].size()-1; j >= 0 ;j--){ 
            //Start removing from the back, as newly added auxiliaries should be in the back
            if(!Removing) assert (Assignments[i][j].IsInOrg);
            if (this->Assignments[i][j].IsInOrg){
                Removing = false;
                this->Assignments[i][j].IsInOrg=false; //For the next time this move has to be transformed
            }
            else if(!this->Assignments[i][j].IsInOrg){
                this->Assignments[i].pop_back();
                assert(Assignments[i].size()==j);
            }
            if (j==0 && Removing){
                MaxBlock=MaxBlock-2;
                Blocks.pop_back();
                assert(Assignments[i].size()==0);
                assert(i==Assignments.size()-1);
                Assignments.pop_back();
            }
        }
            // Assignments[j] is the assignment of variables of the j-th block of interest
    }
}


void Move::Add(PartialAssignment pa, const int& block){
    Assignments[block].push_back(pa);   
    if (IsNULL) IsNULL=false;
}

void Move::Print(ExpansionTree & et, bool print_anyway){
    if (!PRINT && !print_anyway) return;
    assert(MinBlock>= 0 || IsNULL );
    if(IsNULL){
        if(PRINT) cerr << "MOVE IS NULL"<<endl;
        return;
    }
    if(PRINT) cerr << "MinBlock = " << MinBlock << endl;
    if(PRINT) cerr << "MaxBlock = " << MaxBlock << endl;
    for (int i=0; i< Assignments.size();i++){
        cerr << "Assignments of Block " << Blocks[i]  << endl;
        for (int j=0; j< Assignments[i].size();j++){
            if(PRINT) cerr << i <<","<<j<<" LinkerNode=" << Assignments[i][j].LinkerNode  << endl;
            assert (Assignments[i][j].Block == Blocks[i]);
            Assignments[i][j].Print(et, true);
        }
    }
    if(PRINT){
        cerr <<"Blocks: { ";
        for (int i=0;i<Blocks.size();i++){
            cerr << this->Blocks[i]; 
            if (i<Blocks.size()-1) cerr<< " , ";
        }
        cerr << " }"<<endl;
    }
    assert(Blocks.size()==Assignments.size());
}