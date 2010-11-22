// Filename: IBRodForceSpec.h
// Created on 23 Jun 2010 by Boyce Griffith
//
// Copyright (c) 2002-2010, Boyce Griffith
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

#ifndef included_IBRodForceSpec
#define included_IBRodForceSpec

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBTK INCLUDES
#include <ibtk/Streamable.h>

// SAMRAI INCLUDES
#include <tbox/AbstractStream.h>

// C++ STDLIB INCLUDES
#include <vector>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class IBRodForceSpec encapsulates the data necessary to compute the
 * forces generated by a network of Kirchhoff rods at a single node of the
 * Lagrangian mesh.
 */
class IBRodForceSpec
    : public IBTK::Streamable
{
public:
    /*!
     * \brief Register this class and its factory class with the singleton
     * IBTK::StreamableManager object.  This method must be called before any
     * IBRodForceSpec objects are created.
     *
     * \note This method is collective on all MPI processes.  This is done to
     * ensure that all processes employ the same class ID for the
     * IBRodForceSpec class.
     */
    static void
    registerWithStreamableManager();

    /*!
     * \brief Returns a boolean indicating whether the class has been registered
     * with the singleton IBTK::StreamableManager object.
     */
    static bool
    getIsRegisteredWithStreamableManager();

    /*!
     * \brief Default constructor.
     *
     * \note The subdomain indices are ignored unless IBAMR is configured to
     * enable support for subdomain indices.  Subdomain indices are not enabled
     * by default.
     */
    IBRodForceSpec(
        const int master_idx=-1,
        const std::vector<int>& next_idxs=std::vector<int>(),
        const std::vector<std::vector<double> >& material_params=std::vector<std::vector<double> >(),
        const std::vector<int>& subdomain_idxs=std::vector<int>());

    /*!
     * \brief Virtual destructor.
     */
    virtual
    ~IBRodForceSpec();

    /*!
     * \return The number of rods attached to the master node.
     */
    unsigned
    getNumberOfRods() const;

    /*!
     * \return A const reference to the master node index.
     */
    const int&
    getMasterNodeIndex() const;

    /*!
     * \return A non-const reference to the master node index.
     */
    int&
    getMasterNodeIndex();

    /*!
     * \return A const reference to the next node indices for the rods attached
     * to the master node.
     */
    const std::vector<int>&
    getNextNodeIndices() const;

    /*!
     * \return A non-const reference to the next node indices for the rods
     * attached to the master node.
     */
    std::vector<int>&
    getNextNodeIndices();

    /*!
     * \return A const reference to the material parameters of the rods attached
     * to the master node.
     */
    const std::vector<std::vector<double> >&
    getMaterialParams() const;

    /*!
     * \return A non-const reference to the material parameters of the rods
     * attached to the master node.
     */
    std::vector<std::vector<double> >&
    getMaterialParams();

    /*!
     * \return A const reference to the subdomain indices associated with this
     * force spec object.
     *
     * \note IBAMR must be specifically configured to enable support for
     * subdomain indices.  Subdomain indices are not enabled by default.
     */
    const std::vector<int>&
    getSubdomainIndices() const;

    /*!
     * \return A non-const reference to the subdomain indices associated with
     * this force spec object.
     *
     * \note IBAMR must be specifically configured to enable support for
     * subdomain indices.  Subdomain indices are not enabled by default.
     */
    std::vector<int>&
    getSubdomainIndices();

    /*!
     * \brief Return the unique identifier used to specify the
     * IBTK::StreamableFactory object used by the IBTK::StreamableManager to
     * extract Streamable objects from data streams.
     */
    virtual int
    getStreamableClassID() const;

    /*!
     * \brief Return an upper bound on the amount of space required to pack the
     * object to a buffer.
     */
    virtual size_t
    getDataStreamSize() const;

    /*!
     * \brief Pack data into the output stream.
     */
    virtual void
    packStream(
        SAMRAI::tbox::AbstractStream& stream);

private:
    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBRodForceSpec(
        const IBRodForceSpec& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBRodForceSpec&
    operator=(
        const IBRodForceSpec& that);

    /*!
     * Indicates whether the factory has been registered with the
     * IBTK::StreamableManager.
     */
    static bool s_registered_factory;

    /*!
     * The class ID for this object type assigned by the
     * IBTK::StreamableManager.
     */
    static int s_class_id;

    /*!
     * Data required to define the spring forces.
     */
    int d_master_idx;
    std::vector<int> d_next_idxs;
    std::vector<std::vector<double> > d_material_params;

#if ENABLE_SUBDOMAIN_INDICES
    /*!
     * The subdomain indices of the force spec object.
     */
    std::vector<int> d_subdomain_idxs;
#endif
};
}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

#include <ibamr/IBRodForceSpec.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBRodForceSpec
