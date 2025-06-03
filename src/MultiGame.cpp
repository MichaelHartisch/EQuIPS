#include "MultiGame.h"
#include <cassert>


MultiGame::MultiGame(const std::vector <Quantifier>& q_vec): ExpansionTracker(0){
    // For creating the very first MultiGame which is input to the first RAReQIS call
    this->Q=q_vec[0];
    Move empty_game((this->Q==EXISTENTIAL)?UNIVERSAL:EXISTENTIAL);
    this->Games.push_back(empty_game);
    this->NumGames=1;
    BlocksInMove.push_back(0);
    this->NumBlocks=q_vec.size();
}

MultiGame::MultiGame(const MultiGame& g): ExpansionTracker(g.ExpansionTracker) {
    //For creating the (empty) abstraction of MultiGame g

    this->Q=g.Q;
    this->EarlierVars.assign(g.EarlierVars.begin(), g.EarlierVars.end());
    this->NumGames=0; 
    this->NumBlocks=g.NumBlocks;
    this->BlocksInMove.assign(g.BlocksInMove.begin(), g.BlocksInMove.end());
    this->ExpansionTracker=g.ExpansionTracker;
}

MultiGame::MultiGame(const MultiGame& game, int ell, const Move& tao): ExpansionTracker(tao.ExtractLastBlock()+1){
    //For creating the MultiGame in which we assigned Move Tao (\Phi_\ell(\tao))
// Create subgame after performing Move m to check, whether there is a countermove to tao

    // In this case we dive into a specific part of the expansion tree
    // We can actually omit all variables that belong to other parts of the expansion tree
    // We now assign all variables that are part of Move m; that belong to player P
    // We also assign all variables that are part of the scenarion/the subgame, belonging to player \bar{P}

    if(game.Q==EXISTENTIAL) this->Q=UNIVERSAL;
    else this->Q=EXISTENTIAL;
    this->NumBlocks=game.NumBlocks;
    assert(tao.ExtractLastBlock()==game.BlocksInMove.back());
    this->BlocksInMove.push_back(tao.ExtractLastBlock()+1);


    //Copy all assigned variables
    this->EarlierVars.assign(game.EarlierVars.begin(), game.EarlierVars.end());
    // Now add the ones from the Move as well as the SubGame we are in
    int Counter=0;
    // We do this in the correct order, i.e. assignments appearing in EarlierVars adheres to the block original block order
    assert(tao.Blocks.size() == game.Games[ell].Assignments.size()+1);

    deque < int > TrackerPath;
    if(game.Games[ell].IsNULL) TrackerPath.push_back(0);
    else{
        assert(game.Games[ell].Assignments.back().size()==1);
        TrackerPath = game.ExpansionTracker.RetrieveCurrentPath(game.Games[ell].Assignments.back()[0].LinkerNode);
    }
    assert(TrackerPath.size() <= tao.Blocks.size());

    if (PRINT) cerr << "Print TrackerPath:";
    PrintDeque(TrackerPath, PRINT);
    deque < int > MovePos;
    if(game.Games[ell].IsNULL) MovePos.push_back(0);
    else
        MovePos = game.ExpansionTracker.RetrieveMovePositions(game.Games[ell].Assignments.back()[0].LinkerNode);
    if (PRINT) cerr << "MovePos: ";
    PrintDeque(MovePos, PRINT);

    while(Counter < TrackerPath.size()){
        assert(tao.Assignments[Counter].size()>MovePos[Counter]);
        for (int i=0;i<tao.Assignments[Counter][MovePos[Counter]].SubGame.size();i++)
                this->EarlierVars.push_back(tao.Assignments[Counter][MovePos[Counter]].SubGame[i]);

        if (Counter < tao.Blocks.size()-1){
            assert(game.Games[ell].Assignments[Counter].size()==1);
            for (int i=0;i<game.Games[ell].Assignments[Counter][0].SubGame.size();i++)
                this->EarlierVars.push_back(game.Games[ell].Assignments[Counter][0].SubGame[i]);
        }
        Counter++;
    }

    Move empty_game(this->Q==UNIVERSAL?EXISTENTIAL:UNIVERSAL);
    this->Games.push_back(empty_game);
    this->NumGames=1;
    this->ExpansionTracker.SetPathAbove(TrackerPath);
}

Move MultiGame::AllLowerBound(const std::vector < std::vector <VariableInfo> >& block_info){
    Move LBAssignment(this);
    for (int b=0;b< this->BlocksInMove.size();b++){
        int TheBlock=this->BlocksInMove[b];
        PartialAssignment AllLB(TheBlock);
        assert(LBAssignment.Assignments[b].size()==0);
        for (int i=0;i<block_info[TheBlock].size();i++){
            if (PRINT) cerr <<"Add x_"<< block_info[TheBlock][i].Idx<<"="<<block_info[TheBlock][i].LB << endl;
            AllLB.Add(block_info[TheBlock][i].Idx,block_info[TheBlock][i].LB);
            // Iterating over the nodes of the expansion tree should be fine here
            // This is function is called for the empty abstraction
            // The empty abstraction also obtains the expansion tree of the underlying instance
            // Hence the expansion tree contains the exact information of the aux variables
        }
        for (int t=0; t<this->ExpansionTracker.NodesByBlock[b].size();t++){
            if (PRINT) cerr <<"For Node "<< t << endl;
            LBAssignment.Add(AllLB, b);
            LBAssignment.Assignments[b].back().SetLinker(this->ExpansionTracker.NodesByBlock[b][t]);
        }
    }
    return LBAssignment;
}

PartialAssignment MultiGame::AllLowerBound(const std::vector < std::vector <VariableInfo> >& block_info, int block){
    PartialAssignment AllLB(block);
    for (int i=0;i<block_info[block].size();i++){
        if (PRINT) cerr <<"Add x_"<< block_info[block][i].Idx<<"="<<block_info[block][i].LB << endl;
        AllLB.Add(block_info[block][i].Idx,block_info[block][i].LB);
    }
    return AllLB;
}

void MultiGame::Refine(Move phi_ell, Move mu){
    if (PRINT) cerr <<"Now: Refine!"  << endl;
        Move NewGame= phi_ell.Copy();
        int Link;
        if(phi_ell.Assignments.size()==0){
            if (PRINT) cerr  <<"In1"  << endl;
            Link = 0; 
        } 
        else{
            if (PRINT) cerr  <<"In2"  << endl;
            Link = phi_ell.Assignments.back().back().LinkerNode;
        }
        int MyLink=ExpansionTracker.Expand(Link, Games.size());
        NewGame.Add(mu,MyLink);

        assert(NewGame.MaxBlock==mu.MaxBlock);
        assert(NewGame.MaxBlock==mu.MinBlock);
        if(ExpansionTracker.MaxDepth>=BlocksInMove.size() && NewGame.MaxBlock+1<NumBlocks){
            assert(BlocksInMove.back()==NewGame.MaxBlock-1);
            BlocksInMove.push_back(NewGame.MaxBlock+1);
        }
        else{
            assert(ExpansionTracker.MaxDepth>=NewGame.Blocks.size());
        }
        
    Games.push_back(NewGame);
    NumGames++;
    return;
}

bool MultiGame::AllGamesQuantifierFree(const std::vector <Quantifier>& q){

    if (PRINT) cerr << q.size()<< " MinBlock: " << BlocksInMove[0]<< "; MaxBlock: " << BlocksInMove.back()<< " this->NumBlocks: "<<  this->NumBlocks<< endl;
    int RemainingOpponentBlocks=ceil((q.size()-(BlocksInMove[0]+1))/(double)2);
    if (PRINT) cerr << "RemainingOpponentBlocks="<<RemainingOpponentBlocks<<endl;
    for (int g=0;g<Games.size();g++){
        if (Games[g].IsNULL){
            if (PRINT) cerr << "Contains empty game"<<endl;
            if (this->BlocksInMove[0]<this->NumBlocks-1){
                if (PRINT) cerr << "Hence quantifier remaining"<<endl;
                return false;
            }
            else if (PRINT) cerr << "But we are in final block; so it is ok"<<endl;
        } 
        else if(Games[g].MaxBlock < this->NumBlocks-2){
                if (PRINT) cerr << "Quantifier remaining"<<endl;
            return false;
        }
    }
    if (PRINT) cerr << "All Games are quantifier-free "<< RemainingOpponentBlocks<<endl;
    return true;
}

void MultiGame::Print(){
    if (!PRINT) return;
    cerr << "=====================" <<endl;
    cerr << "Print Multigame:"<<endl;
    cerr << "Quantifier = "<< (Q==EXISTENTIAL?"EXIST":"UNIV") << endl;
    cerr << "Blocks in Move = { ";
    for (int i=0; i<BlocksInMove.size();i++){
        if (i>0) cerr << " , ";
        cerr << BlocksInMove[i];
    }
    cerr << " }/" << NumBlocks <<endl;

    cerr << "NumGames = "<<NumGames <<endl;
    cerr << "---------------------" <<endl;
    cerr << "Print EarlierVars:"<<endl;
    for (int i=0; i<EarlierVars.size();i++)
        EarlierVars[i].Print();
    cerr << "---------------------" <<endl;
    cerr << "Print Games:"<<endl;
    for (int i=0; i<Games.size();i++)
        Games[i].Print(ExpansionTracker,false);
    cerr << "---------------------" <<endl;
    cerr << "ExpansionTracker:"<<endl;
    ExpansionTracker.Print();
    cerr << "=====================" <<endl;

}