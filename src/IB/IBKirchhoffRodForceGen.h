#ifndef included_IBKirchhoffRodForceGen
#define included_IBKirchhoffRodForceGen

// Filename: IBKirchhoffRodForceGen.h
// Last modified: <23.Jun.2010 15:43:34 griffith@boyce-griffiths-mac-pro.local>
// Created on 22 Jun 2010 by Boyce Griffith (griffith@boyce-griffiths-mac-pro.local)

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBAMR INCLUDES
#include <ibamr/IBLagrangianForceStrategy.h>

// PETSc INCLUDES
#include <petscmat.h>

// C++ STDLIB INCLUDES
#include <vector>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class IBKirchhoffRodForceGen is a concrete IBLagrangianForceStrategy
 * that computes the forces and torques generated by a collection of linear
 * elements based on Kirchhoff rod theory.
 *
 * \note Class IBKirchhoffRodForceGen DOES NOT correct for periodic
 * displacements of IB points.
 */
class IBKirchhoffRodForceGen
    : public IBLagrangianForceStrategy
{
public:
    /*!
     * \brief Default constructor.
     */
    IBKirchhoffRodForceGen(
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db=NULL);

    /*!
     * \brief Virtual destructor.
     */
    virtual
    ~IBKirchhoffRodForceGen();

    /*!
     * \brief Setup the data needed to compute the beam forces on the specified
     * level of the patch hierarchy.
     */
    virtual void
    initializeLevelData(
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double init_data_time,
        const bool initial_time,
        IBTK::LDataManager* const lag_manager);

    /*!
     * \brief Compute the curvilinear force and torque generated by the given
     * configuration of the curvilinear mesh.
     *
     * \note Nodal forces and moments computed by this method are \em added to
     * the force and moment vectors.
     */
    virtual void
    computeLagrangianForceAndTorque(
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> F_data,
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> N_data,
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> X_data,
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> D_data,
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> U_data,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

private:
    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBKirchhoffRodForceGen(
        const IBKirchhoffRodForceGen& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBKirchhoffRodForceGen&
    operator=(
        const IBKirchhoffRodForceGen& that);

    /*!
     * \brief Read input values, indicated above, from given database.
     *
     * The database pointer may be null.
     */
    void
    getFromInput(
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> db);

    /*!
     * \name Data maintained separately for each level of the patch hierarchy.
     */
    //\{
    std::vector<Mat> d_D_next_mats, d_X_next_mats;
    std::vector<std::vector<int> > d_petsc_curr_node_idxs, d_petsc_next_node_idxs;
    std::vector<std::vector<std::vector<double> > > d_material_params;
    std::vector<bool> d_is_initialized;
    //\}
};
}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibamr/IBKirchhoffRodForceGen.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBKirchhoffRodForceGen
