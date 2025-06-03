#include "Solver.h"

QISSolver::QISSolver(Parser prs, const chrono::steady_clock::time_point& start, const int& time_limit): Prs(prs), MainEnv(), MainModel(MainEnv, prs.TempFilename), UncertaintyModel(MainModel), NumConstrs(MainModel.get(GRB_IntAttr_NumConstrs)), RHS(NumConstrs+1), Sense(NumConstrs+1), L(NumConstrs+1,0), U(NumConstrs+1,0), r(NumConstrs+1,0)
{
    this->Start=start;
    this->TimeLimit=time_limit;
    Prs.ParseQuantificationInfo(MainModel, Vars, Quantification, QuantificationDelimiter, BlockVars, DictGRBToIdx, DictIdxToGRB);
    Prs.CreateUncertaintyModel(UncertaintyModel);
    MainEnv.set(GRB_IntParam_OutputFlag, 0);
    MainEnv.set(GRB_IntParam_Threads, 1);
    MainModel.set(GRB_IntParam_OutputFlag, 0);
    //MainEnv.set(GRB_IntParam_OutputFlag, 0);
    //MainEnv.set(GRB_IntParam_Threads, 1);
    UncertaintyModel.set(GRB_IntParam_OutputFlag, 0);
    UncertaintyModel.set(GRB_IntParam_Threads, 1);
    NumConstrsUncertainty = UncertaintyModel.get(GRB_IntAttr_NumConstrs);
    UncBoundsManager.storeOriginalBounds(UncertaintyModel); 
    MainBoundsManager.storeOriginalBounds(MainModel); 

    if(PRINT) cerr << "We have "<< NumConstrsUncertainty << " constraints describing the uncertainty set" << endl;
    std::vector<int> BlockDelimiter;
    CurrentBlock=0;
    GRBQuadExpr objective = MainModel.getObjective();

    if(objective.getLinExpr().size() == 0 && objective.size() == 0){
        std::cout << "The model does not have an objective function." << std::endl;
        IsOptimizationProblem=false;
    } else {
        std::cout << "The model has an objective function." << std::endl;
        SetObjectiveBounds(objective.getLinExpr());
        IsOptimizationProblem=true;
        int Sense = MainModel.get(GRB_IntAttr_ModelSense);
        //We always want to have a MINIMIZATION view
        if (Sense == GRB_MAXIMIZE) {
            std::cout << "It is a maximization problem." << std::endl;
            IsMinimizationProblem=false;
            double helper=ObjLowerBound;
            ObjLowerBound=-ObjUpperBound;
            ObjUpperBound=-helper;
            objConstr = MainModel.addConstr(-objective.getLinExpr() <= ObjUpperBound, "ObjectiveConstraint");    
        }
        else{
            std::cout << "It is a minimization problem." << std::endl;
            IsMinimizationProblem=true;
            objConstr = MainModel.addConstr(objective.getLinExpr() <= ObjUpperBound, "ObjectiveConstraint");    

        }  
        NumConstrs=NumConstrs+1; 
        MainModel.update();      
    }
    ExtractParameters();
    if(PRINT) PrintForDebug(); 
    CounterExistIP = 0;
    CounterAllIP = 0;
    ModelCounter=0;
};

void QISSolver::ChangeObjRHS(double new_obj){
    objConstr.set(GRB_DoubleAttr_RHS, new_obj);
    MainModel.update(); 
        RHS.back() = new_obj;

//    UpdateObjParameters(new_obj);
}

void QISSolver::SetObjectiveBounds(GRBLinExpr row){
    ObjLowerBound = 0.0;
    ObjUpperBound = 0.0;
    for (int i = 0; i < row.size(); ++i) {
        GRBVar var = row.getVar(i);
        double coeff = row.getCoeff(i);

        if (coeff > 0) {
            ObjLowerBound += coeff * var.get(GRB_DoubleAttr_LB);
            ObjUpperBound += coeff * var.get(GRB_DoubleAttr_UB); 
        } else {
            ObjLowerBound += coeff * var.get(GRB_DoubleAttr_UB);  
            ObjUpperBound += coeff * var.get(GRB_DoubleAttr_LB); 
        }
    }

    // Print results
    if(PRINT) std::cout << "LHS Lower Bound: " << ObjLowerBound << std::endl;
    if(PRINT) std::cout << "LHS Upper Bound: " << ObjUpperBound << std::endl;
}

void QISSolver::ExtractParameters(){
    if(PRINT) cerr << "NumConstrs=" <<NumConstrs <<endl;
    Constrs = MainModel.getConstrs(); 
    ConstrsUncertainty = UncertaintyModel.getConstrs(); 


    for (int c = 0; c < NumConstrs; c++){
        RHS[c] = Constrs[c].get(GRB_DoubleAttr_RHS);
        Sense[c] = Constrs[c].get(GRB_CharAttr_Sense);
    }

    for (int c=0;c<NumConstrs;c++){
        //Prepare L, U and r parameters needed to fill the model

        std::vector <double> Decimals; 
        Decimals.push_back(RHS[c]-floor(RHS[c]));

        GRBLinExpr Expr = MainModel.getRow(Constrs[c]);

        for (int i = 0; i < Expr.size(); ++i) {
            double val = Expr.getCoeff(i);
            Decimals.push_back(val-floor(val));

            GRBVar OrgVar = Expr.getVar(i);

            if(val<0){
                L[c]+=val*OrgVar.get(GRB_DoubleAttr_UB);
                U[c]+=val*OrgVar.get(GRB_DoubleAttr_LB);
            }
            else{
                L[c]+=val*OrgVar.get(GRB_DoubleAttr_LB);
                U[c]+=val*OrgVar.get(GRB_DoubleAttr_UB);
            }
        }
        std::vector <double> Denominators(Decimals.size()); 
        int MaxPow=0;
        for (int i=0;i<Decimals.size();i++){
            Denominators[i]=ApproximateDenominatorPowerTo10(Decimals[i]);
            if (Denominators[i]>MaxPow)
                MaxPow=Denominators[i];
        }
        r[c]=1/pow(10,MaxPow);

        if(PRINT){
            cerr << "Found r value for constraint " << c << " to be " << r[c] << endl;
            cerr << "Found L value for constraint " << c << " to be " << L[c] << endl;
            cerr << "Found U value for constraint " << c << " to be " << U[c] << endl;
        }
    }
}

