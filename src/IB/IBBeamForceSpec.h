// Filename: IBBeamForceSpec.h
// Created on 22 Mar 2007 by Boyce Griffith
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

#ifndef included_IBBeamForceSpec
#define included_IBBeamForceSpec

/////////////////////////////// INCLUDES /////////////////////////////////////

#ifndef included_IBAMR_prefix_config
#include <IBAMR_prefix_config.h>
#define included_IBAMR_prefix_config
#endif

// IBTK INCLUDES
#include <ibtk/Streamable.h>
#include <ibtk/StreamableFactory.h>

// SAMRAI INCLUDES
#include <tbox/AbstractStream.h>

// BLITZ++ INCLUDES
#include <blitz/tinyvec.h>

// C++ STDLIB INCLUDES
#include <utility>
#include <vector>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class IBBeamForceSpec encapsulates the data necessary to compute the
 * forces generated by a collection of linear beams (i.e., structures that
 * resist bending) at a single node of the Lagrangian mesh.
 *
 * Beams are connections between three particular nodes of the Lagrangian mesh.
 * IBBeamForceSpec objects are stored as IBTK::Streamable data associated with
 * only the master beam nodes in the mesh.
 */
class IBBeamForceSpec
    : public IBTK::Streamable
{
public:
    /*!
     * \note This typedef appears to be needed to get g++ to parse the default
     * parameters in the class constructor.
     */
    typedef std::pair<int,int> NeighborIdxs;

    /*!
     * \brief Register this class and its factory class with the singleton
     * IBTK::StreamableManager object.  This method must be called before any
     * IBBeamForceSpec objects are created.
     *
     * \note This method is collective on all MPI processes.  This is done to
     * ensure that all processes employ the same class ID for the
     * IBBeamForceSpec class.
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
     * The unique class ID for this object type assigned by the
     * IBTK::StreamableManager.
     */
    static int STREAMABLE_CLASS_ID;

    /*!
     * \brief Default constructor.
     */
    IBBeamForceSpec(
        unsigned int num_beams=0);

    /*!
     * \brief Alternative constructor.
     */
    IBBeamForceSpec(
        int master_idx,
        const std::vector<NeighborIdxs>& neighbor_idxs,
        const std::vector<double>& bend_rigidities,
        const std::vector<blitz::TinyVector<double,NDIM> >& mesh_dependent_curvatures);

    /*!
     * \brief Destructor.
     */
    ~IBBeamForceSpec();

    /*!
     * \return The number of beams attached to the master node.
     */
    unsigned int
    getNumberOfBeams() const;

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
     * \return A const reference to the neighbor node indices for the beams
     * attached to the master node.
     */
    const std::vector<NeighborIdxs>&
    getNeighborNodeIndices() const;

    /*!
     * \return A non-const reference to the neighbor node indices for the beams
     * attached to the master node.
     */
    std::vector<NeighborIdxs>&
    getNeighborNodeIndices();

    /*!
     * \return A const reference to the bending rigidities of the beams attached
     * to the master node.
     */
    const std::vector<double>&
    getBendingRigidities() const;

    /*!
     * \return A non-const reference to the bending rigidities of the beams
     * attached to the master node.
     */
    std::vector<double>&
    getBendingRigidities();

    /*!
     * \return A const reference to the mesh-dependent curvatures of the beams
     * attached to the master node.
     */
    const std::vector<blitz::TinyVector<double,NDIM> >&
    getMeshDependentCurvatures() const;

    /*!
     * \return A non-const reference to the mesh-dependent curvatures of the
     * beams attached to the master node.
     */
    std::vector<blitz::TinyVector<double,NDIM> >&
    getMeshDependentCurvatures();

    /*!
     * \brief Return the unique identifier used to specify the
     * IBTK::StreamableFactory object used by the IBTK::StreamableManager to
     * extract Streamable objects from data streams.
     */
    int
    getStreamableClassID() const;

    /*!
     * \brief Return an upper bound on the amount of space required to pack the
     * object to a buffer.
     */
    size_t
    getDataStreamSize() const;

    /*!
     * \brief Pack data into the output stream.
     */
    void
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
    IBBeamForceSpec(
        const IBBeamForceSpec& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBBeamForceSpec&
    operator=(
        const IBBeamForceSpec& that);

    /*!
     * Data required to compute the beam forces.
     */
    int d_master_idx;
    std::vector<NeighborIdxs> d_neighbor_idxs;
    std::vector<double> d_bend_rigidities;
    std::vector<blitz::TinyVector<double,NDIM> > d_mesh_dependent_curvatures;

    /*!
     * \brief A factory class to rebuild IBBeamForceSpec objects from
     * SAMRAI::tbox::AbstractStream data streams.
     */
    class Factory
        : public IBTK::StreamableFactory
    {
    public:
        /*!
         * \brief Destructor.
         */
        ~Factory();

        /*!
         * \brief Return the unique identifier used to specify the
         * IBTK::StreamableFactory object used by the IBTK::StreamableManager to
         * extract IBBeamForceSpec objects from data streams.
         */
        int
        getStreamableClassID() const;

        /*!
         * \brief Set the unique identifier used to specify the
         * IBTK::StreamableFactory object used by the IBTK::StreamableManager to
         * extract IBBeamForceSpec objects from data streams.
         */
        void
        setStreamableClassID(
            int class_id);

        /*!
         * \brief Build an IBBeamForceSpec object by unpacking data from the
         * data stream.
         */
        SAMRAI::tbox::Pointer<IBTK::Streamable>
        unpackStream(
            SAMRAI::tbox::AbstractStream& stream,
            const SAMRAI::hier::IntVector<NDIM>& offset);

    private:
        /*!
         * \brief Default constructor.
         */
        Factory();

        /*!
         * \brief Copy constructor.
         *
         * \note This constructor is not implemented and should not be used.
         *
         * \param from The value to copy to this object.
         */
        Factory(
            const Factory& from);

        /*!
         * \brief Assignment operator.
         *
         * \note This operator is not implemented and should not be used.
         *
         * \param that The value to assign to this object.
         *
         * \return A reference to this object.
         */
        Factory&
        operator=(
            const Factory& that);

        friend class IBBeamForceSpec;
    };
    typedef IBBeamForceSpec::Factory IBBeamForceSpecFactory;
};
}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

#include <ibamr/IBBeamForceSpec.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBBeamForceSpec
