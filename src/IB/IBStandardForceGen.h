#ifndef included_IBStandardForceGen
#define included_IBStandardForceGen

// Filename: IBStandardForceGen.h
// Last modified: <29.Jul.2008 15:40:37 griffith@box230.cims.nyu.edu>
// Created on 03 May 2005 by Boyce Griffith (boyce@mstu1.cims.nyu.edu)

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBAMR INCLUDES
#include <ibamr/IBBeamForceGen.h>
#include <ibamr/IBLagrangianForceStrategy.h>
#include <ibamr/IBLagrangianForceStrategySet.h>
#include <ibamr/IBSpringForceGen.h>
#include <ibamr/IBTargetPointForceGen.h>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class IBStandardForceGen is a concrete IBLagrangianForceStrategy that
 * is intended to be used in conjunction with curvilinear mesh data generated by
 * class IBStandardInitializer.
 *
 * Class IBStandardForceGen provides support for linear and nonlinear spring
 * forces, linear beam forces, and target point penalty forces.
 *
 * \see IBSpringForceGen
 * \see IBBeamForceGen
 * \see IBTargetPointForceGen
 */
class IBStandardForceGen
    : public IBLagrangianForceStrategy
{
public:
    /*!
     * \brief Constructor.
     */
    IBStandardForceGen(
        SAMRAI::tbox::Pointer<IBSpringForceGen> spring_force_gen,
        SAMRAI::tbox::Pointer<IBBeamForceGen> beam_force_gen,
        SAMRAI::tbox::Pointer<IBTargetPointForceGen> target_point_force_gen);

    /*!
     * \brief Virtual destructor.
     */
    virtual
    ~IBStandardForceGen();

    /*!
     * \brief Setup the data needed to compute the forces on the specified level
     * of the patch hierarchy.
     */
    virtual void
    initializeLevelData(
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double init_data_time,
        const bool initial_time,
        IBTK::LDataManager* const lag_manager);

    /*!
     * \brief Compute the force generated by the Lagrangian structure on the
     * specified level of the patch hierarchy.
     *
     * \note Nodal forces computed by this method are \em added to the force
     * vector.
     */
    virtual void
    computeLagrangianForce(
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> F_data,
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> X_data,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

    /*!
     * \brief Compute the non-zero structure of the force Jacobian matrix.
     *
     * \note Elements indices must be global PETSc indices.
     */
    virtual void
    computeLagrangianForceJacobianNonzeroStructure(
        std::vector<int>& d_nnz,
        std::vector<int>& o_nnz,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

    /*!
     * \brief Compute the Jacobian of the force with respect to the present
     * structure configuration.
     *
     * \note The elements of the Jacobian should be "accumulated" in the
     * provided matrix J.
     */
    virtual void
    computeLagrangianForceJacobian(
        Mat& J_mat,
        MatAssemblyType assembly_type,
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> X_data,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

private:
    /*!
     * \brief Default constructor.
     *
     * \note This constructor is not implemented and should not be used.
     */
    IBStandardForceGen();

    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBStandardForceGen(
        const IBStandardForceGen& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBStandardForceGen&
    operator=(
        const IBStandardForceGen& that);

    /*!
     * The collection of force generation objects.
     */
    SAMRAI::tbox::Pointer<IBLagrangianForceStrategySet> d_force_strategy_set;
};
}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibamr/IBStandardForceGen.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBStandardForceGen
