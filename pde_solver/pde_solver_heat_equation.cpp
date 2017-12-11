#include "pde_solver_heat_equation.h"
#include "../math_module/math_module.h"

using namespace QtDataVisualization;

PdeSolverHeatEquation::PdeSolverHeatEquation() : PdeSolverBase()
{

}

PdeSolverHeatEquation::~PdeSolverHeatEquation()
{

}

void PdeSolverHeatEquation::get_solution(const PdeSettings& set)
{
    GraphSolution_t solution;
    solution.set = set;
    solution.graph_data.first.reserve(set.countT);
    solution.graph_data.second.reserve(set.countT);

    GraphDataSlice_t init_slice = get_initial_conditions(set);
    solution.graph_data.first.push_back(init_slice.first);
    solution.graph_data.second.push_back(init_slice.second);

//    //explicit solution
//    double val = MathModule::solve_heat_equation_explicitly(QVector2D(1, 1), set.minT, set);
//    solution.occuracy.push_back(val);

    GraphDataSlice_t cur_graph_data_slice;
    GraphDataSlice_t half_new_graph_data_slice;
    GraphDataSlice_t new_graph_data_slice;
    int t_count = 1;
    for (float t_val = set.minT + set.stepT; t_val < set.maxT; t_val += set.stepT)
    {
        cur_graph_data_slice.first = solution.graph_data.first.at(t_count - 1);
        cur_graph_data_slice.second = solution.graph_data.second.at(t_count - 1);

        half_new_graph_data_slice = alternating_direction_method(set, cur_graph_data_slice, 'x');
        new_graph_data_slice = alternating_direction_method(set, half_new_graph_data_slice, 'y');

        solution.graph_data.first.push_back(new_graph_data_slice.first);
        solution.graph_data.second.push_back(new_graph_data_slice.second);

        clear_graph_data_slice(half_new_graph_data_slice);

//        //explicit solution
//        double val = MathModule::solve_heat_equation_explicitly(QVector2D(1, 1), t_val, set);
//        solution.occuracy.push_back(val);

        emit solution_progress_update("Computing the equation...", int(float(t_count * 100) / set.countT));
        ++t_count;
    }

    emit solution_progress_update("", 100);
    qDebug() << "PdeSolverHeatEquation: Data generated";
    emit solution_generated(solution);
}

PdeSolverBase::GraphDataSlice_t PdeSolverHeatEquation::alternating_direction_method(const PdeSettings& set,
                                                                                    const GraphDataSlice_t& prev_graph_data_slice, char stencil)
{
    int max_index1, max_index2;
    float step1, step2;
    if (stencil == 'x')
    {
        max_index1 = set.countY;
        max_index2 = set.countX;
        step1 = set.stepY;
        step2 = set.stepX;
    }
    else if (stencil == 'y')
    {
        max_index1 = set.countX;
        max_index2 = set.countY;
        step1 = set.stepX;
        step2 = set.stepY;
    }
    else throw("Wrong stencil");

    GraphDataSlice_t cur_graph_data_slice(new QSurfaceDataArray(), new QSurfaceDataArray());
    cur_graph_data_slice.first->reserve(max_index1);

    std::vector<float> a(max_index1, -set.c * set.c / step1 / step1);
    std::vector<float> b(max_index1, 2 / set.stepT + 2 * set.c * set.c / step1 / step1);
    std::vector<float> c(max_index1, -set.c * set.c / step1 / step1);
    std::vector<float> d;

    int prev_ind1 = 0, next_ind1 = 0, prev_ind2 = 0, next_ind2 = 0;

    float z_val = 0.0f, z_val_t = 0.0f, u1, u2, u3;
    for (int index1 = 0; index1 < max_index1; ++index1)
    {
        QSurfaceDataRow *row = new QSurfaceDataRow();
        row->reserve(max_index2);

        d.clear();
        for (int index2 = 0; index2 < max_index2; ++index2)
        {
            if (index1 == 0) prev_ind1 = index1;
            else prev_ind1 = index1 - 1;
            if (index2 == 0) prev_ind2 = index2;
            else prev_ind2 = index2 - 1;

            if (index1 >= max_index1 - 1) next_ind1 = index1;
            else next_ind1 = index1 + 1;
            if (index2 >= max_index2 - 1) next_ind2 = index2;
            else next_ind2 = index2 + 1;

            if ((index1 == 0) || (index2 == 0) || (index1 == max_index1 - 1) || (index2 == max_index2 - 1))
            {
                u1 = 0;
                u2 = (2 / set.stepT - 2 * set.c * set.c / step2 / step2) * prev_graph_data_slice.first->at(index1)->at(index2).y();
                u3 = 0;
            }
            else if (stencil == 'x')
            {
                u1 = set.c * set.c / step2 / step2 * prev_graph_data_slice.first->at(index1)->at(prev_ind2).y();
                u2 = (2 / set.stepT - 2 * set.c * set.c / step2 / step2) * prev_graph_data_slice.first->at(index1)->at(index2).y();
                u3 = set.c * set.c / step2 / step2 * prev_graph_data_slice.first->at(index1)->at(next_ind2).y();
            }
            else if (stencil == 'y')
            {
                u1 = set.c * set.c / step2 / step2 * prev_graph_data_slice.first->at(prev_ind1)->at(index2).y();
                u2 = (2 / set.stepT - 2 * set.c * set.c / step2 / step2) * prev_graph_data_slice.first->at(index1)->at(index2).y();
                u3 = set.c * set.c / step2 / step2 * prev_graph_data_slice.first->at(next_ind1)->at(index2).y();
            }

            d.push_back(u1 + u2 + u3);
        }

        MathModule::solve_tridiagonal_equation(a, b, c, d, max_index1);
        for (int index2 = 0; index2 < max_index2; ++index2)
        {
            auto& prev_vector = prev_graph_data_slice.first->at(index2)->at(index1);
            z_val = d[index2];
            row->push_back(QVector3D(prev_vector.x(), z_val, prev_vector.z()));
        }
        cur_graph_data_slice.first->push_back(row);
    }

    return cur_graph_data_slice;
}