void QISSolver::CreateXVariables(GRBModel* model, MultiGame & game){
for (int i=0; i<game.EarlierVars.size();i++){
        // These variables are all fixed in the IP 
        // So create those variables but set their bounds to their assigned values
        // GRBVar addVar(double lb, double ub, double obj, char type, string name = "")
        model->addVar(game.EarlierVars[i].val,game.EarlierVars[i].val,0,GRB_INTEGER,Vars[game.EarlierVars[i].idx].Name);
        if(PRINT) cerr << "1Created variable " << Vars[game.EarlierVars[i].idx].Name << " fixed to "<< game.EarlierVars[i].val <<endl;
        assert(Vars[game.EarlierVars[i].idx].Block<game.BlocksInMove[0]);
    }

    // We now add (in the correct order) variables that belong to the current move of interest, as well as the fixed opponent variables of
    // the abstraction
    // We do this in the correct order, i.e. assignments appearing in EarlierVars adheres to the block original block order
    game.ExpansionTracker.ResetConsidered();
    for (int g=0; g<game.Games.size() ;g++){
        int Counter=0;
        int CurrentBlock=game.BlocksInMove[Counter];
        deque < std::pair < int,bool > > TrackerPath;
        if(game.Games[g].IsNULL) TrackerPath.push_back(std::make_pair(0,false));
        else{ 
            assert(game.Games[g].Assignments.back().size()==1);
            TrackerPath = game.ExpansionTracker.RetrieveCurrentPathNoDoubles(game.Games[g].Assignments.back()[0].LinkerNode);
            game.ExpansionTracker.SetConsidered(game.Games[g].Assignments.back()[0].LinkerNode);
        }
        while(Counter < game.BlocksInMove.size() && CurrentBlock <= game.BlocksInMove.back()){
             if(PRINT) cerr << g <<  " " << Counter <<" The Tracker:" << TrackerPath[Counter].first << " " << TrackerPath[Counter].second << endl;
            if (!( Counter <TrackerPath.size() && TrackerPath.size()>0 && TrackerPath[Counter].second ==true)){
                 if(PRINT) cerr << "Do it"<<endl;
                for (int i=0; i<BlockVars[CurrentBlock].size();i++){
                    string NewName=CopyNoDouble(BlockVars[CurrentBlock][i].Name,TrackerPath, Counter);
                    // All Variables of the relevant blocks are added as in the original model with a respective identifier
                    model->addVar(BlockVars[CurrentBlock][i].LB, BlockVars[CurrentBlock][i].UB,0,GRB_INTEGER,NewName);
                    if(PRINT) cerr << "2Created variable " <<  NewName <<endl;
                }
            }
            CurrentBlock++;
            if (CurrentBlock<game.NumBlocks &&Counter < TrackerPath.size()/* && game.Games[g].Assignments.size()>0*/){
                assert(game.Games[g].Assignments[Counter].size()==1);
                for (int i=0; i<game.Games[g].Assignments[Counter][0].SubGame.size();i++){
                    Assignment CurrentAssignment=game.Games[g].Assignments[Counter][0].SubGame[i];

                    assert(Vars[CurrentAssignment.idx].Block==CurrentBlock);
                    assert(Vars[CurrentAssignment.idx].Block>game.BlocksInMove[0]);

                    // The following variables are the abstraction of the opponents move.
                    // So we create a copy of these opponent variables for every scenario part of the abstraction 
                    // and set their bounds to their respective values
                    string NewName=CopyNoDouble(Vars[CurrentAssignment.idx].Name,TrackerPath, Counter+1);
                    if(PRINT) cerr << "3Created variable " << NewName <<endl;
                    model->addVar(CurrentAssignment.val,CurrentAssignment.val,0,GRB_INTEGER, NewName);
                }
                CurrentBlock++;
            }
            Counter++;
        }
        
        // For all other variable, that have not been assigned in previous blocks and are not part of an abstraction, 
        //we create a copy for every abstraction
        assert(CurrentBlock==game.BlocksInMove.back() || CurrentBlock==game.NumBlocks);
        if(CurrentBlock<game.NumBlocks){
            for (int b=game.BlocksInMove.back()+1; b < BlockVars.size(); b++){
                for (int i=0; i<BlockVars[b].size();i++){
                    string NewName=CopyNoDouble(BlockVars[b][i].Name,TrackerPath, TrackerPath.size());
                    // All Variables of the last blocks are added as in the original model
                    if(PRINT) cerr << "4Created variable " <<  NewName<<endl;

                    model->addVar(BlockVars[b][i].LB, BlockVars[b][i].UB,0,GRB_INTEGER,NewName);
                }
            }
        }
    }
}

void QISSolver::BuildExistentialModel(GRBModel* model, MultiGame & game){
    if(PRINT) cerr << "Write IP from Existential Player View"<<endl;
    for (int c=0;c<NumConstrs;c++){ 
        GRBLinExpr Expr = MainModel.getRow(Constrs[c]);
        if (IsOptimizationProblem && c==NumConstrs-1) assert(Constrs[c].get(GRB_StringAttr_ConstrName)=="ObjectiveConstraint");
        //Prepare all copies of the constraints; we need as many as there are games.
        std::vector<GRBLinExpr> NewExprs;
        for (int g=0; g<game.Games.size() ;g++){
            NewExprs.push_back(GRBLinExpr());
        }

        // Iterate over the terms in the sparse representation.

        for (int i = 0; i < Expr.size(); ++i) {
            GRBVar OrgVar=Expr.getVar(i);
            string OrgName = OrgVar.get(GRB_StringAttr_VarName);    // Get the orignal name
            int OrgIdx = OrgVar.index();                            // Get the GRB name
            int MyOrgIdx = DictGRBToIdx[OrgIdx];                    // Translate to My Index
            
            //now copy the current constraint for every scenario 
            for (int g=0; g<game.Games.size() ;g++){
                deque < int > TrackerPath;
                if(game.Games[g].IsNULL) TrackerPath.push_back(0);
                else{ 
                    assert(game.Games[g].Assignments.back().size()==1);
                    TrackerPath = game.ExpansionTracker.RetrieveCurrentPath(game.Games[g].Assignments.back()[0].LinkerNode);
                }

                
                string NewName;
                if (Vars[MyOrgIdx].Block<game.BlocksInMove[0])
                    NewName=OrgName;
                else{
                    if(game.Games[g].IsNULL)
                        NewName=Copy(OrgName,TrackerPath,0);
                    else
                        NewName=Copy(OrgName,TrackerPath,floor((Vars[MyOrgIdx].Block-game.Games[g].Blocks[0])/(double)2)+1);
                }
                if(PRINT) cerr << "NewName we are looking for is " << NewName <<endl;
                try{
                    NewExprs[g] += Expr.getCoeff(i) * model->getVarByName(NewName);
                }
                catch (GRBException& e) {
                    if (e.getErrorCode() == GRB_ERROR_DATA_NOT_AVAILABLE) {
                        if(PRINT) std::cerr << "Variable " << NewName << " not found in the model." << std::endl;
                    } else {
                        if(PRINT) std::cerr << "Error retrieving variable: "  << NewName  << " :"  << e.getMessage() << std::endl;
                    }
                    assert(0);

                }
                //
            }
        }

        for (int g=0; g<game.Games.size() ;g++){
            if (Sense[c] == GRB_LESS_EQUAL) {
                model->addConstr(NewExprs[g] <= RHS[c]);
            } else if (Sense[c] == GRB_GREATER_EQUAL) {
                model->addConstr(NewExprs[g] >= RHS[c]);
            } else if (Sense[c] == GRB_EQUAL) {
                model->addConstr(NewExprs[g] == RHS[c]);
            }
            else{
                if(PRINT) cerr << "WARNING: Unknown Sense of Constraint"<<endl;
            }
        }
    }
}

void QISSolver::BuildUniversalModel(GRBModel* model, MultiGame & game){
    if(PRINT) cerr << "Write IP from Universal Player View"<<endl;

    GRBVar AuxVarObj = model->addVar(0,1,1,GRB_BINARY,"aux_obj_ind");
    if(PRINT) cerr << "Created aux variable " <<  "aux_obj_ind" <<endl;

    std::vector < GRBVar > AuxVarGameInd;

    // REMINDER:
    // L[c] = the smallest possible value of the left-hand side of constraint c
    // U[c] = the largest possible value of the left-hand side of constraint c
    // U is needed in case of >= or == constraints

    for (int g=0; g<game.Games.size() ;g++){
        std::vector < GRBVar > AuxVarGameConInd;
        string aux_name="aux_game_ind_"+ to_string(g);
        AuxVarGameInd.push_back(model->addVar(0,1,0,GRB_BINARY,aux_name));
        if(PRINT) cerr << "Created aux variable " <<  aux_name <<endl;
        deque < int > TrackerPath;
        if(game.Games[g].IsNULL) TrackerPath.push_back(0);
        else{ 
            assert(game.Games[g].Assignments.back().size()==1);
            TrackerPath = game.ExpansionTracker.RetrieveCurrentPath(game.Games[g].Assignments.back()[0].LinkerNode);
        }

        GRBLinExpr CollectConstraints = 1*AuxVarGameInd.back();
        model->addConstr(AuxVarObj - AuxVarGameInd[g] <= 0);    
        for (int c=0;c<NumConstrs;c++){ 
            string aux_name_c="aux_game_ind_"+ to_string(g)+"_"+to_string(c);
            AuxVarGameConInd.push_back(model->addVar(0,1,0,GRB_BINARY,aux_name_c));
            if(PRINT) cerr << "Created aux variable " <<  aux_name_c <<endl;
            CollectConstraints += (-1)*AuxVarGameConInd.back();
            GRBVar ExtraVarIfEquality;
            if (Sense[c] == GRB_EQUAL) {
                ExtraVarIfEquality = model->addVar(0,1,0,GRB_BINARY,string(aux_name_c+"_extra"));
                CollectConstraints += (-1)*ExtraVarIfEquality;
            }

            GRBLinExpr Expr = MainModel.getRow(Constrs[c]);
            GRBLinExpr NewExpr;

            for (int i = 0; i < Expr.size(); ++i) {
                GRBVar OrgVar=Expr.getVar(i);
                string OrgName = OrgVar.get(GRB_StringAttr_VarName);    // Get the orignal name
                int OrgIdx = OrgVar.index();                            // Get the GRB name
                int MyOrgIdx = DictGRBToIdx[OrgIdx];                    // Translate to My Index
                
                //Now copy this constraint for the current scenario 
                string NewName;
                if (Vars[MyOrgIdx].Block<game.BlocksInMove[0])
                    NewName=OrgName;
                else{
                    if(game.Games[g].IsNULL)
                        NewName=Copy(OrgName,TrackerPath,0);
                    else
                        NewName=Copy(OrgName,TrackerPath,floor((Vars[MyOrgIdx].Block-game.Games[g].Blocks[0])/(double)2)+1);
                }
                if(PRINT) cerr << "NewName we are looking for is " << NewName <<endl;
                try{
                    NewExpr += -Expr.getCoeff(i) * model->getVarByName(NewName);
                }
                catch (GRBException& e) {
                    if (e.getErrorCode() == GRB_ERROR_DATA_NOT_AVAILABLE) {
                            if(PRINT) std::cerr << "Variable " << NewName << " not found in the model." << std::endl;
                    } else {
                            if(PRINT) std::cerr << "Error retrieving variable: " << e.getMessage() << std::endl;
                    }
                    assert(0);
                }
            }
            if (Sense[c] == GRB_LESS_EQUAL) {
                NewExpr += -(L[c]-RHS[c]-r[c])*AuxVarGameConInd[c];
                model->addConstr(NewExpr <= -L[c]);
            } 
            else if (Sense[c] == GRB_GREATER_EQUAL) {
                NewExpr = -NewExpr;
                NewExpr += -(-U[c]+RHS[c]-r[c])*AuxVarGameConInd[c];
                model->addConstr(NewExpr <= U[c]);
            } 
            else if (Sense[c] == GRB_EQUAL) {
                //Need to Add 2 Constraints
                GRBLinExpr NewExpr2=-NewExpr;
                NewExpr += -(L[c]-RHS[c]-r[c])*AuxVarGameConInd[c];
                NewExpr2 += -(-U[c]+RHS[c]-r[c])*ExtraVarIfEquality;
                model->addConstr(NewExpr <= -L[c]);
                model->addConstr(NewExpr2 <= U[c]);
            }
        }
        model->addConstr(CollectConstraints <= 0);

        //Now add constraints restricting the universally quantified variables; the constraints defining the uncertainty set
        if(PRINT) cerr << "Now create constraints of uncertainty set" <<endl;

        for (int c = 0; c < NumConstrsUncertainty; ++c) {

            GRBLinExpr Expr = UncertaintyModel.getRow(ConstrsUncertainty[c]);
            GRBLinExpr NewExpr;

            for (int i = 0; i < Expr.size(); ++i) {
                GRBVar OrgVar=Expr.getVar(i);
                string OrgName = OrgVar.get(GRB_StringAttr_VarName);    // Get the orignal name
                int OrgIdx = OrgVar.index();                            // Get the GRB name
                int MyOrgIdx = DictGRBToIdx[OrgIdx];                    // Translate to My Index
                
                //Now copy this constraint for the current scenario 
                string NewName;
                if (Vars[MyOrgIdx].Block<game.BlocksInMove[0])
                    NewName=OrgName;
                else{
                    if(game.Games[g].IsNULL)
                        NewName=Copy(OrgName,TrackerPath,0);
                    else
                        NewName=Copy(OrgName,TrackerPath,floor((Vars[MyOrgIdx].Block-game.Games[g].Blocks[0])/(double)2)+1);
                }
                if(PRINT) cerr << "NewName we are looking for is " << NewName <<endl;
                try{
                    NewExpr += Expr.getCoeff(i) * model->getVarByName(NewName);
                }
                catch (GRBException& e) {
                    if (e.getErrorCode() == GRB_ERROR_DATA_NOT_AVAILABLE) {
                            if(PRINT) std::cerr << "Variable " << NewName << " not found in the model." << std::endl;
                    } else {
                            if(PRINT) std::cerr << "Error retrieving variable: " << e.getMessage() << std::endl;
                    }
                    assert(0);
                }
            }
            if (ConstrsUncertainty[c].get(GRB_CharAttr_Sense) == GRB_LESS_EQUAL) {
                model->addConstr(NewExpr <= ConstrsUncertainty[c].get(GRB_DoubleAttr_RHS));
            } 
            else if (ConstrsUncertainty[c].get(GRB_CharAttr_Sense) == GRB_GREATER_EQUAL) {
                model->addConstr(NewExpr >= ConstrsUncertainty[c].get(GRB_DoubleAttr_RHS));
            } 
            else if (ConstrsUncertainty[c].get(GRB_CharAttr_Sense) == GRB_EQUAL) {
                //Need to Add 2 Constraints
                model->addConstr(NewExpr == ConstrsUncertainty[c].get(GRB_DoubleAttr_RHS));
            }
        }

    }
    model->setObjective(GRBLinExpr(AuxVarObj), GRB_MAXIMIZE);

}

Move QISSolver::Wins(MultiGame & game){
    if(PRINT) cerr << "Call Wins Function" <<endl;
    GRBEnv* Env = new GRBEnv(true);
    Env->set(GRB_IntParam_OutputFlag, 0);
    Env->set(GRB_IntParam_Threads, 1);
    Env->set(GRB_DoubleParam_TimeLimit, GetRemainingTime());
    Env->set(GRB_IntParam_Cuts, 2);

    Env->start();

    /* Create model */
    GRBModel* Model = new GRBModel(*Env);
    if(PRINT) game.Print();

    // Create Variables that are the same in both cases
    CreateXVariables(Model, game);
    Model->update();

    if(game.Q==EXISTENTIAL){
        BuildExistentialModel(Model, game);
        Model->update();
        CounterExistIP++;
    }
    else{
        BuildUniversalModel(Model, game);
        Model->update();
         CounterAllIP++;
    }
    //Model->write("LP"+std::to_string(ModelCounter)+".lp");
    //ModelCounter++;
    Model->optimize();
    CheckTimeLimit();
    int status = Model->get(GRB_IntAttr_Status);
    if (status == GRB_INFEASIBLE){
        if(PRINT) cerr << "Model is infeasible" << endl;
        assert(game.Q!=UNIVERSAL);
        delete Model;
        delete Env;
        return Move(game.Q);
    }
    else if (game.Q==UNIVERSAL && Model->get(GRB_DoubleAttr_ObjVal)<0.5){
        //In this case the model is always feasible
        //An objective value of 0 however indicates, that the universal player cannot win all subgames, i.e. cannot violate all systems
         if(PRINT) cerr << "Model is feasible but objective value is " << Model->get(GRB_DoubleAttr_ObjVal) << endl;

        delete Model;
        delete Env;
        return Move(game.Q);
    }
    else{
        if(PRINT) cerr << "Model is feasible " << endl;

        if(PRINT) if (game.Q==UNIVERSAL) cerr << "Objective value is " << Model->get(GRB_DoubleAttr_ObjVal) << endl;
        //Only return the Move corresponding to variable of the current block
        Move ReturnMove(game.Q, game.BlocksInMove[0],game.BlocksInMove.back());

        //Create space for all variables of interest in this move
        for (int b=0;b<game.BlocksInMove.size();b++){
            vector<PartialAssignment> NewVec;
            ReturnMove.Assignments.push_back(NewVec);
            for (int i=0; i<game.ExpansionTracker.NumPosPerDepth[b]; i++ )
                ReturnMove.Assignments[b].push_back(PartialAssignment(game.BlocksInMove[b]));
        }
        assert(game.BlocksInMove.back()==ReturnMove.Blocks.back());

        if(PRINT)cerr << "::::::::::::::::::::::::::::::: "<<endl; 
        game.ExpansionTracker.ResetConsidered();
        int MissingReaction=false;
        if(game.BlocksInMove.back()+2==game.NumBlocks){
            MissingReaction=true;
            if(PRINT) cerr << "MissingReaction set to true "<<endl;
            assert(ReturnMove.Assignments.size()==game.BlocksInMove.size());
        }
        for (int g=0; g<game.Games.size() ;g++){
            int Counter=0;
            int CurrentBlock=game.BlocksInMove[Counter];
            if(PRINT) cerr << "------------------------------ "<<endl; 
            if(PRINT) cerr << "retrieve solution of game  " <<  g <<endl; 
            if(!game.Games[g].IsNULL && PRINT) cerr << "LinkerNode is " << game.Games[g].Assignments.back()[0].LinkerNode<<endl;
            if(MissingReaction) assert(game.ExpansionTracker.Nodes[game.Games[g].Assignments.back()[0].LinkerNode].Block==game.NumBlocks);
            deque < std::pair < int,bool > > TrackerPath;

            if(game.Games[g].IsNULL) TrackerPath.push_back(std::make_pair(0,false));
            else{ 
                assert(game.Games[g].Assignments.back().size()==1);
                TrackerPath = game.ExpansionTracker.RetrieveCurrentPathNoDoubles(game.Games[g].Assignments.back()[0].LinkerNode);
                game.ExpansionTracker.SetConsidered(game.Games[g].Assignments.back()[0].LinkerNode);
            }
            deque < int > MovePos;
            if(game.Games[g].IsNULL) MovePos.push_back(0);
            else
                MovePos = game.ExpansionTracker.RetrieveMovePositions(game.Games[g].Assignments.back()[0].LinkerNode);
             if(PRINT) cerr << "MovePos: ";
            PrintDeque(MovePos, PRINT);
            while(Counter < TrackerPath.size()  && CurrentBlock <= game.BlocksInMove.back()){
                 if(PRINT) cerr << Counter <<" The Tracker:" << TrackerPath[Counter].first << " " << TrackerPath[Counter].second << endl;
                if (Counter <TrackerPath.size() && TrackerPath.size()>0 && TrackerPath[Counter].second ==true){
                    CurrentBlock=CurrentBlock+2;
                     Counter++;
                    continue;
                }
                 if(PRINT) cerr << "Do it; CurrentBlock=" << CurrentBlock << "; Counter=" <<Counter<<endl;
                for (int i=0; i<BlockVars[CurrentBlock].size();i++){
                    string TheName=CopyNoDouble(BlockVars[CurrentBlock][i].Name,TrackerPath, Counter);
                    if(PRINT) cerr << "retrieve solution of  " <<  TheName  <<endl;
                    try{
                        GRBVar Var = Model->getVarByName(TheName);
                        ReturnMove.Assignments[Counter][MovePos[Counter]].Add(BlockVars[CurrentBlock][i].Idx,floor(Var.get(GRB_DoubleAttr_X)+0.5));
                        if(PRINT) cerr << std::to_string(Var.get(GRB_DoubleAttr_X)) << endl;
                    }
                    catch (GRBException& e) {
                        if (e.getErrorCode() == GRB_ERROR_DATA_NOT_AVAILABLE) {
                            std::cerr << "Variable " << TheName << " not found in the model." << std::endl;
                        } else {
                            std::cerr << "Error retrieving variable: " << TheName << ": " << e.getMessage() << std::endl;
                        }
                        assert(0);
                    }
                }
                int TheLink;
                if(game.Games[g].IsNULL){
                    assert(Counter==0);
                    TheLink = 0;
                    ReturnMove.Assignments[Counter][MovePos[Counter]].SetLinker(TheLink);
                }
                else{
                    TheLink=game.ExpansionTracker.RetrievePredecessor(game.Games[g].Assignments.back()[0].LinkerNode,MovePos.size()-2-Counter+MissingReaction);
                    assert(TheLink<game.ExpansionTracker.Nodes.size());
                    ReturnMove.Assignments[Counter][MovePos[Counter]].SetLinker(TheLink);
                }
                if(PRINT) cout << "This Link was "<< TheLink << " for Counter=" << Counter << " and MovePos[Counter]="<<MovePos[Counter] <<endl;
                CurrentBlock=CurrentBlock+2;
                Counter++;
            }
        }
        ReturnMove.IsNULL=false;
        if(game.ExpansionTracker.Leaves.size()>game.NumGames){
             if(PRINT) cerr << "Current ReturnMove:"<<endl;
            ReturnMove.Print(game.ExpansionTracker,false); 
            // We ignored some games in this Abstraction and hence we ignored the assignments of 
            // the corresonding auxiliary variables.
            // We need to fill values for these auxiliaries. 
            // We just copy them from other auxiliaries of the same underlying variable
            for (int l=0; l< game.ExpansionTracker.Leaves.size();l++){
                int MissingReaction=false;
                if(game.ExpansionTracker.Nodes[game.ExpansionTracker.Leaves[l]].Block ==game.NumBlocks){
                    MissingReaction=true;
                     if(PRINT) cerr << "MissingReaction set to true "<<endl;
                }
                if(PRINT) cerr << "Happens "<<l<<endl;
                int LeafPos=game.ExpansionTracker.Leaves[l];

                deque < int > MovePos=game.ExpansionTracker.RetrieveMovePositions(LeafPos);

                deque < int > Path=game.ExpansionTracker.RetrieveCurrentPath(LeafPos);
                deque < int > PathOfNodes=game.ExpansionTracker.RetrieveCurrentPathOfNodes(LeafPos);

                if(PRINT) cout << "Path=";
                PrintDeque(Path, PRINT);
                if(PRINT) cout << "PathOfNodes=";
                PrintDeque(PathOfNodes, PRINT);
                PrintDeque(MovePos, PRINT);
                for (int i=0; i< MovePos.size()-MissingReaction;i++){
                    if(PRINT) cerr << "ReturnMove.Assignments[i].size()=" << ReturnMove.Assignments[i].size() <<endl;
                    if(PRINT) cerr << "MovePos="; PrintDeque(MovePos, PRINT);
                    if(PRINT) cerr << "i="<<i<<endl;
                    assert(ReturnMove.Assignments.size()>i);  
                    if (ReturnMove.Assignments[i].size()<=MovePos[i] || ReturnMove.Assignments[i][MovePos[i]].SubGame.empty()){
                        if(PRINT) cerr << "Found empty subgame for " << i << " " << MovePos[i] << endl;
                        assert(ReturnMove.Assignments[i][MovePos[i]].LinkerNode > game.ExpansionTracker.Leaves.size());
                        ReturnMove.Assignments[i][MovePos[i]].SetLinker(PathOfNodes[i]);
                        if(PRINT) cout << "LinkerNode:" << ReturnMove.Assignments[i][MovePos[i]].LinkerNode << endl;
                        //if(PRINT) cout << "OR:" << game.ExpansionTracker.RetrievePredecessor(LeafPos,MovePos.size()-2-i+MissingReaction);
                        if(ReturnMove.Assignments[i].size()<=MovePos[i]){
                            while(ReturnMove.Assignments[i].size()<=MovePos[i]){
                                ReturnMove.Assignments[i].push_back(ReturnMove.Assignments[i].back());
                                assert(0);
                            }
                        }
                        else if (ReturnMove.Assignments[i][MovePos[i]].SubGame.empty()){
                            if(game.Q!=UNIVERSAL || NumConstrsUncertainty==0){
                                //In this case we just can copy some scenario 
                                // Find copy that is not empty
                                for (int j=0;j<ReturnMove.Assignments[i].size();j++){
                                    if(!ReturnMove.Assignments[i][j].SubGame.empty())
                                        ReturnMove.Assignments[i][MovePos[i]]=ReturnMove.Assignments[i][j];
                                }
                            }
                            else{
                                // otherwise we need to be careful to have a scenario adhering to the uncertainty set
                                // we try to find another scneario that stems from the same parents in the expansion tree
                                // i.e. we look at siblings of this node
                                // First find predecessor in tree that has non empty Assignment
                                bool FoundOne=false;
                                for (int pred=i-1;pred>=0;pred--){
                                    if(!ReturnMove.Assignments[pred][MovePos[pred]].SubGame.empty()){
                                        FoundOne=true;
                                        //Now find a successor of this one, that is not empty
                                        for (int succ=0;succ<game.ExpansionTracker.Nodes[ReturnMove.Assignments[pred][MovePos[pred]].LinkerNode].Successors.size(); succ++){
                                            int SuccNode=game.ExpansionTracker.Nodes[ReturnMove.Assignments[pred][MovePos[pred]].LinkerNode].Successors[succ];
                                            if (!ReturnMove.Assignments[i][game.ExpansionTracker.Nodes[SuccNode].PosInMove].SubGame.empty()){
                                                ReturnMove.Assignments[i][MovePos[i]]=ReturnMove.Assignments[i][game.ExpansionTracker.Nodes[SuccNode].PosInMove];
                                            }
                                        }
                                        break;
                                    }
                                }
                                assert(FoundOne);

                            }
                        }
                        //Already done differently above
                        //ReturnMove.Assignments[i][MovePos[i]].LinkerNode=game.ExpansionTracker.RetrievePredecessor(LeafPos,MovePos.size()-2-i+MissingReaction);
                    }
                }

                if(game.Q==UNIVERSAL){
                    //Check feasibility of scenarios
                    std::vector < Assignment> CurrentScenario=game.EarlierVars;
                    for (int i=0; i< MovePos.size()-MissingReaction;i++){
                        for (int j=0;j<ReturnMove.Assignments[i][MovePos[i]].SubGame.size();j++){
                            CurrentScenario.push_back(ReturnMove.Assignments[i][MovePos[i]].SubGame[j]);
                        }
                    }
                    UncBoundsManager.updateFixedVariables(UncertaintyModel, CurrentScenario,DictIdxToGRB);

                    UncertaintyModel.update();

                    UncertaintyModel.optimize();

                    int status = UncertaintyModel.get(GRB_IntAttr_Status);
                    if (status == GRB_INFEASIBLE){
                        UncertaintyModel.write("Problem.lp");
                    }
                    assert(status != GRB_INFEASIBLE);
                }
            }
            
            
        } 
        ReturnMove.Print(game.ExpansionTracker,false);
        if(PRINT) cerr << "::::::::::::::::::::::::::::::: "<<endl; 

        delete Model;
        delete Env;
        return ReturnMove;
    }
    assert(0);
}

void QISSolver::CheckTimeLimit(){
    chrono::steady_clock::time_point now = chrono::steady_clock::now();
    long long elapsed = chrono::duration_cast<chrono::seconds>(now - Start).count();
    //cout << "Time elapsed is " <<elapsed << endl;
    if (elapsed >= TimeLimit) {
        std::cout << "The time limit is reached. Process is stopped. "<<  '\n';
        std::cout << "Time: Limit "<<  '\n';
        std::cout << "Solved Exist-IPs: " << CounterExistIP <<endl;
        std::cout << "Solved All-IPs: " << CounterAllIP <<endl;
        exit(1);
    }
}

double QISSolver::GetRemainingTime(){
    chrono::steady_clock::time_point now = chrono::steady_clock::now();
    long long elapsed = chrono::duration_cast<chrono::seconds>(now - Start).count();
    return TimeLimit-elapsed;
}
bool QISSolver::Solve(){

    if (!IsOptimizationProblem){
        MultiGame FullInstance(Quantification);
        FullInstance.Print();
        Move WinningMove=RAReQIS(FullInstance);
        if (WinningMove.IsNULL){
            cout << "Starting Player loses" <<endl;
            cout << "Status: INFEASIBLE"<< endl;
        }
        else{
            cout << "Starting Player wins" <<endl;
            WinningMove.Print(FullInstance.ExpansionTracker, true);
            cout << "Status: OPTIMAL"<< endl;
        }
    }
    else{
        cout << "Check feasibility of problem with unbounded objective..." <<endl;
        MultiGame FullInstance(Quantification);
        FullInstance.Print();

        Move StoreMove=RAReQIS(FullInstance);
        if (StoreMove.IsNULL){
            cout << "Instance is infeasible; starting player loses" <<endl;
            cout << "Status: INFEASIBLE"<< endl;
            return false;
        }
        cout << "Found a solution. Now let's tighten the bounds!" <<endl;
        CheckTimeLimit();
        //int CurrentObjective=(int)ObjLowerBound+(ObjUpperBound-ObjLowerBound)/2;
        int CurrentObjective=ObjLowerBound;
        ChangeObjRHS(CurrentObjective);
        
        int BestFoundSolution;           
        BestFoundSolution=ObjUpperBound;

        while(ObjUpperBound-ObjLowerBound>=1){
            CheckTimeLimit();
            MultiGame FullInstance(Quantification);

            FullInstance.Print();
            Move WinningMove=RAReQIS(FullInstance,StoreMove);
            if (WinningMove.IsNULL){
                if(IsMinimizationProblem)
                cout << "Bound " <<  CurrentObjective << " is too tight" <<endl;
                else cout << "Bound " <<  -CurrentObjective << " is too tight" <<endl;
                ObjLowerBound=CurrentObjective+1;
                //CurrentObjective=CurrentObjective+2;
            }
            else{
                if(IsMinimizationProblem)
                cout << "Found solution with bound " << CurrentObjective  <<endl;
                else cout << "Found solution with bound " << -CurrentObjective  <<endl;
                BestFoundSolution=CurrentObjective;
                ObjUpperBound=CurrentObjective;
                //CurrentObjective--;
                StoreMove=WinningMove.Copy();
                
            }
            
            CurrentObjective=floor(ObjLowerBound+(ObjUpperBound-ObjLowerBound)/2);
            if(IsMinimizationProblem)
                cout << "Check with new objective: "<<CurrentObjective << "; current bounds: ["<<ObjLowerBound <<"," <<ObjUpperBound <<"]"<<endl;
            else  cout << "Check with new objective: "<<-CurrentObjective << "; current bounds: ["<<-ObjUpperBound <<"," <<-ObjLowerBound <<"]"<<endl;
            ChangeObjRHS(CurrentObjective);
        }
        if(!IsMinimizationProblem)BestFoundSolution=-BestFoundSolution;
        StoreMove.Print(FullInstance.ExpansionTracker, true);
        cout << "Status: OPTIMAL"<< endl;
        cout << "Objective: "<< BestFoundSolution << endl;
    }

    return true;
}

void QISSolver::UpdateAssignedVariablesInUncertaintyModel(MultiGame * game){
    
    for (int i=0;i<game->EarlierVars.size();i++){

    }

}

Move QISSolver::GetScenarioFromMainModel(const std::vector < std::vector <VariableInfo> >& block_info,MultiGame& game){

    MainBoundsManager.updateFixedVariables(MainModel, game.EarlierVars,DictIdxToGRB);
    MainModel.update();

    MainModel.optimize();

    int status = MainModel.get(GRB_IntAttr_Status);
    Move ReturnMove(&game);
    if (status == GRB_INFEASIBLE){
        //cout << "Did not find starting move"<<endl;
        MainBoundsManager.resetBounds(MainModel);
        MainModel.update();

        return ReturnMove;
        //MainModel.write("Problem.lp");
    }
    assert(status != GRB_INFEASIBLE);

    /*Move ReturnMove(game.Q, game.BlocksInMove[0],game.BlocksInMove.back());
      //Create space for all variables of interest in this move
    for (int b=0;b<game.BlocksInMove.size();b++){
        vector<PartialAssignment> NewVec;
        ReturnMove.Assignments.push_back(NewVec);
        //for (int i=0; i<game.ExpansionTracker.NumPosPerDepth[b]; i++ )
        //    ReturnMove.Assignments[b].push_back(PartialAssignment(game.BlocksInMove[b]));
    }*/
    
    assert(game.BlocksInMove.back()==ReturnMove.Blocks.back());
    GRBVar* vars = MainModel.getVars();
    for (int b=0;b< game.BlocksInMove.size();b++){
        int TheBlock=game.BlocksInMove[b];
        PartialAssignment Scenario(TheBlock);
        assert(ReturnMove.Assignments[b].size()==0);
        for (int i=0;i<block_info[TheBlock].size();i++){
            assert(DictIdxToGRB[block_info[TheBlock][i].Idx]==block_info[TheBlock][i].GRBNum);
            double Value=vars[DictIdxToGRB[block_info[TheBlock][i].Idx]].get(GRB_DoubleAttr_X);
            if (PRINT) cout <<"Add x_"<< block_info[TheBlock][i].Idx<<"="<<Value << endl;
            Scenario.Add(block_info[TheBlock][i].Idx,floor(Value+.5));
            // Iterating over the nodes of the expansion tree should be fine here
            // This is function is called for the empty abstraction
            // The empty abstraction also obtains the expansion tree of the underlying instance
            // Hence the expansion tree contains the exact information of the aux variables
        }
        for (int t=0; t<game.ExpansionTracker.NodesByBlock[b].size();t++){
            if (PRINT) cout <<"For Node "<< t << endl;
            ReturnMove.Add(Scenario, b);
            ReturnMove.Assignments[b].back().SetLinker(game.ExpansionTracker.NodesByBlock[b][t]);
        }
    }
    delete[] vars;
    MainBoundsManager.resetBounds(MainModel);
    MainModel.update();

    return ReturnMove;
}

Move QISSolver::GetScenarioFromUncertaintySet(const std::vector < std::vector <VariableInfo> >& block_info,MultiGame& game){

    UncBoundsManager.updateFixedVariables(UncertaintyModel, game.EarlierVars,DictIdxToGRB);
    UncertaintyModel.update();

    UncertaintyModel.optimize();

    int status = UncertaintyModel.get(GRB_IntAttr_Status);
    if (status == GRB_INFEASIBLE){
        UncertaintyModel.write("Problem.lp");
    }
    assert(status != GRB_INFEASIBLE);
    Move ReturnMove(&game);

    /*Move ReturnMove(game.Q, game.BlocksInMove[0],game.BlocksInMove.back());
      //Create space for all variables of interest in this move
    for (int b=0;b<game.BlocksInMove.size();b++){
        vector<PartialAssignment> NewVec;
        ReturnMove.Assignments.push_back(NewVec);
        //for (int i=0; i<game.ExpansionTracker.NumPosPerDepth[b]; i++ )
        //    ReturnMove.Assignments[b].push_back(PartialAssignment(game.BlocksInMove[b]));
    }*/
    
    assert(game.BlocksInMove.back()==ReturnMove.Blocks.back());
    GRBVar* vars = UncertaintyModel.getVars();
    for (int b=0;b< game.BlocksInMove.size();b++){
        int TheBlock=game.BlocksInMove[b];
        PartialAssignment Scenario(TheBlock);
        assert(ReturnMove.Assignments[b].size()==0);
        for (int i=0;i<block_info[TheBlock].size();i++){
            assert(DictIdxToGRB[block_info[TheBlock][i].Idx]==block_info[TheBlock][i].GRBNum);
            double Value=vars[DictIdxToGRB[block_info[TheBlock][i].Idx]].get(GRB_DoubleAttr_X);
            if (PRINT) cout <<"Add x_"<< block_info[TheBlock][i].Idx<<"="<<Value << endl;
            Scenario.Add(block_info[TheBlock][i].Idx,floor(Value+.5));
            // Iterating over the nodes of the expansion tree should be fine here
            // This is function is called for the empty abstraction
            // The empty abstraction also obtains the expansion tree of the underlying instance
            // Hence the expansion tree contains the exact information of the aux variables
        }
        for (int t=0; t<game.ExpansionTracker.NodesByBlock[b].size();t++){
            if (PRINT) cout <<"For Node "<< t << endl;
            ReturnMove.Add(Scenario, b);
            ReturnMove.Assignments[b].back().SetLinker(game.ExpansionTracker.NodesByBlock[b][t]);
        }
    }
    delete[] vars;

    return ReturnMove;
}

