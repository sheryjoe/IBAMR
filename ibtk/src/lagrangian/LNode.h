// Filename: LNode.h
// Created on 05 May 2011 by Boyce Griffith
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

#ifndef included_LNode
#define included_LNode

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBTK INCLUDES
#include <ibtk/LNodeIndex.h>
#include <ibtk/Streamable.h>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBTK
{
/*!
 * \brief Class LNode is the basic element of an LMesh.
 *
 * Class LNode provides Lagrangian and <A
 * HREF="http://www-unix.mcs.anl.gov/petsc">PETSc</A> indexing information and
 * data storage for a single node of a Lagrangian mesh.
 */
class LNode
    : public LNodeIndex
{
public:
    /*!
     * \brief Default constructor.
     */
    LNode(
        const int lagrangian_nidx=-1,
        const int global_petsc_nidx=-1,
        const int local_petsc_nidx=-1,
        const SAMRAI::hier::IntVector<NDIM>& periodic_offset=SAMRAI::hier::IntVector<NDIM>(0),
        const blitz::TinyVector<double,NDIM>& periodic_displacement=blitz::TinyVector<double,NDIM>(0.0),
        const std::vector<SAMRAI::tbox::Pointer<Streamable> >& node_data=std::vector<SAMRAI::tbox::Pointer<Streamable> >());

    /*!
     * \brief Copy constructor.
     *
     * \param from The value to copy to this object.
     */
    LNode(
        const LNode& from);

    /*!
     * \brief Virtuald destructor.
     *
     * The LNode destructor does nothing interesting.
     */
    virtual ~LNode();

    /*!
     * \brief Assignment operator.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    LNode&
    operator=(
        const LNode& that);

    /*!
     * \return A constant reference to any additional data items associated with
     * the node referenced by this LNode object.
     */
    const std::vector<SAMRAI::tbox::Pointer<Streamable> >&
    getNodeData() const;

    /*!
     * \return A non-constant reference to any additional data items associated
     * with the node referenced by this LNode object.
     */
    std::vector<SAMRAI::tbox::Pointer<Streamable> >&
    getNodeData();

    /*!
     * \brief Reset the collection of additional data items associated with the
     * node referenced by this LNode object.
     */
    void
    setNodeData(
        const std::vector<SAMRAI::tbox::Pointer<Streamable> >& node_data);

    /*!
     * \return A pointer to the first data item of type T associated with the
     * node referenced by the LNode object.
     *
     * If no object of the specified type is encountered, this method returns a
     * null pointer.
     *
     * \note It is possible for multiple objects of the same type to be
     * associated with each node.  This method returns only the \em first such
     * object encountered in the collection of data items associated with the
     * node.
     */
    template<typename T>
    SAMRAI::tbox::Pointer<T>
    getNodeData() const;

    /*!
     * \return A vector of pointers to all data items of type T associated with
     * the node referenced by the LNode object.
     *
     * If no object of the specified type is encountered, this method returns an
     * empty vector.
     *
     * \note It is possible for multiple objects of the same type to be
     * associated with each node.  This method returns a vector of \em all such
     * objects encountered in the collection of data items associated with the
     * node.
     */
    template<typename T>
    std::vector<SAMRAI::tbox::Pointer<T> >
    getNodeDataVector() const;

    /*!
     * \brief Indicate that the LNode object has been shifted across a periodic
     * boundary.
     */
    virtual void
    registerPeriodicShift(
        const SAMRAI::hier::IntVector<NDIM>& offset,
        const blitz::TinyVector<double,NDIM>& displacement);

    /*!
     * \brief Copy data from the source.
     *
     * \note The cell index of the destination object is src_index + src_offset.
     */
    virtual void
    copySourceItem(
        const SAMRAI::hier::Index<NDIM>& src_index,
        const SAMRAI::hier::IntVector<NDIM>& src_offset,
        const LNode& src_item);

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

    /*!
     * \brief Unpack data from the input stream.
     */
    virtual void
    unpackStream(
        SAMRAI::tbox::AbstractStream& stream,
        const SAMRAI::hier::IntVector<NDIM>& offset);

private:
    /*!
     * Assign that to this.
     */
    void
    assignThatToThis(
        const LNode& that);

    // a (possibly empty) collection of data objects that are associated with
    // the node
    std::vector<SAMRAI::tbox::Pointer<Streamable> > d_node_data;
};

}// namespace IBTK

/////////////////////////////// INLINE ///////////////////////////////////////

#include <ibtk/LNode.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_LNode