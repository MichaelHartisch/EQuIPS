#include "gurobi_c++.h"
#include <vector>
#include <iostream>
#include <unordered_map>
#include "Moves.h"

class VariableBoundsManager {
public:
    std::vector<std::pair<double, double> > originalBounds; // Stores original (LB, UB) per variable

//public:
    // Store original bounds before modifying them
    void storeOriginalBounds(GRBModel& unc_model) {
        GRBVar* vars = unc_model.getVars();
        int numVars = unc_model.get(GRB_IntAttr_NumVars);

        for (int i = 0; i < numVars; ++i) {
            originalBounds.push_back(make_pair(vars[i].get(GRB_DoubleAttr_LB), vars[i].get(GRB_DoubleAttr_UB)));
        }
        delete[] vars;
    }

    // Reset all variable bounds to original values
    void resetBounds(GRBModel& model) {
        //cout << "Called reset"<<endl;
        GRBVar* vars = model.getVars();
        for (int i = 0; i < originalBounds.size(); ++i) {
            vars[i].set(GRB_DoubleAttr_LB, originalBounds[i].first);
            vars[i].set(GRB_DoubleAttr_UB, originalBounds[i].second);
            //cout << "Set Bounds of x_"<<i<<" back to "<< originalBounds[i].first << " , " << originalBounds[i].second << endl;
        }

        delete[] vars;
    }

    // Apply new fixed variable values
    void updateFixedVariables(GRBModel& model, const std::vector<Assignment>& earlier_vars, const std::vector <int> & dict_i_2_g) {
        resetBounds(model);  // Ensure all variables have original bounds before fixing new ones

        GRBVar* Vars = model.getVars();
        for (int i=0; i<earlier_vars.size();i++) {
            Vars[dict_i_2_g[earlier_vars[i].idx]].set(GRB_DoubleAttr_LB, earlier_vars[i].val);
            Vars[dict_i_2_g[earlier_vars[i].idx]].set(GRB_DoubleAttr_UB, earlier_vars[i].val);
            //cerr << "Set bounds of " << Vars[dict_i_2_g[earlier_vars[i].idx]].get(GRB_StringAttr_VarName) << " to " << earlier_vars[i].val << "; x_" <<earlier_vars[i].idx<<endl ;
        }
        delete[] Vars;
    }
};