Move QISSolver::RAReQIS(MultiGame & game){
    Move EmptyMove(game.Q);
    return RAReQIS(game, EmptyMove);
}
Move QISSolver::RAReQIS(MultiGame & game, Move stored_move){
    CheckTimeLimit();
    if(PRINT) cout << "Start RAReQIS"<<endl;
    if(PRINT) game.Print();
    if (game.AllGamesQuantifierFree(Quantification))
        return Wins(game);

#ifdef LEARN_MODE
    //TODO
    //SetOfMovesAndCounterMoves = empty
#endif

    //Create empty abstraction of game; contains everything that game contains; but not the Games
    MultiGame AlphaAbstraction (game);
    if(PRINT) cout << "Created New Abstraction" << endl;
    if(PRINT) AlphaAbstraction.Print();
    Move MoveTao(game.Q);
    while(true){
        // we call RAReQIS on the new abstraction to find a winning move
        if (AlphaAbstraction.Games.empty()){
            // we have the empty comjunction; hence we return a "random" assignment; 
            // we also could solve the IP with all variables being existential... (TODO)
            // then we would maybe get a better starting solution
            if (!stored_move.IsNULL){
                MoveTao=stored_move.Copy();
                assert(game.ExpansionTracker.NodesByBlock[0].size()==1);
                // Assertion, because we currently only are able to do this at the root
                MoveTao.Assignments[0].back().SetLinker(game.ExpansionTracker.NodesByBlock[0][0]);
                if(PRINT) cout << "Used Stored Move" << endl;
            }
            else if (NumConstrsUncertainty==0 || game.Q==EXISTENTIAL){
                if(1){
                    MoveTao = GetScenarioFromMainModel(BlockVars,game);
                    MainModel.reset();
                    if(MoveTao.IsNULL){
                        if(game.Q==EXISTENTIAL){
                            return MoveTao;
                        }
                        else MoveTao = game.AllLowerBound(BlockVars); 
                    } 
                }
                else
                    MoveTao = game.AllLowerBound(BlockVars); 

                if(PRINT) cerr << "Found LB Move "<< endl;
            }
            else{
                MoveTao = GetScenarioFromUncertaintySet(BlockVars,game);
                if(PRINT) cout << "Created Scenario:"<<endl;
                MoveTao.Print(game.ExpansionTracker,false);
            }

        }
        else{
            // In this case, the abstraction contains some Games. 
            // These games remain in the MultiGame in the Games Vector 
            // This way, we know that we need auxiliary variables in case we end up solving an IP
            MoveTao = RAReQIS(AlphaAbstraction);
            if (MoveTao.IsNULL){
                return MoveTao;
            }
            if(PRINT) cerr << "Found Move from RAReQIS call"<< endl;

        }

        MoveTao.Transform(game.ExpansionTracker); 
        if(PRINT) cerr << "MoveTao after transform "<< endl;
        MoveTao.Print(AlphaAbstraction.ExpansionTracker,false);
        // Here we should consider all elements of game.BlocksInMove!!! So the moves belonging to the input game; not the abstraction 
        bool IsWinningMove=true;
        Move MoveMu((game.Q==EXISTENTIAL?UNIVERSAL:EXISTENTIAL), game.BlocksInMove.back()+1);
        int ell=-1;
        for (int g=0;g<game.Games.size();g++){
            if(PRINT) cerr << "In Loop "<< g+1<< "/"<<game.Games.size()<<endl;
            // Check whether all games present in the underlying multigame are won by MoveTao 
            // Here, the game part of the new abstraction changes the structure of the instance, as blocks are merged
            MultiGame CheckAbstraction(game, g, MoveTao);
            if(PRINT) cerr << "Created Abstraction to find Countermove based on Phi_"<<g<<endl;
            game.Games[g].Print(game.ExpansionTracker,false);
            MoveMu = RAReQIS(CheckAbstraction);
            if (!MoveMu.IsNULL){
                ell=g;
                if(PRINT) cerr << "Found Counter Move"<<endl;
                if(PRINT) MoveMu.Print(CheckAbstraction.ExpansionTracker,false);
                break;
            }
        }
        if (MoveMu.IsNULL){
            if(PRINT) cerr << "Ensured Winning Move"<<endl;
            if(PRINT) MoveTao.Print(AlphaAbstraction.ExpansionTracker,false);

            /*cerr << "Game Expansion Tracker"<<endl;
            game.ExpansionTracker.Print(true);
            cerr << "Abstraction Expansion Tracker"<<endl;

            AlphaAbstraction.ExpansionTracker.Print(true);*/
            return MoveTao;
        }
        else{
            assert(ell>=0);
            if(PRINT) cerr << "Before Refinement" << endl;
            if(PRINT) AlphaAbstraction.Print();
            AlphaAbstraction.Refine(game.Games[ell],MoveMu);
            if(PRINT) cerr << "Refined Abstraction"<<endl;
            if(PRINT) AlphaAbstraction.Print();
        }

    }
}

void QISSolver::PrintForDebug(){
    if (PRINT) return;
    cerr << "Quantification:"<<endl;
    for (int i=0;i<Quantification.size();i++) cerr << Quantification[i]<< " ";
    cerr << endl << endl;
    cerr << "BlockVars:"<<endl;
    for (int b=0;b<BlockVars.size();b++){
        cerr << "Block "<<b<< ":" <<endl;
        for (int i=0; i<BlockVars[b].size();i++){
            BlockVars[b][i].PrintVariableInfo();
        }
    }
}