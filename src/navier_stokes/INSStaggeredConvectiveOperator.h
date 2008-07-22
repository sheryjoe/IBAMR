#ifndef included_INSStaggeredConvectiveOperator
#define included_INSStaggeredConvectiveOperator

// Filename: INSStaggeredConvectiveOperator.h
// Last modified: <22.Jul.2008 15:11:37 griffith@box230.cims.nyu.edu>
// Created on 08 May 2008 by Boyce Griffith (griffith@box230.cims.nyu.edu)

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBAMR INCLUDES
#include <ibamr/INSStaggeredPhysicalBoundaryHelper.h>

// IBTK INCLUDES
#include <ibtk/GeneralOperator.h>

// SAMRAI INCLUDES
#include <RefineAlgorithm.h>
#include <RefineOperator.h>
#include <SideVariable.h>

// C++ STDLIB INCLUDES
#include <vector>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class INSStaggeredConvectiveOperator is a concrete
 * IBTK::GeneralOperator which implements a upwind convective differencing
 * operator based on the piecewise parabolic method (PPM).
 *
 * \see INSStaggeredHierarchyIntegrator
 */
class INSStaggeredConvectiveOperator
    : public IBTK::GeneralOperator
{
public:
    /*!
     * \brief Class constructor.
     */
    INSStaggeredConvectiveOperator(
        const double rho,
        const double mu,
        const double lambda,
        const bool conservation_form,
        const SAMRAI::tbox::Pointer<INSStaggeredPhysicalBoundaryHelper>& U_bc_helper);

    /*!
     * \brief Virtual destructor.
     */
    virtual
    ~INSStaggeredConvectiveOperator();

    /*!
     * \brief Compute the actaion of the convective operator.
     */
    void
    applyConvectiveOperator(
        const int U_idx,
        const int N_idx);

    /*!
     * \name General operator functionality.
     */
    //\{

    /*!
     * \brief Compute \f$y=F[x]\f$.
     *
     * Before calling apply(), the form of the vectors \a x and \a y should be
     * set properly by the user on all patch interiors on the specified range of
     * levels in the patch hierarchy.  The user is responsible for all data
     * management for the quantities associated with the vectors.  In
     * particular, patch data in these vectors must be allocated prior to
     * calling this method.
     *
     * \param x input vector
     * \param y output vector, i.e., \f$y=F[x]\f$
     *
     * <b>Conditions on Parameters:</b>
     * - vectors \a x and \a y must have same hierarchy
     * - vectors \a x and \a y must have same structure, depth, etc.
     *
     * In general, the vectors \a x and \a y \em cannot be the same.
     *
     * \see initializeOperatorState
     */
    virtual void
    apply(
        SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& x,
        SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& y)
        {
            // Initialize the operator (if necessary).
            const bool deallocate_at_completion = !d_is_initialized;
            if (!d_is_initialized) initializeOperatorState(x,y);

            // Get the vector components.
            const int U_idx = x.getComponentDescriptorIndex(0);
            const int N_idx = y.getComponentDescriptorIndex(0);

            // Compute the action of the operator.
            applyConvectiveOperator(U_idx, N_idx);

            // Deallocate the operator (if necessary).
            if (deallocate_at_completion) deallocateOperatorState();
            return;
        }// apply

    /*!
     * \brief Compute hierarchy dependent data required for computing y=F[x] and
     * z=F[x]+y.
     *
     * The vector arguments for apply(), applyAdjoint(), etc, need not match
     * those for initializeOperatorState().  However, there must be a certain
     * degree of similarity, including
     * - hierarchy configuration (hierarchy pointer and level range)
     * - number, type and alignment of vector component data
     * - ghost cell widths of data in the input and output vectors
     *
     * \note It is generally necessary to reinitialize the operator state when
     * the hierarchy configuration changes.
     *
     * It is safe to call initializeOperatorState() when the state is already
     * initialized.  In this case, the operator state is first deallocated and
     * then reinitialized.
     *
     * Conditions on arguments:
     * - input and output vectors must have same hierarchy
     * - input and output vectors must have same structure, depth, etc.
     *
     * Call deallocateOperatorState() to remove any data allocated by this
     * method.
     *
     * \see deallocateOperatorState
     *
     * \param in input vector
     * \param out output vector
     */
    virtual void
    initializeOperatorState(
        const SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& in,
        const SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& out);

    /*!
     * \brief Remove all hierarchy dependent data allocated by
     * initializeOperatorState().
     *
     * \note It is safe to call deallocateOperatorState() when the operator
     * state is already deallocated.
     *
     * \see initializeOperatorState
     */
    virtual void
    deallocateOperatorState();

    //\}

    /*!
     * \name Logging functions.
     */
    //\{

    /*!
     * \brief Enable or disable logging.
     *
     * \param enabled logging state: true=on, false=off
     */
    virtual void
    enableLogging(
        bool enabled=true)
        {
            // intentionally blank
            return;
        }// enableLogging

    /*!
     * \brief Print out internal class data for debugging.
     */
    virtual void
    printClassData(
        std::ostream& os) const
        {
            // intentionally blank
            return;
        }// printClassData

    //\}

private:
    /*!
     * \brief Default constructor.
     *
     * \note This constructor is not implemented and should not be used.
     */
    INSStaggeredConvectiveOperator();

    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    INSStaggeredConvectiveOperator(
        const INSStaggeredConvectiveOperator& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    INSStaggeredConvectiveOperator&
    operator=(
        const INSStaggeredConvectiveOperator& that);

    // Whether the operator is initialized.
    bool d_is_initialized;

    // Problem coefficients.
    const double d_rho;
    const double d_mu;
    const double d_lambda;

    // Whether to use conservative or non-conservative differencing.
    const bool d_conservation_form;

    // Data communication algorithms, operators, and schedules.
    SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineAlgorithm<NDIM> > d_refine_alg;
    SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineOperator<NDIM> > d_refine_op;
    SAMRAI::tbox::Pointer<SAMRAI::xfer::RefinePatchStrategy<NDIM> > d_refine_strategy;
    std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > > d_refine_scheds;

    // Hierarchy configuration.
    SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > d_hierarchy;
    int d_coarsest_ln, d_finest_ln;

    // Boundary condition objects.
    const SAMRAI::tbox::Pointer<INSStaggeredPhysicalBoundaryHelper>& d_U_bc_helper;

    // Scratch data.
    SAMRAI::tbox::Pointer<SAMRAI::pdat::SideVariable<NDIM,double> > d_U_var;
    int d_U_scratch_idx;
};
}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibamr/INSStaggeredConvectiveOperator.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_INSStaggeredConvectiveOperator
