// Filename: IBFEMethod.cpp
// Created on 5 Oct 2011 by Boyce Griffith
//
// Copyright (c) 2002-2013, Boyce Griffith
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of New York University nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/////////////////////////////// INCLUDES /////////////////////////////////////

#include "IBAMR_config.h"
#include "IBFEMethod.h"
#include "SAMRAI_config.h"
#include "ibamr/IBHierarchyIntegrator.h"
#include "ibamr/namespaces.h" // IWYU pragma: keep
#include "ibtk/IBTK_CHKERRQ.h"
#include "ibtk/IndexUtilities.h"
#include "ibtk/LEInteractor.h"
#include "ibtk/libmesh_utilities.h"
#include "libmesh/boundary_info.h"
#include "libmesh/dense_vector.h"
#include "libmesh/dof_map.h"
#include "libmesh/equation_systems.h"
#include "libmesh/fe_base.h"
#include "libmesh/fe_interface.h"
#include "libmesh/mesh.h"
#include "libmesh/petsc_vector.h"
#include "libmesh/quadrature.h"
#include "libmesh/string_to_enum.h"

using namespace libMesh;

/////////////////////////////// NAMESPACE ////////////////////////////////////

namespace IBAMR
{
/////////////////////////////// STATIC ///////////////////////////////////////

namespace
{
// Version of IBFEMethod restart file data.
static const int IBFE_METHOD_VERSION = 1;

inline
short int
get_dirichlet_bdry_ids(
    const std::vector<short int>& bdry_ids)
{
    short int dirichlet_bdry_ids = 0;
    for (std::vector<short int>::const_iterator cit = bdry_ids.begin(); cit != bdry_ids.end(); ++cit)
    {
        const short int bdry_id = *cit;
        if      (bdry_id == FEDataManager::ZERO_DISPLACEMENT_X_BDRY_ID  ) dirichlet_bdry_ids |= FEDataManager::ZERO_DISPLACEMENT_X_BDRY_ID;
        else if (bdry_id == FEDataManager::ZERO_DISPLACEMENT_Y_BDRY_ID  ) dirichlet_bdry_ids |= FEDataManager::ZERO_DISPLACEMENT_Y_BDRY_ID;
        else if (bdry_id == FEDataManager::ZERO_DISPLACEMENT_Z_BDRY_ID  ) dirichlet_bdry_ids |= FEDataManager::ZERO_DISPLACEMENT_Z_BDRY_ID;
        else if (bdry_id == FEDataManager::ZERO_DISPLACEMENT_XY_BDRY_ID ) dirichlet_bdry_ids |= FEDataManager::ZERO_DISPLACEMENT_XY_BDRY_ID;
        else if (bdry_id == FEDataManager::ZERO_DISPLACEMENT_XZ_BDRY_ID ) dirichlet_bdry_ids |= FEDataManager::ZERO_DISPLACEMENT_XZ_BDRY_ID;
        else if (bdry_id == FEDataManager::ZERO_DISPLACEMENT_YZ_BDRY_ID ) dirichlet_bdry_ids |= FEDataManager::ZERO_DISPLACEMENT_YZ_BDRY_ID;
        else if (bdry_id == FEDataManager::ZERO_DISPLACEMENT_XYZ_BDRY_ID) dirichlet_bdry_ids |= FEDataManager::ZERO_DISPLACEMENT_XYZ_BDRY_ID;
    }
    return dirichlet_bdry_ids;
}// get_dirichlet_bdry_ids

static const double POINT_FACTOR = 2.0;

inline double
get_elem_hmax(
    Elem* elem,
    const blitz::Array<double,2>& X_node)
{
    static const int MAX_NODES = (NDIM == 2 ? 9 : 27);
    Point s_node_cache[MAX_NODES];
    const int n_node = elem->n_nodes();
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(n_node <= MAX_NODES);
#endif
    for (int k = 0; k < n_node; ++k)
    {
        s_node_cache[k] = elem->point(k);
        Point& X = elem->point(k);
        for (int d = 0; d < NDIM; ++d)
        {
            X(d) = X_node(k,d);
        }
    }
    const double hmax = elem->hmax();
    for (int k = 0; k < n_node; ++k)
    {
        elem->point(k) = s_node_cache[k];
    }
    return hmax;
}// get_elem_hmax
}

const std::string IBFEMethod::       COORDS_SYSTEM_NAME = "IB coordinates system";
const std::string IBFEMethod::COORD_MAPPING_SYSTEM_NAME = "IB coordinate mapping system";
const std::string IBFEMethod::        FORCE_SYSTEM_NAME = "IB force system";
const std::string IBFEMethod::     VELOCITY_SYSTEM_NAME = "IB velocity system";
const std::string IBFEMethod::BODY_VELOCITY_SYSTEM_NAME = "body velocity system";

/////////////////////////////// PUBLIC ///////////////////////////////////////

IBFEMethod::IBFEMethod(
    const std::string& object_name,
    Pointer<Database> input_db,
    Mesh* mesh,
    int max_level_number,
    bool register_for_restart)
    : d_num_parts(1)
{
    commonConstructor(object_name, input_db, std::vector<Mesh*>(1,mesh), max_level_number, register_for_restart);
    return;
}// IBFEMethod

IBFEMethod::IBFEMethod(
    const std::string& object_name,
    Pointer<Database> input_db,
    const std::vector<Mesh*>& meshes,
    int max_level_number,
    bool register_for_restart)
    : d_num_parts(meshes.size())
{
    commonConstructor(object_name, input_db, meshes, max_level_number, register_for_restart);
    return;
}// IBFEMethod

IBFEMethod::~IBFEMethod()
{
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        delete d_equation_systems[part];
    }
    if (d_registered_for_restart)
    {
        RestartManager::getManager()->unregisterRestartItem(d_object_name);
        d_registered_for_restart = false;
    }
    return;
}// ~IBFEMethod

FEDataManager*
IBFEMethod::getFEDataManager(
    const unsigned int part) const
{
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(part < d_num_parts);
#endif
    return d_fe_data_managers[part];
}// getFEDataManager

void
IBFEMethod::registerConstrainedPart(
    unsigned int part)
{
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(part < d_num_parts);
#endif
    d_has_constrained_parts = true;
    d_constrained_part[part] = true;
    return;
}// registerConstrainedPart

void
IBFEMethod::registerConstrainedPartVelocityFunction(
    ConstrainedPartVelocityFcnPtr constrained_part_velocity_fcn,
    void* constrained_part_velocity_fcn_ctx,
    unsigned int part)
{
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(part < d_num_parts);
    TBOX_ASSERT(d_constrained_part[part]);
#endif
    d_constrained_part_velocity_fcns[part] = constrained_part_velocity_fcn;
    d_constrained_part_velocity_fcn_ctxs[part] = constrained_part_velocity_fcn_ctx;
    return;
}// registerConstrainedPartVelocityFunction

void
IBFEMethod::registerInitialCoordinateMappingFunction(
    CoordinateMappingFcnPtr coordinate_mapping_fcn,
    void* coordinate_mapping_fcn_ctx,
    const unsigned int part)
{
    d_coordinate_mapping_fcns    [part] = coordinate_mapping_fcn;
    d_coordinate_mapping_fcn_ctxs[part] = coordinate_mapping_fcn_ctx;
    return;
}// registerInitialCoordinateMappingFunction

void
IBFEMethod::registerPK1StressTensorFunction(
    PK1StressFcnPtr PK1_stress_fcn,
    std::vector<unsigned int> PK1_stress_fcn_systems,
    void* PK1_stress_fcn_ctx,
    const unsigned int part)
{
    d_PK1_stress_fcns       [part] = PK1_stress_fcn;
    d_PK1_stress_fcn_systems[part] = PK1_stress_fcn_systems;
    d_PK1_stress_fcn_ctxs   [part] = PK1_stress_fcn_ctx;
    return;
}// registerPK1StressTensorFunction

void
IBFEMethod::registerLagBodyForceFunction(
    LagBodyForceFcnPtr lag_body_force_fcn,
    std::vector<unsigned int> lag_body_force_fcn_systems,
    void* lag_body_force_fcn_ctx,
    const unsigned int part)
{
    d_lag_body_force_fcns       [part] = lag_body_force_fcn;
    d_lag_body_force_fcn_systems[part] = lag_body_force_fcn_systems;
    d_lag_body_force_fcn_ctxs   [part] = lag_body_force_fcn_ctx;
    return;
}// registerLagBodyForceFunction

void
IBFEMethod::registerLagPressureFunction(
    LagPressureFcnPtr lag_pressure_fcn,
    std::vector<unsigned int> lag_pressure_fcn_systems,
    void* lag_pressure_fcn_ctx,
    const unsigned int part)
{
    d_lag_pressure_fcns       [part] = lag_pressure_fcn;
    d_lag_pressure_fcn_systems[part] = lag_pressure_fcn_systems;
    d_lag_pressure_fcn_ctxs   [part] = lag_pressure_fcn_ctx;
    return;
}// registerLagPressureFunction

void
IBFEMethod::registerLagSurfaceForceFunction(
    LagSurfaceForceFcnPtr lag_surface_force_fcn,
    std::vector<unsigned int> lag_surface_force_fcn_systems,
    void* lag_surface_force_fcn_ctx,
    const unsigned int part)
{
    d_lag_surface_force_fcns       [part] = lag_surface_force_fcn;
    d_lag_surface_force_fcn_systems[part] = lag_surface_force_fcn_systems;
    d_lag_surface_force_fcn_ctxs   [part] = lag_surface_force_fcn_ctx;
    return;
}// registerLagSurfaceForceFunction

const IntVector<NDIM>&
IBFEMethod::getMinimumGhostCellWidth() const
{
    return d_ghosts;
}// getMinimumGhostCellWidth

void
IBFEMethod::setupTagBuffer(
    Array<int>& tag_buffer,
    Pointer<GriddingAlgorithm<NDIM> > gridding_alg) const
{
    const int finest_hier_ln = gridding_alg->getMaxLevels()-1;
    const int tsize = tag_buffer.size();
    tag_buffer.resizeArray(finest_hier_ln);
    for (int i = tsize; i < finest_hier_ln; ++i) tag_buffer[i] = 0;
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        const int gcw    = d_fe_data_managers[part]->getGhostCellWidth().max();
        const int tag_ln = d_fe_data_managers[part]->getLevelNumber()-1;
        if (tag_ln >= 0 && tag_ln < finest_hier_ln)
        {
            tag_buffer[tag_ln] = std::max(tag_buffer[tag_ln], gcw);
        }
    }
    for (int ln = finest_hier_ln-2; ln >= 0; --ln)
    {
        tag_buffer[ln] = std::max(tag_buffer[ln], tag_buffer[ln+1]/gridding_alg->getRatioToCoarserLevel(ln+1).max()+1);
    }
    return;
}// setupTagBuffer

void
IBFEMethod::preprocessIntegrateData(
    double current_time,
    double new_time,
    int /*num_cycles*/)
{
    d_current_time = current_time;
    d_new_time = new_time;
    d_half_time = current_time+0.5*(new_time-current_time);

    // Extract the FE data.
    d_X_systems      .resize(d_num_parts);
    d_X_current_vecs .resize(d_num_parts);
    d_X_new_vecs     .resize(d_num_parts);
    d_X_half_vecs    .resize(d_num_parts);
    d_X_IB_ghost_vecs.resize(d_num_parts);

    d_U_systems      .resize(d_num_parts);
    d_U_current_vecs .resize(d_num_parts);
    d_U_new_vecs     .resize(d_num_parts);
    d_U_half_vecs    .resize(d_num_parts);

    d_F_systems      .resize(d_num_parts);
    d_F_half_vecs    .resize(d_num_parts);
    d_F_IB_ghost_vecs.resize(d_num_parts);

    d_U_b_systems    .resize(d_num_parts);
    d_U_b_vecs       .resize(d_num_parts);

    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        d_X_systems      [part] = &d_equation_systems[part]->get_system(  COORDS_SYSTEM_NAME);
        d_X_current_vecs [part] = dynamic_cast<PetscVector<double>*>(d_X_systems       [part]->solution.get());
        d_X_new_vecs     [part] = dynamic_cast<PetscVector<double>*>(d_X_current_vecs  [part]->clone().release());  // WARNING: must be manually deleted
        d_X_half_vecs    [part] = dynamic_cast<PetscVector<double>*>(d_X_systems       [part]->current_local_solution.get());
        d_X_IB_ghost_vecs[part] = dynamic_cast<PetscVector<double>*>(d_fe_data_managers[part]->buildGhostedCoordsVector());

        d_U_systems      [part] = &d_equation_systems[part]->get_system(VELOCITY_SYSTEM_NAME);
        d_U_current_vecs [part] = dynamic_cast<PetscVector<double>*>(d_U_systems       [part]->solution.get());
        d_U_new_vecs     [part] = dynamic_cast<PetscVector<double>*>(d_U_current_vecs  [part]->clone().release());  // WARNING: must be manually deleted
        d_U_half_vecs    [part] = dynamic_cast<PetscVector<double>*>(d_U_systems       [part]->current_local_solution.get());

        d_F_systems      [part] = &d_equation_systems[part]->get_system(   FORCE_SYSTEM_NAME);
        d_F_half_vecs    [part] = dynamic_cast<PetscVector<double>*>(d_F_systems       [part]->solution.get());
        d_F_IB_ghost_vecs[part] = dynamic_cast<PetscVector<double>*>(d_fe_data_managers[part]->buildGhostedSolutionVector(FORCE_SYSTEM_NAME));

        if (d_constrained_part[part])
        {
            d_U_b_systems[part] = &d_equation_systems[part]->get_system(BODY_VELOCITY_SYSTEM_NAME);
            d_U_b_vecs   [part] = dynamic_cast<PetscVector<double>*>(d_U_b_systems[part]->solution.get());
        }

        // Initialize X^{n+1/2} and X^{n+1} to equal X^{n}, and initialize
        // U^{n+1/2} and U^{n+1} to equal U^{n}.
        d_X_current_vecs[part]->localize(*d_X_half_vecs[part]);
        d_X_half_vecs[part]->close();
        d_X_current_vecs[part]->localize(*d_X_new_vecs[part]);
        d_X_new_vecs[part]->close();
        d_U_current_vecs[part]->localize(*d_U_half_vecs[part]);
        d_U_half_vecs[part]->close();
        d_U_current_vecs[part]->localize(*d_U_new_vecs[part]);
        d_U_new_vecs[part]->close();

        // Compute the constraint velocity.
        if (d_constrained_part[part] && d_constrained_part_velocity_fcns[part])
        {
            EquationSystems* equation_systems = d_fe_data_managers[part]->getEquationSystems();
            MeshBase& mesh = equation_systems->get_mesh();
            d_constrained_part_velocity_fcns[part](*d_U_b_vecs[part], *d_U_current_vecs[part], *d_X_current_vecs[part], mesh, d_current_time, d_constrained_part_velocity_fcn_ctxs[part]);
        }
    }
    return;
}// preprocessIntegrateData

void
IBFEMethod::postprocessIntegrateData(
    double /*current_time*/,
    double /*new_time*/,
    int /*num_cycles*/)
{
    for (unsigned part = 0; part < d_num_parts; ++part)
    {
        // Reset time-dependent Lagrangian data.
        (*d_X_current_vecs[part]) = (*d_X_new_vecs[part]);
        (*d_U_current_vecs[part]) = (*d_U_new_vecs[part]);

        d_X_systems[part]->solution->localize(*d_X_systems[part]->current_local_solution);
        d_U_systems[part]->solution->localize(*d_U_systems[part]->current_local_solution);
        d_F_systems[part]->solution->localize(*d_F_systems[part]->current_local_solution);

        // Update the coordinate mapping dX = X - s.
        updateCoordinateMapping(part);

        // Deallocate Lagrangian scratch data.
        delete d_X_new_vecs[part];
        delete d_U_new_vecs[part];
    }
    d_X_systems      .clear();
    d_X_current_vecs .clear();
    d_X_new_vecs     .clear();
    d_X_half_vecs    .clear();
    d_X_IB_ghost_vecs.clear();

    d_U_systems      .clear();
    d_U_current_vecs .clear();
    d_U_new_vecs     .clear();
    d_U_half_vecs    .clear();

    d_F_systems      .clear();
    d_F_half_vecs    .clear();
    d_F_IB_ghost_vecs.clear();

    d_U_b_systems    .clear();
    d_U_b_vecs       .clear();

    // Reset the current time step interval.
    d_current_time = std::numeric_limits<double>::quiet_NaN();
    d_new_time     = std::numeric_limits<double>::quiet_NaN();
    d_half_time    = std::numeric_limits<double>::quiet_NaN();
    return;
}// postprocessIntegrateData

void
IBFEMethod::interpolateVelocity(
    const int u_data_idx,
    const std::vector<Pointer<CoarsenSchedule<NDIM> > >& /*u_synch_scheds*/,
    const std::vector<Pointer<RefineSchedule<NDIM> > >& u_ghost_fill_scheds,
    const double data_time)
{
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        NumericVector<double>* X_vec = NULL;
        NumericVector<double>* X_ghost_vec = d_X_IB_ghost_vecs[part];
        NumericVector<double>* U_vec = NULL;
        if (MathUtilities<double>::equalEps(data_time, d_current_time))
        {
            X_vec = d_X_current_vecs[part];
            U_vec = d_U_current_vecs[part];
        }
        else if (MathUtilities<double>::equalEps(data_time, d_half_time))
        {
            X_vec = d_X_half_vecs[part];
            U_vec = d_U_half_vecs[part];
        }
        else if (MathUtilities<double>::equalEps(data_time, d_new_time))
        {
            X_vec = d_X_new_vecs[part];
            U_vec = d_U_new_vecs[part];
        }
        X_vec->localize(*X_ghost_vec);
        X_ghost_vec->close();
        if (d_use_IB_interp_operator)
        {
            d_fe_data_managers[part]->interp(u_data_idx, *U_vec, *X_ghost_vec, VELOCITY_SYSTEM_NAME, u_ghost_fill_scheds, data_time, false);
        }
        else
        {
            d_fe_data_managers[part]->restrictData(u_data_idx, *U_vec, *X_ghost_vec, VELOCITY_SYSTEM_NAME, false);
        }
    }
    return;
}// interpolateVelocity

void
IBFEMethod::eulerStep(
    const double current_time,
    const double new_time)
{
    const double dt = new_time-current_time;
    int ierr;
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        PetscVector<double>* U_current_vec = NULL;
        if (d_constrained_part[part])
        {
            U_current_vec = d_U_b_vecs[part];
        }
        else
        {
            U_current_vec = d_U_current_vecs[part];
        }
        ierr = VecWAXPY(d_X_new_vecs[part]->vec(), dt, U_current_vec->vec(), d_X_current_vecs[part]->vec()); IBTK_CHKERRQ(ierr);
        ierr = VecAXPBYPCZ(d_X_half_vecs[part]->vec(), 0.5, 0.5, 0.0, d_X_current_vecs[part]->vec(), d_X_new_vecs[part]->vec());  IBTK_CHKERRQ(ierr);
        d_X_new_vecs [part]->close();
        d_X_half_vecs[part]->close();
    }
    return;
}// eulerStep

void
IBFEMethod::midpointStep(
    const double current_time,
    const double new_time)
{
    const double dt = new_time-current_time;
    int ierr;
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        PetscVector<double>* U_half_vec = NULL;
        if (d_constrained_part[part])
        {
            U_half_vec = d_U_b_vecs[part];
        }
        else
        {
            U_half_vec = d_U_half_vecs[part];
        }
        ierr = VecWAXPY(d_X_new_vecs[part]->vec(), dt, U_half_vec->vec(), d_X_current_vecs[part]->vec()); IBTK_CHKERRQ(ierr);
        ierr = VecAXPBYPCZ(d_X_half_vecs[part]->vec(), 0.5, 0.5, 0.0, d_X_current_vecs[part]->vec(), d_X_new_vecs[part]->vec()); IBTK_CHKERRQ(ierr);
        d_X_new_vecs [part]->close();
        d_X_half_vecs[part]->close();
    }
    return;
}// midpointStep

void
IBFEMethod::trapezoidalStep(
    const double current_time,
    const double new_time)
{
    const double dt = new_time-current_time;
    int ierr;
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        PetscVector<double>* U_current_vec = NULL;
        PetscVector<double>* U_new_vec     = NULL;
        if (d_constrained_part[part])
        {
            U_current_vec = d_U_b_vecs[part];
            U_new_vec     = d_U_b_vecs[part];
        }
        else
        {
            U_current_vec = d_U_current_vecs[part];
            U_new_vec     = d_U_new_vecs    [part];
        }
        ierr = VecWAXPY(d_X_new_vecs[part]->vec(), 0.5*dt, U_current_vec->vec(), d_X_current_vecs[part]->vec()); IBTK_CHKERRQ(ierr);
        ierr = VecAXPY( d_X_new_vecs[part]->vec(), 0.5*dt, U_new_vec    ->vec()); IBTK_CHKERRQ(ierr);
        ierr = VecAXPBYPCZ(d_X_half_vecs[part]->vec(), 0.5, 0.5, 0.0, d_X_current_vecs[part]->vec(), d_X_new_vecs[part]->vec()); IBTK_CHKERRQ(ierr);
        d_X_new_vecs [part]->close();
        d_X_half_vecs[part]->close();
    }
    return;
}// trapezoidalStep

void
IBFEMethod::computeLagrangianForce(
    const double data_time)
{
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(MathUtilities<double>::equalEps(data_time, d_half_time));
#endif
    for (unsigned part = 0; part < d_num_parts; ++part)
    {
        if (d_constrained_part[part])
        {
           computeConstraintForceDensity(*d_F_half_vecs[part], *d_X_half_vecs[part], *d_U_half_vecs[part], *d_U_b_vecs[part], data_time, part);
        }
        else
        {
            computeInteriorForceDensity(*d_F_half_vecs[part], *d_X_half_vecs[part], data_time, part);
        }
    }
    return;
}// computeLagrangianForce

void
IBFEMethod::spreadForce(
    const int f_data_idx,
    const std::vector<Pointer<RefineSchedule<NDIM> > >& /*f_prolongation_scheds*/,
    const double data_time)
{
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(MathUtilities<double>::equalEps(data_time, d_half_time));
#endif
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        PetscVector<double>* X_vec = d_X_half_vecs[part];
        PetscVector<double>* X_ghost_vec = d_X_IB_ghost_vecs[part];
        PetscVector<double>* F_vec = d_F_half_vecs[part];
        PetscVector<double>* F_ghost_vec = d_F_IB_ghost_vecs[part];
        X_vec->localize(*X_ghost_vec);
        X_ghost_vec->close();
        F_vec->localize(*F_ghost_vec);
        F_ghost_vec->close();
        if (d_use_IB_spread_operator)
        {
            d_fe_data_managers[part]->spread(f_data_idx, *F_ghost_vec, *X_ghost_vec, FORCE_SYSTEM_NAME, false, false);
        }
        else
        {
            d_fe_data_managers[part]->prolongData(f_data_idx, *F_ghost_vec, *X_ghost_vec, FORCE_SYSTEM_NAME, false, false, /*is_density*/ true, /*accumulate_on_grid*/ true);
        }
        if (d_split_forces)
        {
            if (d_use_jump_conditions)
            {
                imposeJumpConditions(f_data_idx, *F_ghost_vec, *X_ghost_vec, data_time, part);
            }
            else
            {
                spreadTransmissionForceDensity(f_data_idx, *X_ghost_vec, data_time, part);
            }
        }
    }
    return;
}// spreadForce

void
IBFEMethod::initializeFEData()
{
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        EquationSystems* equation_systems = d_equation_systems[part];

        // Create FE systems and corresponding variables.
        d_fe_data_managers[part]->COORDINATES_SYSTEM_NAME = COORDS_SYSTEM_NAME;
        System& X_system = equation_systems->add_system<System>(COORDS_SYSTEM_NAME);
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            std::ostringstream os;
            os << "X_" << d;
            X_system.add_variable(os.str(), d_fe_order, d_fe_family);
        }

        System& X_mapping_system = equation_systems->add_system<System>(COORD_MAPPING_SYSTEM_NAME);
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            std::ostringstream os;
            os << "dX_" << d;
            X_mapping_system.add_variable(os.str(), d_fe_order, d_fe_family);
        }

        System& U_system = equation_systems->add_system<System>(VELOCITY_SYSTEM_NAME);
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            std::ostringstream os;
            os << "U_" << d;
            U_system.add_variable(os.str(), d_fe_order, d_fe_family);
        }

        System& F_system = equation_systems->add_system<System>(FORCE_SYSTEM_NAME);
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            std::ostringstream os;
            os << "F_" << d;
            F_system.add_variable(os.str(), d_fe_order, d_fe_family);
        }

        System* U_b_system;
        if (d_constrained_part[part])
        {
            U_b_system = &equation_systems->add_system<System>(BODY_VELOCITY_SYSTEM_NAME);
            for (unsigned int d = 0; d < NDIM; ++d)
            {
                std::ostringstream os;
                os << "U_b_" << d;
                U_b_system->add_variable(os.str(), d_fe_order, d_fe_family);
            }
        }

        // Initialize FE equation systems.
        equation_systems->init();
        initializeCoordinates(part);
        updateCoordinateMapping(part);

        // Assemble systems.
        X_system.assemble_before_solve = false;
        X_system.assemble();

        X_mapping_system.assemble_before_solve = false;
        X_mapping_system.assemble();

        U_system.assemble_before_solve = false;
        U_system.assemble();

        F_system.assemble_before_solve = false;
        F_system.assemble();

        if (d_constrained_part[part])
        {
            U_b_system->assemble_before_solve = false;
            U_b_system->assemble();
        }

        // Set up boundary conditions.  Specifically, add appropriate boundary
        // IDs to the BoundaryInfo object associated with the mesh, and add DOF
        // constraints for the nodal forces and velocities.
        const MeshBase& mesh = equation_systems->get_mesh();
        DofMap& F_dof_map = F_system.get_dof_map();
        DofMap& U_dof_map = U_system.get_dof_map();
        const unsigned int F_sys_num = F_system.number();
        const unsigned int U_sys_num = U_system.number();
        MeshBase::const_element_iterator       el_it  = mesh.elements_begin();
        const MeshBase::const_element_iterator el_end = mesh.elements_end();
        for ( ; el_it != el_end; ++el_it)
        {
            Elem* const elem = *el_it;
            for (unsigned int side = 0; side < elem->n_sides(); ++side)
            {
                const bool at_mesh_bdry = !elem->neighbor(side);
                if (!at_mesh_bdry) continue;

                static const short int dirichlet_bdry_id_set[3] = { FEDataManager::ZERO_DISPLACEMENT_X_BDRY_ID , FEDataManager::ZERO_DISPLACEMENT_Y_BDRY_ID , FEDataManager::ZERO_DISPLACEMENT_Z_BDRY_ID };
                const short int dirichlet_bdry_ids = get_dirichlet_bdry_ids(mesh.boundary_info->boundary_ids(elem, side));
                if (!dirichlet_bdry_ids) continue;

                for (unsigned int n = 0; n < elem->n_nodes(); ++n)
                {
                    if (!elem->is_node_on_side(n, side)) continue;

                    Node* node = elem->get_node(n);
                    mesh.boundary_info->add_node(node, dirichlet_bdry_ids);
                    for (unsigned int d = 0; d < NDIM; ++d)
                    {
                        if (!(dirichlet_bdry_ids & dirichlet_bdry_id_set[d])) continue;
                        if (node->n_dofs(F_sys_num) > 0)
                        {
                            const int F_dof_index = node->dof_number(F_sys_num,d,0);
                            DofConstraintRow F_constraint_row;
                            F_constraint_row[F_dof_index] = 1.0;
                            F_dof_map.add_constraint_row(F_dof_index, F_constraint_row, 0.0, false);
                        }
                        if (node->n_dofs(U_sys_num) > 0)
                        {
                            const int U_dof_index = node->dof_number(U_sys_num,d,0);
                            DofConstraintRow U_constraint_row;
                            U_constraint_row[U_dof_index] = 1.0;
                            U_dof_map.add_constraint_row(U_dof_index, U_constraint_row, 0.0, false);
                        }
                    }
                }
            }
        }
    }

    d_is_initialized = true;
    return;
}// initializeFEData

void
IBFEMethod::initializePatchHierarchy(
    Pointer<PatchHierarchy<NDIM> > hierarchy,
    Pointer<GriddingAlgorithm<NDIM> > gridding_alg,
    int /*u_data_idx*/,
    const std::vector<Pointer<CoarsenSchedule<NDIM> > >& /*u_synch_scheds*/,
    const std::vector<Pointer<RefineSchedule<NDIM> > >& /*u_ghost_fill_scheds*/,
    int /*integrator_step*/,
    double /*init_data_time*/,
    bool /*initial_time*/)
{
    // Cache pointers to the patch hierarchy and gridding algorithm.
    d_hierarchy = hierarchy;
    d_gridding_alg = gridding_alg;

    // Initialize the FE data manager.
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        d_fe_data_managers[part]->reinitElementMappings();
    }

    d_is_initialized = true;
    return;
}// initializePatchHierarchy

void
IBFEMethod::registerLoadBalancer(
    Pointer<LoadBalancer<NDIM> > load_balancer,
    int workload_data_idx)
{
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(load_balancer);
#endif
    d_load_balancer = load_balancer;
    d_workload_idx = workload_data_idx;

    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        d_fe_data_managers[part]->registerLoadBalancer(load_balancer, workload_data_idx);
    }
    return;
}// registerLoadBalancer

void
IBFEMethod::updateWorkloadEstimates(
    Pointer<PatchHierarchy<NDIM> > /*hierarchy*/,
    int /*workload_data_idx*/)
{
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        d_fe_data_managers[part]->updateWorkloadEstimates();
    }
    return;
}// updateWorkloadEstimates

void
IBFEMethod::beginDataRedistribution(
    Pointer<PatchHierarchy<NDIM> > /*hierarchy*/,
    Pointer<GriddingAlgorithm<NDIM> > /*gridding_alg*/)
{
    // intentionally blank
    return;
}// beginDataRedistribution

void
IBFEMethod::endDataRedistribution(
    Pointer<PatchHierarchy<NDIM> > /*hierarchy*/,
    Pointer<GriddingAlgorithm<NDIM> > /*gridding_alg*/)
{
    if (d_is_initialized)
    {
        for (unsigned int part = 0; part < d_num_parts; ++part)
        {
            d_fe_data_managers[part]->reinitElementMappings();
        }
    }
    return;
}// endDataRedistribution

void
IBFEMethod::initializeLevelData(
    Pointer<BasePatchHierarchy<NDIM> > hierarchy,
    int level_number,
    double init_data_time,
    bool can_be_refined,
    bool initial_time,
    Pointer<BasePatchLevel<NDIM> > old_level,
    bool allocate_data)
{
    const int finest_hier_level = hierarchy->getFinestLevelNumber();
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        d_fe_data_managers[part]->setPatchHierarchy(hierarchy);
        d_fe_data_managers[part]->resetLevels(0,finest_hier_level);
        d_fe_data_managers[part]->initializeLevelData(hierarchy, level_number, init_data_time, can_be_refined, initial_time, old_level, allocate_data);
        if (d_load_balancer && level_number == d_fe_data_managers[part]->getLevelNumber())
        {
            d_load_balancer->setWorkloadPatchDataIndex(d_workload_idx, level_number);
            d_fe_data_managers[part]->updateWorkloadEstimates(level_number, level_number);
        }
    }
    return;
}// initializeLevelData

void
IBFEMethod::resetHierarchyConfiguration(
    Pointer<BasePatchHierarchy<NDIM> > hierarchy,
    int coarsest_level,
    int /*finest_level*/)
{
    const int finest_hier_level = hierarchy->getFinestLevelNumber();
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        d_fe_data_managers[part]->setPatchHierarchy(hierarchy);
        d_fe_data_managers[part]->resetLevels(0,hierarchy->getFinestLevelNumber());
        d_fe_data_managers[part]->resetHierarchyConfiguration(hierarchy, coarsest_level, finest_hier_level);
    }
    return;
}// resetHierarchyConfiguration

void
IBFEMethod::applyGradientDetector(
    Pointer<BasePatchHierarchy<NDIM> > base_hierarchy,
    int level_number,
    double error_data_time,
    int tag_index,
    bool initial_time,
    bool uses_richardson_extrapolation_too)
{
    Pointer<PatchHierarchy<NDIM> > hierarchy = base_hierarchy;
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(hierarchy);
    TBOX_ASSERT((level_number >= 0) && (level_number <= hierarchy->getFinestLevelNumber()));
    TBOX_ASSERT(hierarchy->getPatchLevel(level_number));
#endif
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        d_fe_data_managers[part]->applyGradientDetector(hierarchy, level_number, error_data_time, tag_index, initial_time, uses_richardson_extrapolation_too);
    }
    return;
}// applyGradientDetector

void
IBFEMethod::putToDatabase(
    Pointer<Database> db)
{
    db->putInteger("IBFE_METHOD_VERSION", IBFE_METHOD_VERSION);
    db->putIntegerArray("d_ghosts", d_ghosts, NDIM);
    db->putBool("d_use_IB_spread_operator", d_use_IB_spread_operator);
    db->putString("d_spread_delta_fcn", d_spread_delta_fcn);
    db->putBool("d_use_IB_interp_operator", d_use_IB_interp_operator);
    db->putString("d_interp_delta_fcn", d_interp_delta_fcn);
    db->putBool("d_split_forces", d_split_forces);
    db->putBool("d_use_jump_conditions", d_use_jump_conditions);
    db->putBool("d_use_consistent_mass_matrix", d_use_consistent_mass_matrix);
    db->putString("d_fe_family", Utility::enum_to_string<FEFamily>(d_fe_family));
    db->putString("d_fe_order", Utility::enum_to_string<Order>(d_fe_order));
    db->putString("d_quad_type", Utility::enum_to_string<QuadratureType>(d_quad_type));
    db->putString("d_quad_order", Utility::enum_to_string<Order>(d_quad_order));
    return;
}// putToDatabase

/////////////////////////////// PROTECTED ////////////////////////////////////

void
IBFEMethod::computeConstraintForceDensity(
    PetscVector<double>& F_vec,
    PetscVector<double>& X_vec,
    PetscVector<double>& U_vec,
    PetscVector<double>& U_b_vec,
    const double data_time,
    const unsigned int part)
{
    if (d_constrained_part[part])
    {
        EquationSystems* equation_systems = d_fe_data_managers[part]->getEquationSystems();
        MeshBase& mesh = equation_systems->get_mesh();
        if (d_constrained_part_velocity_fcns[part])
        {
            d_constrained_part_velocity_fcns[part](U_b_vec, U_vec, X_vec, mesh, data_time, d_constrained_part_velocity_fcn_ctxs[part]);
        }
        const double dt = d_new_time-d_current_time;
        const double rho = getINSHierarchyIntegrator()->getStokesSpecifications()->getRho();
        int ierr = VecAXPBYPCZ(F_vec.vec(), d_constraint_omega*rho/dt, -d_constraint_omega*rho/dt, 0.0, U_b_vec.vec(), U_vec.vec()); IBTK_CHKERRQ(ierr);
        F_vec.close();
    }
    else
    {
        F_vec.zero();
        F_vec.close();
    }
    return;
}// computeConstraintForceDensity

void
IBFEMethod::computeInteriorForceDensity(
    PetscVector<double>& G_vec,
    PetscVector<double>& X_vec,
    const double data_time,
    const unsigned int part)
{
    // Extract the mesh.
    EquationSystems* equation_systems = d_fe_data_managers[part]->getEquationSystems();
    const MeshBase& mesh = equation_systems->get_mesh();
    const int dim = mesh.mesh_dimension();
    AutoPtr<QBase> qrule = QBase::build(d_quad_type, dim, d_quad_order);
    AutoPtr<QBase> qrule_face = QBase::build(d_quad_type, dim-1, d_quad_order);

    // Extract the FE systems and DOF maps, and setup the FE objects.
    System& system = equation_systems->get_system(FORCE_SYSTEM_NAME);
    const DofMap& dof_map = system.get_dof_map();
#ifdef DEBUG_CHECK_ASSERTIONS
    for (unsigned int d = 0; d < NDIM; ++d) TBOX_ASSERT(dof_map.variable_type(d) == dof_map.variable_type(0));
#endif
    blitz::Array<std::vector<unsigned int>,1> dof_indices(NDIM);
    for (unsigned int d = 0; d < NDIM; ++d) dof_indices(d).reserve(27);
    AutoPtr<FEBase> fe(FEBase::build(dim, dof_map.variable_type(0)));
    fe->attach_quadrature_rule(qrule.get());
    const std::vector<Point>& q_point = fe->get_xyz();
    const std::vector<double>& JxW = fe->get_JxW();
    const std::vector<std::vector<double> >& phi = fe->get_phi();
    const std::vector<std::vector<VectorValue<double> > >& dphi = fe->get_dphi();
    AutoPtr<FEBase> fe_face(FEBase::build(dim, dof_map.variable_type(0)));
    fe_face->attach_quadrature_rule(qrule_face.get());
    const std::vector<Point>& q_point_face = fe_face->get_xyz();
    const std::vector<double>& JxW_face = fe_face->get_JxW();
    const std::vector<Point>& normal_face = fe_face->get_normals();
    const std::vector<std::vector<double> >& phi_face = fe_face->get_phi();
    const std::vector<std::vector<VectorValue<double> > >& dphi_face = fe_face->get_dphi();

#ifdef DEBUG_CHECK_ASSERTIONS
    System& X_system = equation_systems->get_system(COORDS_SYSTEM_NAME);
    const DofMap& X_dof_map = X_system.get_dof_map();
    for (unsigned int d = 0; d < NDIM; ++d) TBOX_ASSERT(dof_map.variable_type(d) == X_dof_map.variable_type(d));
#endif

    // Setup extra data needed to compute stresses/forces.
    std::vector<NumericVector<double>*> PK1_stress_fcn_data;
    for (std::vector<unsigned int>::const_iterator cit = d_PK1_stress_fcn_systems[part].begin(); cit != d_PK1_stress_fcn_systems[part].end(); ++cit)
    {
        System& system = equation_systems->get_system(*cit);
        system.update();
        PK1_stress_fcn_data.push_back(system.current_local_solution.get());
    }

    std::vector<NumericVector<double>*> lag_body_force_fcn_data;
    for (std::vector<unsigned int>::const_iterator cit = d_lag_body_force_fcn_systems[part].begin(); cit != d_lag_body_force_fcn_systems[part].end(); ++cit)
    {
        System& system = equation_systems->get_system(*cit);
        system.update();
        lag_body_force_fcn_data.push_back(system.current_local_solution.get());
    }

    std::vector<NumericVector<double>*> lag_pressure_fcn_data;
    for (std::vector<unsigned int>::const_iterator cit = d_lag_pressure_fcn_systems[part].begin(); cit != d_lag_pressure_fcn_systems[part].end(); ++cit)
    {
        System& system = equation_systems->get_system(*cit);
        system.update();
        lag_pressure_fcn_data.push_back(system.current_local_solution.get());
    }

    std::vector<NumericVector<double>*> lag_surface_force_fcn_data;
    for (std::vector<unsigned int>::const_iterator cit = d_lag_surface_force_fcn_systems[part].begin(); cit != d_lag_surface_force_fcn_systems[part].end(); ++cit)
    {
        System& system = equation_systems->get_system(*cit);
        system.update();
        lag_surface_force_fcn_data.push_back(system.current_local_solution.get());
    }

    // Setup global and elemental right-hand-side vectors.
    AutoPtr<NumericVector<double> > G_rhs_vec = G_vec.zero_clone();
    DenseVector<double> G_rhs_e[NDIM];

    // Loop over the elements to compute the right-hand side vector.  This is
    // computed via
    //
    //    rhs_k = -int{PP(s,t) grad phi_k(s)}ds + int{PP(s,t) N(s,t) phi_k(s)}dA(s)
    //
    // This right-hand side vector is used to solve for the nodal values of the
    // interior elastic force density.
    TensorValue<double> PP, FF, FF_inv_trans;
    VectorValue<double> F, F_b, F_s, F_qp, n;
    Point X_qp;
    double P;
    blitz::Array<double,2> X_node;
    const MeshBase::const_element_iterator el_begin = mesh.active_local_elements_begin();
    const MeshBase::const_element_iterator el_end   = mesh.active_local_elements_end();
    for (MeshBase::const_element_iterator el_it = el_begin; el_it != el_end; ++el_it)
    {
        Elem* const elem = *el_it;

        fe->reinit(elem);
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            dof_map.dof_indices(elem, dof_indices(d), d);
            if (G_rhs_e[d].size() != dof_indices(d).size())
            {
                G_rhs_e[d].resize(dof_indices(d).size());  // NOTE: DenseVector::resize() automatically zeroes the vector contents.
            }
            else
            {
                G_rhs_e[d].zero();
            }
        }

        const unsigned int n_qp = qrule->n_points();
        const unsigned int n_basis = dof_indices(0).size();

        get_values_for_interpolation(X_node, X_vec, dof_indices);
        for (unsigned int qp = 0; qp < n_qp; ++qp)
        {
            const Point& s_qp = q_point[qp];
            interpolate(X_qp,qp,X_node,phi);
            jacobian(FF,qp,X_node,dphi);

            if (d_PK1_stress_fcns[part])
            {
                // Compute the value of the first Piola-Kirchhoff stress tensor
                // at the quadrature point and add the corresponding forces to
                // the right-hand-side vector.
                d_PK1_stress_fcns[part](PP,FF,X_qp,s_qp,elem,X_vec,PK1_stress_fcn_data,data_time,d_PK1_stress_fcn_ctxs[part]);
                for (unsigned int k = 0; k < n_basis; ++k)
                {
                    F_qp = -PP*dphi[k][qp]*JxW[qp];
                    for (unsigned int i = 0; i < NDIM; ++i)
                    {
                        G_rhs_e[i](k) += F_qp(i);
                    }
                }
            }

            if (d_lag_body_force_fcns[part])
            {
                // Compute the value of the body force at the quadrature point
                // and add the corresponding forces to the right-hand-side
                // vector.
                d_lag_body_force_fcns[part](F_b,FF,X_qp,s_qp,elem,X_vec,lag_body_force_fcn_data,data_time,d_lag_body_force_fcn_ctxs[part]);
                for (unsigned int k = 0; k < n_basis; ++k)
                {
                    F_qp = phi[k][qp]*JxW[qp]*F_b;
                    for (unsigned int i = 0; i < NDIM; ++i)
                    {
                        G_rhs_e[i](k) += F_qp(i);
                    }
                }
            }
        }

        // Loop over the element boundaries.
        for (unsigned short int side = 0; side < elem->n_sides(); ++side)
        {
            // Determine whether we are at a physical boundary and, if so,
            // whether it is a Dirichlet boundary.
            bool at_physical_bdry = !elem->neighbor(side);
            const std::vector<short int>& bdry_ids = mesh.boundary_info->boundary_ids(elem, side);
            for (std::vector<short int>::const_iterator cit = bdry_ids.begin(); cit != bdry_ids.end(); ++cit)
            {
                at_physical_bdry = at_physical_bdry && !dof_map.is_periodic_boundary(*cit);
            }
            const bool at_dirichlet_bdry = get_dirichlet_bdry_ids(bdry_ids) != 0;

            // Skip non-physical boundaries.
            if (!at_physical_bdry) continue;

            // Determine whether we need to compute surface forces along this
            // part of the physical boundary; if not, skip the present side.
            const bool compute_transmission_force = d_PK1_stress_fcns       [part] && (( d_split_forces && !at_dirichlet_bdry) ||
                                                                                               (!d_split_forces &&  at_dirichlet_bdry));
            const bool compute_pressure           = d_lag_pressure_fcns     [part] && ( !d_split_forces && !at_dirichlet_bdry );
            const bool compute_surface_force      = d_lag_surface_force_fcns[part] && ( !d_split_forces && !at_dirichlet_bdry );
            if (!(compute_transmission_force || compute_pressure || compute_surface_force)) continue;

            fe_face->reinit(elem, side);

            const unsigned int n_qp = qrule_face->n_points();
            const unsigned int n_basis = dof_indices(0).size();

            get_values_for_interpolation(X_node, X_vec, dof_indices);
            for (unsigned int qp = 0; qp < n_qp; ++qp)
            {
                const Point& s_qp = q_point_face[qp];
                interpolate(X_qp,qp,X_node,phi_face);
                jacobian(FF,qp,X_node,dphi_face);
                const double J = std::abs(FF.det());
                tensor_inverse_transpose(FF_inv_trans,FF,NDIM);
                F.zero();
                if (compute_transmission_force)
                {
                    // Compute the value of the first Piola-Kirchhoff stress
                    // tensor at the quadrature point and add the corresponding
                    // force to the right-hand-side vector.
                    d_PK1_stress_fcns[part](PP,FF,X_qp,s_qp,elem,X_vec,PK1_stress_fcn_data,data_time,d_PK1_stress_fcn_ctxs[part]);
                    F += PP*normal_face[qp];
                }
                if (compute_pressure && d_lag_pressure_fcns[part])
                {
                    // Compute the value of the pressure at the quadrature point
                    // and add the corresponding force to the right-hand-side
                    // vector.
                    d_lag_pressure_fcns[part](P,FF,X_qp,s_qp,elem,side,X_vec,lag_pressure_fcn_data,data_time,d_lag_pressure_fcn_ctxs[part]);
                    F -= P*J*FF_inv_trans*normal_face[qp];
                }
                if (compute_surface_force)
                {
                    // Compute the value of the surface force at the quadrature
                    // point and add the corresponding force to the
                    // right-hand-side vector.
                    d_lag_surface_force_fcns[part](F_s,FF,X_qp,s_qp,elem,side,X_vec,lag_surface_force_fcn_data,data_time,d_lag_surface_force_fcn_ctxs[part]);
                    F += F_s;
                }

                // If we are imposing jump conditions, then we keep only the
                // normal part of the force.  This has the effect of projecting
                // the tangential part of the surface force onto the interior
                // force density.
                if (d_split_forces && d_use_jump_conditions && !at_dirichlet_bdry)
                {
                    n = (FF_inv_trans*normal_face[qp]).unit();
                    F = (F*n)*n;
                }

                // Add the boundary forces to the right-hand-side vector.
                for (unsigned int k = 0; k < n_basis; ++k)
                {
                    F_qp = phi_face[k][qp]*JxW_face[qp]*F;
                    for (unsigned int i = 0; i < NDIM; ++i)
                    {
                        G_rhs_e[i](k) += F_qp(i);
                    }
                }
            }
        }

        // Apply constraints (e.g., enforce periodic boundary conditions) and
        // add the elemental contributions to the global vector.
        for (unsigned int i = 0; i < NDIM; ++i)
        {
            dof_map.constrain_element_vector(G_rhs_e[i], dof_indices(i));
            G_rhs_vec->add_vector(G_rhs_e[i], dof_indices(i));
        }
    }

    // Solve for G.
    G_rhs_vec->close();
    d_fe_data_managers[part]->computeL2Projection(G_vec, *G_rhs_vec, FORCE_SYSTEM_NAME, d_use_consistent_mass_matrix);
    return;
}// computeInteriorForceDensity

void
IBFEMethod::spreadTransmissionForceDensity(
    const int f_data_idx,
    PetscVector<double>& X_ghost_vec,
    const double data_time,
    const unsigned int part)
{
    if (!d_split_forces) return;

    // Extract the mesh.
    EquationSystems* equation_systems = d_fe_data_managers[part]->getEquationSystems();
    const MeshBase& mesh = equation_systems->get_mesh();
    const int dim = mesh.mesh_dimension();
    AutoPtr<QBase> qrule_face = QBase::build(QGAUSS, dim-1, FIRST);

    // Extract the FE systems and DOF maps, and setup the FE objects.
    System& system = equation_systems->get_system(FORCE_SYSTEM_NAME);
    const DofMap& dof_map = system.get_dof_map();
#ifdef DEBUG_CHECK_ASSERTIONS
    for (unsigned int d = 0; d < NDIM; ++d) TBOX_ASSERT(dof_map.variable_type(d) == dof_map.variable_type(0));
#endif
    blitz::Array<std::vector<unsigned int>,1> dof_indices(NDIM);
    for (unsigned int d = 0; d < NDIM; ++d) dof_indices(d).reserve(27);
    blitz::Array<std::vector<unsigned int>,1> side_dof_indices(NDIM);
    for (unsigned int d = 0; d < NDIM; ++d) side_dof_indices(d).reserve(9);
    AutoPtr<FEBase> fe_face(FEBase::build(dim, dof_map.variable_type(0)));
    fe_face->attach_quadrature_rule(qrule_face.get());
    const std::vector<Point>& q_point_face = fe_face->get_xyz();
    const std::vector<double>& JxW_face = fe_face->get_JxW();
    const std::vector<Point>& normal_face = fe_face->get_normals();
    const std::vector<std::vector<double> >& phi_face = fe_face->get_phi();
    const std::vector<std::vector<VectorValue<double> > >& dphi_face = fe_face->get_dphi();

#ifdef DEBUG_CHECK_ASSERTIONS
    System& X_system = equation_systems->get_system(COORDS_SYSTEM_NAME);
    const DofMap& X_dof_map = X_system.get_dof_map();
    for (unsigned int d = 0; d < NDIM; ++d) TBOX_ASSERT(dof_map.variable_type(d) == X_dof_map.variable_type(d));
#endif

    // Setup extra data needed to compute stresses/forces.
    std::vector<NumericVector<double>*> PK1_stress_fcn_data;
    for (std::vector<unsigned int>::const_iterator cit = d_PK1_stress_fcn_systems[part].begin(); cit != d_PK1_stress_fcn_systems[part].end(); ++cit)
    {
        System& system = equation_systems->get_system(*cit);
        PK1_stress_fcn_data.push_back(d_fe_data_managers[part]->buildGhostedSolutionVector(system.name()));
    }

    std::vector<NumericVector<double>*> lag_pressure_fcn_data;
    for (std::vector<unsigned int>::const_iterator cit = d_lag_pressure_fcn_systems[part].begin(); cit != d_lag_pressure_fcn_systems[part].end(); ++cit)
    {
        System& system = equation_systems->get_system(*cit);
        lag_pressure_fcn_data.push_back(d_fe_data_managers[part]->buildGhostedSolutionVector(system.name()));
    }

    std::vector<NumericVector<double>*> lag_surface_force_fcn_data;
    for (std::vector<unsigned int>::const_iterator cit = d_lag_surface_force_fcn_systems[part].begin(); cit != d_lag_surface_force_fcn_systems[part].end(); ++cit)
    {
        System& system = equation_systems->get_system(*cit);
        lag_surface_force_fcn_data.push_back(d_fe_data_managers[part]->buildGhostedSolutionVector(system.name()));
    }

    // Loop over the patches to spread the transmission elastic force density
    // onto the grid.
    const blitz::Array<blitz::Array<Elem*,1>,1>& active_patch_element_map = d_fe_data_managers[part]->getActivePatchElementMap();
    const int level_num = d_fe_data_managers[part]->getLevelNumber();
    TensorValue<double> PP, FF, FF_inv_trans;
    VectorValue<double> F, F_s;
    Point X_qp;
    double P;
    std::vector<Point> elem_X;
    blitz::Array<double,2> X_node, X_node_side;
    Pointer<PatchLevel<NDIM> > level = d_hierarchy->getPatchLevel(level_num);
    int local_patch_num = 0;
    for (PatchLevel<NDIM>::Iterator p(level); p; p++, ++local_patch_num)
    {
        // The relevant collection of elements.
        const blitz::Array<Elem*,1>& patch_elems = active_patch_element_map(local_patch_num);
        const int num_active_patch_elems = patch_elems.size();
        if (num_active_patch_elems == 0) continue;

        Pointer<Patch<NDIM> > patch = level->getPatch(p());
        const Pointer<CartesianPatchGeometry<NDIM> > patch_geom = patch->getPatchGeometry();
        const double* const patch_dx = patch_geom->getDx();
        const double patch_dx_min = *std::min_element(patch_dx, patch_dx+NDIM);

        // Setup vectors to store the values of T and X at the quadrature
        // points.  We compute a somewhat conservative upper bound on the number
        // of quadrature points to try to avoid reallocations.
        static const unsigned int n_qp_estimate = (NDIM == 2 ? 12 : 12*12);
        std::vector<double> T_bdry;
        T_bdry.reserve(NDIM*n_qp_estimate*num_active_patch_elems);
        std::vector<double> X_bdry;
        X_bdry.reserve(NDIM*n_qp_estimate*num_active_patch_elems);

        // Loop over the elements and compute the values to be spread and the
        // positions of the quadrature points.
        int qp_offset = 0;
        for (int e_idx = 0; e_idx < num_active_patch_elems; ++e_idx)
        {
            Elem* const elem = patch_elems(e_idx);

            bool has_physical_boundaries = false;
            for (unsigned short int side = 0; side < elem->n_sides(); ++side)
            {
                bool at_physical_bdry = !elem->neighbor(side);
                const std::vector<short int>& bdry_ids = mesh.boundary_info->boundary_ids(elem, side);
                for (std::vector<short int>::const_iterator cit = bdry_ids.begin(); cit != bdry_ids.end(); ++cit)
                {
                    at_physical_bdry = at_physical_bdry && !dof_map.is_periodic_boundary(*cit);
                }
                has_physical_boundaries = has_physical_boundaries || at_physical_bdry;
            }
            if (!has_physical_boundaries) continue;

            for (unsigned int d = 0; d < NDIM; ++d)
            {
                dof_map.dof_indices(elem, dof_indices(d), d);
            }
            get_values_for_interpolation(X_node, X_ghost_vec, dof_indices);

            // Loop over the element boundaries.
            for (unsigned short int side = 0; side < elem->n_sides(); ++side)
            {
                // Determine whether we are at a physical boundary and, if so,
                // whether it is a Dirichlet boundary.
                bool at_physical_bdry = !elem->neighbor(side);
                const std::vector<short int>& bdry_ids = mesh.boundary_info->boundary_ids(elem, side);
                for (std::vector<short int>::const_iterator cit = bdry_ids.begin(); cit != bdry_ids.end(); ++cit)
                {
                    at_physical_bdry = at_physical_bdry && !dof_map.is_periodic_boundary(*cit);
                }
                const bool at_dirichlet_bdry = get_dirichlet_bdry_ids(bdry_ids) != 0;

                // Skip non-physical boundaries.
                if (!at_physical_bdry) continue;

                // Determine whether we need to compute surface forces along
                // this part of the physical boundary; if not, skip the present
                // side.
                const bool compute_transmission_force = d_PK1_stress_fcns       [part] && !at_dirichlet_bdry;
                const bool compute_pressure           = d_lag_pressure_fcns     [part] && !at_dirichlet_bdry;
                const bool compute_surface_force      = d_lag_surface_force_fcns[part] && !at_dirichlet_bdry;
                if (!(compute_transmission_force || compute_pressure || compute_surface_force)) continue;

                AutoPtr<Elem> side_elem = elem->build_side(side);
                for (unsigned int d = 0; d < NDIM; ++d)
                {
                    dof_map.dof_indices(side_elem.get(), side_dof_indices(d), d);
                }
                get_values_for_interpolation(X_node_side, X_ghost_vec, side_dof_indices);
                const double hmax = get_elem_hmax(side_elem.get(), X_node_side);
                const int npts = std::max(elem->default_order() == FIRST ? 1.0 : 2.0,
                                          std::ceil(POINT_FACTOR*hmax/patch_dx_min));
                const Order order = static_cast<Order>(std::min(2*npts-1,static_cast<int>(FORTYTHIRD)));
                if (order != qrule_face->get_order())
                {
                    qrule_face = QBase::build(QGAUSS, dim-1, order);
                    fe_face->attach_quadrature_rule(qrule_face.get());
                }
                fe_face->reinit(elem, side);

                const unsigned int n_qp = qrule_face->n_points();

                T_bdry.resize(T_bdry.size()+NDIM*n_qp);
                X_bdry.resize(X_bdry.size()+NDIM*n_qp);

                for (unsigned int qp = 0; qp < n_qp; ++qp, ++qp_offset)
                {
                    const Point& s_qp = q_point_face[qp];
                    interpolate(X_qp,qp,X_node,phi_face);
                    jacobian(FF,qp,X_node,dphi_face);
                    const double J = std::abs(FF.det());
                    tensor_inverse_transpose(FF_inv_trans,FF,NDIM);
                    F.zero();
                    if (compute_transmission_force)
                    {
                        // Compute the value of the first Piola-Kirchhoff stress
                        // tensor at the quadrature point and compute the
                        // corresponding force.
                        d_PK1_stress_fcns[part](PP,FF,X_qp,s_qp,elem,X_ghost_vec,PK1_stress_fcn_data,data_time,d_PK1_stress_fcn_ctxs[part]);
                        F -= PP*normal_face[qp]*JxW_face[qp];
                    }
                    if (compute_pressure)
                    {
                        // Compute the value of the pressure at the quadrature
                        // point and compute the corresponding force.
                        d_lag_pressure_fcns[part](P,FF,X_qp,s_qp,elem,side,X_ghost_vec,lag_pressure_fcn_data,data_time,d_lag_pressure_fcn_ctxs[part]);
                        F -= P*J*FF_inv_trans*normal_face[qp]*JxW_face[qp];
                    }
                    if (compute_surface_force)
                    {
                        // Compute the value of the surface force at the
                        // quadrature point and compute the corresponding force.
                        d_lag_surface_force_fcns[part](F_s,FF,X_qp,s_qp,elem,side,X_ghost_vec,lag_surface_force_fcn_data,data_time,d_lag_surface_force_fcn_ctxs[part]);
                        F += F_s*JxW_face[qp];
                    }
                    const int idx = NDIM*qp_offset;
                    for (unsigned int i = 0; i < NDIM; ++i)
                    {
                        T_bdry[idx+i] = F(i);
                    }
                    for (unsigned int i = 0; i < NDIM; ++i)
                    {
                        X_bdry[idx+i] = X_qp(i);
                    }
                }
            }
        }

        if (qp_offset == 0) continue;

        // Spread the boundary forces to the grid.
        const std::string& spread_weighting_fcn = d_fe_data_managers[part]->getSpreadWeightingFunction();
        const hier::IntVector<NDIM>& ghost_width = d_fe_data_managers[part]->getGhostCellWidth();
        const Box<NDIM> spread_box = Box<NDIM>::grow(patch->getBox(), ghost_width);
        Pointer<SideData<NDIM,double> > f_data = patch->getPatchData(f_data_idx);
        LEInteractor::spread(f_data, T_bdry, NDIM, X_bdry, NDIM, patch, spread_box, spread_weighting_fcn);
    }
    return;
}// spreadTransmissionForceDensity

void
IBFEMethod::imposeJumpConditions(
    const int f_data_idx,
    PetscVector<double>& F_ghost_vec,
    PetscVector<double>& X_ghost_vec,
    const double data_time,
    const unsigned int part)
{
    if (!d_split_forces) return;

    // Extract the mesh.
    EquationSystems* equation_systems = d_fe_data_managers[part]->getEquationSystems();
    const MeshBase& mesh = equation_systems->get_mesh();
    const int dim = mesh.mesh_dimension();

    // Extract the FE systems and DOF maps, and setup the FE objects.
    System& system = equation_systems->get_system(FORCE_SYSTEM_NAME);
    const DofMap& dof_map = system.get_dof_map();
#ifdef DEBUG_CHECK_ASSERTIONS
    for (unsigned int d = 0; d < NDIM; ++d) TBOX_ASSERT(dof_map.variable_type(d) == dof_map.variable_type(0));
#endif
    blitz::Array<std::vector<unsigned int>,1> dof_indices(NDIM);
    for (unsigned int d = 0; d < NDIM; ++d) dof_indices(d).reserve(27);
    blitz::Array<std::vector<unsigned int>,1> side_dof_indices(NDIM);
    for (unsigned int d = 0; d < NDIM; ++d) side_dof_indices(d).reserve(9);
    AutoPtr<FEBase> fe_face(FEBase::build(dim, dof_map.variable_type(0)));
    const std::vector<Point>& q_point_face = fe_face->get_xyz();
    const std::vector<Point>& normal_face = fe_face->get_normals();
    const std::vector<std::vector<double> >& phi_face = fe_face->get_phi();
    const std::vector<std::vector<VectorValue<double> > >& dphi_face = fe_face->get_dphi();

#ifdef DEBUG_CHECK_ASSERTIONS
    System& X_system = equation_systems->get_system(COORDS_SYSTEM_NAME);
    const DofMap& X_dof_map = X_system.get_dof_map();
    for (unsigned int d = 0; d < NDIM; ++d) TBOX_ASSERT(dof_map.variable_type(d) == X_dof_map.variable_type(d));
#endif

    // Setup extra data needed to compute stresses/forces.
    std::vector<NumericVector<double>*> PK1_stress_fcn_data;
    for (std::vector<unsigned int>::const_iterator cit = d_PK1_stress_fcn_systems[part].begin(); cit != d_PK1_stress_fcn_systems[part].end(); ++cit)
    {
        System& system = equation_systems->get_system(*cit);
        PK1_stress_fcn_data.push_back(d_fe_data_managers[part]->buildGhostedSolutionVector(system.name()));
    }

    std::vector<NumericVector<double>*> lag_pressure_fcn_data;
    for (std::vector<unsigned int>::const_iterator cit = d_lag_pressure_fcn_systems[part].begin(); cit != d_lag_pressure_fcn_systems[part].end(); ++cit)
    {
        System& system = equation_systems->get_system(*cit);
        lag_pressure_fcn_data.push_back(d_fe_data_managers[part]->buildGhostedSolutionVector(system.name()));
    }

    std::vector<NumericVector<double>*> lag_surface_force_fcn_data;
    for (std::vector<unsigned int>::const_iterator cit = d_lag_surface_force_fcn_systems[part].begin(); cit != d_lag_surface_force_fcn_systems[part].end(); ++cit)
    {
        System& system = equation_systems->get_system(*cit);
        lag_surface_force_fcn_data.push_back(d_fe_data_managers[part]->buildGhostedSolutionVector(system.name()));
    }

    // Loop over the patches to impose jump conditions on the Eulerian grid that
    // are determined from the interior and transmission elastic force
    // densities.
    const blitz::Array<blitz::Array<Elem*,1>,1>& active_patch_element_map = d_fe_data_managers[part]->getActivePatchElementMap();
    const int level_num = d_fe_data_managers[part]->getLevelNumber();
    TensorValue<double> PP, FF, FF_inv_trans;
    VectorValue<double> F, F_s, F_qp, n;
    Point X_qp;
    double P;
    blitz::Array<double,2> F_node, X_node;
    static const unsigned int MAX_NODES = (NDIM == 2 ? 9 : 27);
    Point s_node_cache[MAX_NODES], X_node_cache[MAX_NODES];
    blitz::TinyVector<double,NDIM> X_min, X_max;
    Pointer<PatchLevel<NDIM> > level = d_hierarchy->getPatchLevel(level_num);
    int local_patch_num = 0;
    for (PatchLevel<NDIM>::Iterator p(level); p; p++, ++local_patch_num)
    {
        // The relevant collection of elements.
        const blitz::Array<Elem*,1>& patch_elems = active_patch_element_map(local_patch_num);
        const int num_active_patch_elems = patch_elems.size();
        if (num_active_patch_elems == 0) continue;

        const Pointer<Patch<NDIM> > patch = level->getPatch(p());
        Pointer<SideData<NDIM,double> > f_data = patch->getPatchData(f_data_idx);
        const Box<NDIM>& patch_box = patch->getBox();
        const CellIndex<NDIM>& patch_lower = patch_box.lower();
        const CellIndex<NDIM>& patch_upper = patch_box.upper();
        const Pointer<CartesianPatchGeometry<NDIM> > patch_geom = patch->getPatchGeometry();
        const double* const x_lower = patch_geom->getXLower();
        const double* const x_upper = patch_geom->getXUpper();
        const double* const dx = patch_geom->getDx();

        blitz::TinyVector<Box<NDIM>,NDIM> side_boxes;
        for (unsigned int axis = 0; axis < NDIM; ++axis)
        {
            side_boxes[axis] = SideGeometry<NDIM>::toSideBox(patch_box,axis);
        }

        // Loop over the elements.
        for (int e_idx = 0; e_idx < num_active_patch_elems; ++e_idx)
        {
            Elem* const elem = patch_elems(e_idx);

            bool has_physical_boundaries = false;
            for (unsigned short int side = 0; side < elem->n_sides(); ++side)
            {
                bool at_physical_bdry = !elem->neighbor(side);
                const std::vector<short int>& bdry_ids = mesh.boundary_info->boundary_ids(elem, side);
                for (std::vector<short int>::const_iterator cit = bdry_ids.begin(); cit != bdry_ids.end(); ++cit)
                {
                    at_physical_bdry = at_physical_bdry && !dof_map.is_periodic_boundary(*cit);
                }
                has_physical_boundaries = has_physical_boundaries || at_physical_bdry;
            }
            if (!has_physical_boundaries) continue;

            for (unsigned int d = 0; d < NDIM; ++d)
            {
                dof_map.dof_indices(elem, dof_indices(d), d);
            }

            // Loop over the element boundaries.
            for (unsigned short int side = 0; side < elem->n_sides(); ++side)
            {
                // Determine whether we are at a physical boundary and, if so,
                // whether it is a Dirichlet boundary.
                bool at_physical_bdry = !elem->neighbor(side);
                const std::vector<short int>& bdry_ids = mesh.boundary_info->boundary_ids(elem, side);
                for (std::vector<short int>::const_iterator cit = bdry_ids.begin(); cit != bdry_ids.end(); ++cit)
                {
                    at_physical_bdry = at_physical_bdry && !dof_map.is_periodic_boundary(*cit);
                }
                const bool at_dirichlet_bdry = get_dirichlet_bdry_ids(bdry_ids) != 0;

                // Skip non-physical boundaries.
                if (!at_physical_bdry) continue;

                // Determine whether we need to compute surface forces along
                // this part of the physical boundary; if not, skip the present
                // side.
                const bool compute_transmission_force = d_PK1_stress_fcns       [part] && (( d_split_forces && !at_dirichlet_bdry) ||
                                                                                                   (!d_split_forces &&  at_dirichlet_bdry));
                const bool compute_pressure           = d_lag_pressure_fcns     [part] && ( !d_split_forces && !at_dirichlet_bdry );
                const bool compute_surface_force      = d_lag_surface_force_fcns[part] && ( !d_split_forces && !at_dirichlet_bdry );
                if (!(compute_transmission_force || compute_pressure || compute_surface_force)) continue;

                // Construct a side element.
                AutoPtr<Elem> side_elem = elem->build_side(side);
                const unsigned int n_node_side = side_elem->n_nodes();
                for (int d = 0; d < NDIM; ++d)
                {
                    dof_map.dof_indices(side_elem.get(), side_dof_indices(d), d);
                }

                // Cache the nodal and physical coordinates of the side element,
                // determine the bounding box of the current configuration of
                // the side element, and set the nodal coordinates to correspond
                // to the physical coordinates.
#ifdef DEBUG_CHECK_ASSERTIONS
                TBOX_ASSERT(n_node_side <= MAX_NODES);
#endif
                X_min =  0.5*std::numeric_limits<double>::max();
                X_max = -0.5*std::numeric_limits<double>::max();
                for (unsigned int k = 0; k < n_node_side; ++k)
                {
                    s_node_cache[k] = side_elem->point(k);
                    for (unsigned int d = 0; d < NDIM; ++d)
                    {
                        X_node_cache[k](d) = X_ghost_vec(side_dof_indices(d)[k]);
                        X_min[d] = std::min(X_min[d],X_node_cache[k](d));
                        X_max[d] = std::max(X_max[d],X_node_cache[k](d));
                    }
                    side_elem->point(k) = X_node_cache[k];
                }

                // Loop over coordinate directions and look for intersections
                // with the background fluid grid.
                std::vector<Point> intersection_ref_points;
                std::vector<int>   intersection_axes;
                static const int estimated_max_size = (NDIM == 2 ? 64 : 512);
                intersection_ref_points.reserve(estimated_max_size);
                intersection_axes      .reserve(estimated_max_size);
                for (unsigned int axis = 0; axis < NDIM; ++axis)
                {
                    // Setup a unit vector pointing in the coordinate direction
                    // of interest.
                    VectorValue<double> q;
                    q(axis) = 1.0;

                    // Loop over the relevant range of indices.
                    blitz::TinyVector<int,NDIM> i_begin, i_end, ic;
                    for (unsigned int d = 0; d < NDIM; ++d)
                    {
                        if (d == axis)
                        {
                            i_begin[d] = 0;
                            i_end  [d] = 1;
                        }
                        else
                        {
                            i_begin[d] = std::ceil((X_min[d]-x_lower[d])/dx[d] - 0.5 - 1.0) + patch_lower[d];  // NOTE: added "safety factor" of one grid cell to range of indices
                            i_end  [d] = std::ceil((X_max[d]-x_lower[d])/dx[d] - 0.5 + 1.0) + patch_lower[d];
                        }
                    }
#if (NDIM == 3)
                    for (ic[2] = i_begin[2]; ic[2] < i_end[2]; ++ic[2])
                    {
#endif
                        for (ic[1] = i_begin[1]; ic[1] < i_end[1]; ++ic[1])
                        {
                            for (ic[0] = i_begin[0]; ic[0] < i_end[0]; ++ic[0])
                            {
                                Point r;
                                for (unsigned int d = 0; d < NDIM; ++d)
                                {
                                    r(d) = (d == axis ? 0.0 : x_lower[d] + dx[d]*(static_cast<double>(ic[d]-patch_lower[d])+0.5));
                                }
#if (NDIM == 2)
                                std::vector<std::pair<double,Point> > intersections = intersect_line_with_edge(dynamic_cast<Edge*>(side_elem.get()), r, q);
#endif
#if (NDIM == 3)
                                std::vector<std::pair<double,Point> > intersections = intersect_line_with_face(dynamic_cast<Face*>(side_elem.get()), r, q);
#endif
                                for (unsigned int k = 0; k < intersections.size(); ++k)
                                {
                                    intersection_ref_points.push_back(intersections[k].second);
                                    intersection_axes.push_back(axis);
                                }
                            }
                        }
#if (NDIM == 3)
                    }
#endif
                }

                // Restore the element coordinates.
                for (unsigned int k = 0; k < n_node_side; ++k)
                {
                    side_elem->point(k) = s_node_cache[k];
                }

                // If there are no intersection points, then continue to the
                // next side.
                if (intersection_ref_points.empty()) continue;

                // Evaluate the jump conditions and apply them to the Eulerian
                // grid.
                fe_face->reinit(elem, side, TOLERANCE, &intersection_ref_points);

                if (!d_use_IB_spread_operator)
                {
                    get_values_for_interpolation(F_node, F_ghost_vec, dof_indices);
                }
                get_values_for_interpolation(X_node, X_ghost_vec, dof_indices);

                for (unsigned int qp = 0; qp < intersection_ref_points.size(); ++qp)
                {
                    const unsigned int axis = intersection_axes[qp];
                    interpolate(X_qp,qp,X_node,phi_face);
                    Index<NDIM> i = IndexUtilities::getCellIndex(&X_qp(0), x_lower, x_upper, dx, patch_lower, patch_upper);
                    if (X_qp(axis) > x_lower[axis] + static_cast<double>(i(axis)-patch_lower[axis]+0.5)*dx[axis])
                    {
                        ++i(axis);
                    }
#ifdef DEBUG_CHECK_ASSERTIONS
                    for (unsigned int d = 0; d < NDIM; ++d)
                    {
                        if (d == axis)
                        {
                            const double X_lower_bound = x_lower[d]+(static_cast<double>(i(d)-patch_lower[d])-0.5)*dx[d]-sqrt(std::numeric_limits<double>::epsilon());
                            const double X_upper_bound = x_lower[d]+(static_cast<double>(i(d)-patch_lower[d])+0.5)*dx[d]+sqrt(std::numeric_limits<double>::epsilon());
                            TBOX_ASSERT(X_lower_bound <= X_qp(d) && X_upper_bound >= X_qp(d));
                        }
                        else
                        {
                            const double X_intersection = x_lower[d]+(static_cast<double>(i(d)-patch_lower[d])+0.5)*dx[d];
                            const double X_interp = X_qp(d);
                            const double rel_diff = std::abs(X_intersection-X_interp)/std::max(1.0,std::max(std::abs(X_intersection),std::abs(X_interp)));
                            TBOX_ASSERT(rel_diff <= sqrt(std::numeric_limits<double>::epsilon()));
                        }
                    }
#endif
                    const int d = axis;
                    const SideIndex<NDIM> s_i(i,d,0);
                    if (!side_boxes[d].contains(s_i)) continue;
                    const Point& s_qp = q_point_face[qp];
                    interpolate(X_qp,qp,X_node,phi_face);
                    jacobian(FF,qp,X_node,dphi_face);
                    const double J = std::abs(FF.det());
                    tensor_inverse_transpose(FF_inv_trans,FF,NDIM);
                    F.zero();
                    if (compute_transmission_force)
                    {
                        // Compute the value of the first Piola-Kirchhoff stress
                        // tensor at the quadrature point and compute the
                        // corresponding force.
                        d_PK1_stress_fcns[part](PP,FF,X_qp,s_qp,elem,X_ghost_vec,PK1_stress_fcn_data,data_time,d_PK1_stress_fcn_ctxs[part]);
                        F -= PP*normal_face[qp];
                    }
                    if (compute_pressure)
                    {
                        // Compute the value of the pressure at the quadrature
                        // point and compute the corresponding force.
                        d_lag_pressure_fcns[part](P,FF,X_qp,s_qp,elem,side,X_ghost_vec,lag_pressure_fcn_data,data_time,d_lag_pressure_fcn_ctxs[part]);
                        F -= P*J*FF_inv_trans*normal_face[qp];
                    }
                    if (compute_surface_force)
                    {
                        // Compute the value of the surface force at the
                        // quadrature point and compute the corresponding force.
                        d_lag_surface_force_fcns[part](F_s,FF,X_qp,s_qp,elem,side,X_ghost_vec,lag_surface_force_fcn_data,data_time,d_lag_surface_force_fcn_ctxs[part]);
                        F += F_s;
                    }

                    // Use Nanson's formula (n da = J FF^{-T} N dA) to convert
                    // force per unit area in the reference configuration into
                    // force per unit area in the current configuration.  This
                    // value determines the discontinuity in the pressure at the
                    // fluid-structure interface.
                    n = (FF_inv_trans*normal_face[qp]).unit();
                    const double dA_da = 1.0/(J*(FF_inv_trans*normal_face[qp])*n);
                    F *= dA_da;

                    // Determine the value of the interior force density at the
                    // boundary, and convert it to force per unit volume in the
                    // current configuration.  This value determines the
                    // discontinuity in the normal derivative of the pressure at
                    // the fluid-structure interface.
                    //
                    // NOTE: This additional correction appears to be
                    // ineffective when we use "diffuse" force spreading; hence,
                    // we compute it only when we do NOT use the IB/FE version
                    // of the IB force spreading operator.
                    if (d_use_IB_spread_operator)
                    {
                        F_qp.zero();
                    }
                    else
                    {
                        interpolate(F_qp,qp,F_node,phi_face);
                        F_qp /= J;
                    }

                    // Impose the jump conditions.
                    const double X = X_qp(d);
                    const double x_cell_bdry = x_lower[d]+static_cast<double>(i(d)-patch_lower[d])*dx[d];
                    const double h = x_cell_bdry + (X > x_cell_bdry ? +0.5 : -0.5)*dx[d] - X;
                    const double C_p = F*n - h*F_qp(d);
                    (*f_data)(s_i) += (n(d) > 0.0 ? +1.0 : -1.0)*(C_p/dx[d]);
                }
            }
        }
    }
    return;
}// imposeJumpConditions

void
IBFEMethod::initializeCoordinates(
    const unsigned int part)
{
    EquationSystems* equation_systems = d_fe_data_managers[part]->getEquationSystems();
    MeshBase& mesh = equation_systems->get_mesh();
    System& X_system = equation_systems->get_system(COORDS_SYSTEM_NAME);
    const unsigned int X_sys_num = X_system.number();
    NumericVector<double>& X_coords = *X_system.solution;
    const bool identity_mapping = !d_coordinate_mapping_fcns[part];
    for (MeshBase::node_iterator it = mesh.local_nodes_begin(); it != mesh.local_nodes_end(); ++it)
    {
        Node* n = *it;
        if (n->n_vars(X_sys_num) > 0)
        {
            libmesh_assert(n->n_vars(X_sys_num) == NDIM);
            const Point& s = *n;
            Point X = s;
            if (!identity_mapping)
            {
                d_coordinate_mapping_fcns[part](X, s, d_coordinate_mapping_fcn_ctxs[part]);
            }
            for (unsigned int d = 0; d < NDIM; ++d)
            {
                const int dof_index = n->dof_number(X_sys_num,d,0);
                X_coords.set(dof_index,X(d));
            }
        }
    }
    X_system.get_dof_map().enforce_constraints_exactly(X_system, &X_coords);
    X_coords.close();
    return;
}// initializeCoordinates

void
IBFEMethod::updateCoordinateMapping(
    const unsigned int part)
{
    EquationSystems* equation_systems = d_fe_data_managers[part]->getEquationSystems();
    MeshBase& mesh = equation_systems->get_mesh();
    System& X_system = equation_systems->get_system(COORDS_SYSTEM_NAME);
    const unsigned int X_sys_num = X_system.number();
    NumericVector<double>& X_coords = *X_system.solution;
    System& X_mapping_system = equation_systems->get_system(COORD_MAPPING_SYSTEM_NAME);
    const unsigned int X_mapping_sys_num = X_mapping_system.number();
    NumericVector<double>& dX_coords = *X_mapping_system.solution;
    for (MeshBase::node_iterator it = mesh.local_nodes_begin(); it != mesh.local_nodes_end(); ++it)
    {
        Node* n = *it;
        if (n->n_vars(X_sys_num) > 0)
        {
            libmesh_assert(n->n_vars(X_sys_num) == NDIM);
            libmesh_assert(n->n_vars(X_mapping_sys_num) == NDIM);
            const Point& s = *n;
            for (unsigned int d = 0; d < NDIM; ++d)
            {
                const int X_dof_index = n->dof_number(X_sys_num,d,0);
                const int dX_dof_index = n->dof_number(X_mapping_sys_num,d,0);
                dX_coords.set(dX_dof_index,X_coords(X_dof_index)-s(d));
            }
        }
    }
    dX_coords.close();
    return;
}// updateCoordinateMapping

/////////////////////////////// PRIVATE //////////////////////////////////////

void
IBFEMethod::commonConstructor(
    const std::string& object_name,
    Pointer<Database> input_db,
    const std::vector<libMesh::Mesh*>& meshes,
    int max_level_number,
    bool register_for_restart)
{
    // Set the object name and register it with the restart manager.
    d_object_name = object_name;
    d_registered_for_restart = false;
    if (register_for_restart)
    {
        RestartManager::getManager()->registerRestartItem(d_object_name, this);
        d_registered_for_restart = true;
    }

    // Set some default values.
    d_use_IB_spread_operator = true;
    d_spread_delta_fcn = "IB_4";
    d_use_IB_interp_operator = true;
    d_interp_delta_fcn = "IB_4";
    d_ghosts = 0;
    d_split_forces = false;
    d_use_jump_conditions = false;
    d_use_consistent_mass_matrix = true;
    d_fe_family = LAGRANGE;
    d_fe_order = INVALID_ORDER;
    d_quad_type = QGAUSS;
    d_quad_order = FIFTH;
    d_do_log = false;

    // Indicate that all of the parts are unconstrained by default and set some
    // default values.
    d_has_constrained_parts = false;
    d_constrained_part.resize(d_num_parts,false);
    d_constrained_part_velocity_fcns.resize(d_num_parts,NULL);
    d_constrained_part_velocity_fcn_ctxs.resize(d_num_parts,NULL);
    d_constraint_omega = 2.0;

    // Initialize function pointers to NULL.
    d_coordinate_mapping_fcns.resize(d_num_parts,NULL);
    d_coordinate_mapping_fcn_ctxs.resize(d_num_parts,NULL);
    d_PK1_stress_fcns.resize(d_num_parts,NULL);
    d_PK1_stress_fcn_systems.resize(d_num_parts);
    d_PK1_stress_fcn_ctxs.resize(d_num_parts,NULL);
    d_lag_body_force_fcns.resize(d_num_parts,NULL);
    d_lag_body_force_fcn_systems.resize(d_num_parts);
    d_lag_body_force_fcn_ctxs.resize(d_num_parts,NULL);
    d_lag_pressure_fcns.resize(d_num_parts,NULL);
    d_lag_pressure_fcn_systems.resize(d_num_parts);
    d_lag_pressure_fcn_ctxs.resize(d_num_parts,NULL);
    d_lag_surface_force_fcns.resize(d_num_parts,NULL);
    d_lag_surface_force_fcn_systems.resize(d_num_parts);
    d_lag_surface_force_fcn_ctxs.resize(d_num_parts,NULL);

    // Determine whether we should use first-order or second-order shape
    // functions for each part of the structure.
    bool mesh_has_first_order_elems  = false;
    bool mesh_has_second_order_elems = false;
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        const MeshBase& mesh = *meshes[part];
        MeshBase::const_element_iterator       el_it  = mesh.elements_begin();
        const MeshBase::const_element_iterator el_end = mesh.elements_end();
        for ( ; el_it != el_end; ++el_it)
        {
            const Elem* const elem = *el_it;
            mesh_has_first_order_elems  = mesh_has_first_order_elems  || elem->default_order() == FIRST ;
            mesh_has_second_order_elems = mesh_has_second_order_elems || elem->default_order() == SECOND;
        }
    }
    mesh_has_first_order_elems  = SAMRAI_MPI::maxReduction(mesh_has_first_order_elems );
    mesh_has_second_order_elems = SAMRAI_MPI::maxReduction(mesh_has_second_order_elems);
    if (( mesh_has_first_order_elems &&  mesh_has_second_order_elems) ||
        (!mesh_has_first_order_elems && !mesh_has_second_order_elems))
    {
        TBOX_ERROR(d_object_name << "::IBFEMethod():\n"
                   << "  all parts of FE mesh must contain only FIRST order elements or only SECOND order elements" << std::endl);
    }
    if (mesh_has_first_order_elems)
    {
        d_fe_order = FIRST;
        d_quad_order = THIRD;
    }
    if (mesh_has_second_order_elems)
    {
        d_fe_order = SECOND;
        d_quad_order = FIFTH;
    }

    // Initialize object with data read from the input and restart databases.
    bool from_restart = RestartManager::getManager()->isFromRestart();
    if (from_restart) getFromRestart();
    if (input_db) getFromInput(input_db, from_restart);

    // Report configuration.
    pout << "\n";
    pout << d_object_name << ": using " << Utility::enum_to_string<Order>(d_fe_order) << " order " << Utility::enum_to_string<FEFamily>(d_fe_family) << " finite elements.\n";
    pout << "\n";

    // Check the choices for the delta function.
    if (d_interp_delta_fcn != d_spread_delta_fcn)
    {
        pout << "WARNING: different delta functions are being used for velocity interpolation and force spreading.\n"
             << "         recommended usage is to employ the same delta functions for both interpolation and spreading.\n";
    }

    // Create the FE data managers that manage mappings between the FE mesh
    // parts and the Cartesian grid.
    d_meshes = meshes;
    d_equation_systems.resize(d_num_parts, NULL);
    d_fe_data_managers.resize(d_num_parts, NULL);
    for (unsigned int part = 0; part < d_num_parts; ++part)
    {
        // Create FE data managers.
        std::ostringstream manager_stream;
        manager_stream << "IBFEMethod FEDataManager::" << part;
        const std::string& manager_name = manager_stream.str();
        d_fe_data_managers[part] = FEDataManager::getManager(manager_name, d_interp_delta_fcn, d_spread_delta_fcn, d_use_consistent_mass_matrix);
        d_ghosts = IntVector<NDIM>::max(d_ghosts,d_fe_data_managers[part]->getGhostCellWidth());

        // Create FE equation systems object.
        d_equation_systems[part] = new EquationSystems(*d_meshes[part]);
        EquationSystems* equation_systems = d_equation_systems[part];
        d_fe_data_managers[part]->setEquationSystems(equation_systems, max_level_number-1);
    }

    // Reset the current time step interval.
    d_current_time = std::numeric_limits<double>::quiet_NaN();
    d_new_time     = std::numeric_limits<double>::quiet_NaN();
    d_half_time    = std::numeric_limits<double>::quiet_NaN();

    d_is_initialized = false;
    return;
}// commonConstructor

void
IBFEMethod::getFromInput(
    Pointer<Database> db,
    bool is_from_restart)
{
    if (!is_from_restart)
    {
        if      (db->isBool("use_IB_spread_operator"   )) d_use_IB_spread_operator = db->getBool("use_IB_spread_operator"   );
        else if (db->isBool("use_IB_spreading_operator")) d_use_IB_spread_operator = db->getBool("use_IB_spreading_operator");
        if      (db->isString("spread_delta_fcn"   )) d_spread_delta_fcn = db->getString("spread_delta_fcn"   );
        else if (db->isString("spreading_delta_fcn")) d_spread_delta_fcn = db->getString("spreading_delta_fcn");
        else if (db->isString("delta_fcn"          )) d_spread_delta_fcn = db->getString("delta_fcn"          );

        if      (db->isBool("use_IB_interp_operator"       )) d_use_IB_interp_operator = db->getBool("use_IB_interp_operator"       );
        else if (db->isBool("use_IB_interpolation_operator")) d_use_IB_interp_operator = db->getBool("use_IB_interpolation_operator");
        if      (db->isString("interp_delta_fcn"       )) d_interp_delta_fcn = db->getString("interp_delta_fcn"       );
        else if (db->isString("interpolation_delta_fcn")) d_interp_delta_fcn = db->getString("interpolation_delta_fcn");
        else if (db->isString("delta_fcn"              )) d_interp_delta_fcn = db->getString("delta_fcn"              );

        if (db->isBool("split_forces")) d_split_forces = db->getBool("split_forces");
        if (db->isBool("use_jump_conditions")) d_use_jump_conditions = db->getBool("use_jump_conditions");
        if (db->isBool("use_consistent_mass_matrix")) d_use_consistent_mass_matrix = db->getBool("use_consistent_mass_matrix");
        if (db->isString("quad_type")) d_quad_type = Utility::string_to_enum<QuadratureType>(db->getString("quad_type"));
        if (db->isString("quad_order")) d_quad_order = Utility::string_to_enum<Order>(db->getString("quad_order"));

        if (db->isInteger("min_ghost_cell_width"))
        {
            d_ghosts = db->getInteger("min_ghost_cell_width");
        }
        else if (db->isDouble("min_ghost_cell_width"))
        {
            d_ghosts = static_cast<int>(std::ceil(db->getDouble("min_ghost_cell_width")));
        }
    }
    if      (db->keyExists("do_log"        )) d_do_log = db->getBool("do_log"        );
    else if (db->keyExists("enable_logging")) d_do_log = db->getBool("enable_logging");

    if (db->isDouble("constraint_omega")) d_constraint_omega = db->getDouble("constraint_omega");
    return;
}// getFromInput

void
IBFEMethod::getFromRestart()
{
    Pointer<Database> restart_db = RestartManager::getManager()->getRootDatabase();
    Pointer<Database> db;
    if (restart_db->isDatabase(d_object_name))
    {
        db = restart_db->getDatabase(d_object_name);
    }
    else
    {
        TBOX_ERROR(d_object_name << ":  Restart database corresponding to "
                   << d_object_name << " not found in restart file." << std::endl);
    }
    int ver = db->getInteger("IBFE_METHOD_VERSION");
    if (ver != IBFE_METHOD_VERSION)
    {
        TBOX_ERROR(d_object_name << ":  Restart file version different than class version." << std::endl);
    }
    db->getIntegerArray("d_ghosts", d_ghosts, NDIM);
    d_use_IB_spread_operator = db->getBool("d_use_IB_spread_operator");
    d_spread_delta_fcn = db->getString("d_spread_delta_fcn");
    d_use_IB_interp_operator = db->getBool("d_use_IB_interp_operator");
    d_interp_delta_fcn = db->getString("d_interp_delta_fcn");
    d_split_forces = db->getBool("d_split_forces");
    d_use_jump_conditions = db->getBool("d_use_jump_conditions");
    d_use_consistent_mass_matrix = db->getBool("d_use_consistent_mass_matrix");
    d_fe_family = Utility::string_to_enum<FEFamily>(db->getString("d_fe_family"));
    d_fe_order = Utility::string_to_enum<Order>(db->getString("d_fe_order"));
    d_quad_type = Utility::string_to_enum<QuadratureType>(db->getString("d_quad_type"));
    d_quad_order = Utility::string_to_enum<Order>(db->getString("d_quad_order"));
    return;
}// getFromRestart

/////////////////////////////// NAMESPACE ////////////////////////////////////

} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////
